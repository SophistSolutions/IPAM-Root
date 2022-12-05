/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2022.  All rights reserved
 */
#include <iostream>
#include <filesystem>
#include <fstream>

#include "Stroika/Foundation/Debug/Trace.h"
#include "Stroika/Foundation/DataExchange/ObjectVariantMapper.h"
#include "Stroika/Foundation/DataExchange/StructFieldMetaInfo.h"
#include "Stroika/Foundation/DataExchange/Variant/JSON/Reader.h"
#include "Stroika/Foundation/Time/DateTime.h"

#include "Digikam.h"
#include "DocumentMetadata.h"
#include "ImageMetadataExtraction.h"

using  Metadata::DocumentMetadata;


#include <chrono>
using namespace std::chrono;


namespace {
    const bool  kTallyExtensions  = false;
    const bool  kScrapeFileSystem = false;
    const bool  kScrapeDigikamDB  = true;
    const bool  kCreateMasterFile = true;

    //const char* kSourceDirectory  = "P:/1900-1909";
    const char* kSourceDirectory = "P:/";
    const char* kOutputDirectory = "c:/ssw/mdResults/";
    const char* kSampleExtractionFilesDirectory = kOutputDirectory;
    const char* kDigikamDatabase = "c:/Digikam/digikam4.db";

    const String kDigikamScrapeFileName = L"DigikamScrape.json";
    const String kFileScrapeFileName    = L"FileScrape.json";
    const String kMergedTagsFileName    = L"DocumentMetaData.json";

}


using namespace std::filesystem;


int main([[maybe_unused]] int argc, [[maybe_unused]] const char* argv[])
{
    Debug::TraceContextBumper ctx{ Stroika_Foundation_Debug_OptionalizeTraceArgs(L"main", L"argv=%s", Characters::ToString(vector<const char*>{argv, argv + argc}).c_str()) };

    Containers::Mapping<String, DocumentMetadata> mergedMetaData;

    Containers::Mapping<String, DocumentMetadata> fileScrape;
    if (kScrapeFileSystem) {
        fileScrape = Metadata::ImageMetadataExtractor().ExtractAll(kSourceDirectory);
        DocumentMetadata::WriteToFileAsJSON(
            fileScrape,
            String::FromNarrowSDKString (kOutputDirectory) + kFileScrapeFileName);
        mergedMetaData = fileScrape;
    }

    if (kTallyExtensions) {
        Containers::MultiSet<String> extTally = Metadata::ImageMetadataExtractor().TallyExtensions(kSourceDirectory, kSampleExtractionFilesDirectory);
        DbgTrace(L"****************************");
        for (const auto& i : extTally) {
            DbgTrace(L"%s : %d", i.fValue.c_str(), i.fCount);
        }
    }
    Containers::Mapping<String, DocumentMetadata> dbScrape;
    if (kScrapeDigikamDB) {
        auto start = high_resolution_clock::now ();
        dbScrape   = digikam::ScrapeDigikamDB (kDigikamDatabase);
        Metadata::DocumentMetadata::WriteToFileAsJSON(
            dbScrape,
            String::FromNarrowSDKString(kOutputDirectory) + kDigikamScrapeFileName);
        auto stop = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(stop - start);
        DbgTrace(L"scraping digikam and writing json took %ld milliseconds", duration);
    }

    if (kCreateMasterFile) {
        if (not kScrapeFileSystem) {
            // read in an old copy
            DbgTrace (L"about to read file system metadata");
            auto start = high_resolution_clock::now ();
            DocumentMetadata::ReadFromJSONFile (&fileScrape, (String::FromNarrowSDKString (kOutputDirectory) + kFileScrapeFileName).c_str ());
            auto stop     = high_resolution_clock::now ();
            auto duration = duration_cast<milliseconds> (stop - start);
            DbgTrace (L"reading file system  json took %ld milliseconds", duration);
        }
        if (not kScrapeDigikamDB) {
            // read in an old copy
            DbgTrace(L"about to read digikam metadata");
            auto start = high_resolution_clock::now ();
            DocumentMetadata::ReadFromJSONFile (&dbScrape, (String::FromNarrowSDKString (kOutputDirectory) + kDigikamScrapeFileName).c_str ());
            auto stop     = high_resolution_clock::now ();
            auto duration = duration_cast<milliseconds> (stop - start);
            DbgTrace (L"reading digikam json took %ld milliseconds", duration);
        }
        DbgTrace (L"merging file and database sources, files length = %d, db length = %d", fileScrape.Keys ().length (), dbScrape.Keys ().length ());

        Containers::Mapping<String, DocumentMetadata> masterList;
        for (const auto& it : fileScrape) {
            DocumentMetadata dmd = it.fValue;
            DocumentMetadata digikamDmd;
            if (dbScrape.Lookup(it.fKey, &digikamDmd)) {
                String ext = String{path(it.fKey.c_str ()).extension ().wstring ()}.ToLowerCase ();
                bool ignoreMissingFromFileScrapeForNow = (ext == L".nef" or ext == L".heic" or ext == L".mov" or ext == L".bmp");

                dmd.album = digikamDmd.album;    // digikam does better here at capture correct top of collection when you don't do full file scan
                if (digikamDmd.comment.has_value()) {
                    if (dmd.comment.has_value()) {
                        if (dmd.comment.value () != digikamDmd.comment.value ()) { // should just be assert
                            DbgTrace (L"COMMENT disagreement for %s (%s vs %s)", it.fKey.c_str (), DocumentMetadata::Comment::ToString (dmd.comment.value ()).c_str (), DocumentMetadata::Comment::ToString (digikamDmd.comment.value ()).c_str ());                       
                        }
                    }
                    else {
                        DbgTrace (L"adding missing comment for %s (adding %s)", it.fKey.c_str (), DocumentMetadata::Comment::ToString (digikamDmd.comment.value ()).c_str ());
                        dmd.comment = digikamDmd.comment;               
                    }
                }
                if (digikamDmd.date.has_value ()) {
                    if (dmd.date.has_value ()) {
                        if (dmd.date.value () != digikamDmd.date.value ()) { 
                            DbgTrace (L"DATE disagreement for %s (%s vs %s)", it.fKey.c_str (), dmd.date.value ().c_str (), digikamDmd.date.value ().c_str ());
                        }
                    }
                    else {
       //                 DbgTrace (L"adding missing date for %s (adding %s)", it.fKey.c_str (), digikamDmd.date.value ().c_str ());
                        dmd.date = digikamDmd.date;
                    }
                }
                if (digikamDmd.location.has_value ()) {
                    if (dmd.location.has_value ()) {
                        if (dmd.location.value () != digikamDmd.location.value ()) { // should just be assert
                            DbgTrace (L"LOCATION disagreement for %s (%s vs %s)", it.fKey.c_str (), dmd.location.value ().c_str (), digikamDmd.location.value ().c_str ());
                        }
                    }
                    else {
          //              DbgTrace (L"adding missing location for %s (adding %s)", it.fKey.c_str (), digikamDmd.location.value ().c_str ());
                        dmd.location = digikamDmd.location;
                    }
                }
                if (digikamDmd.rating.has_value ()) {
                    if (dmd.rating.has_value ()) {
                        if (dmd.rating.value () != digikamDmd.rating.value ()) { // should just be assert
                            DbgTrace (L"RATING DISAGREEMENT for %s %f : %f", it.fKey.c_str (), dmd.rating.value (), digikamDmd.rating.value ());
                        }
                    }
                    else {
                        DbgTrace (L"adding missing rating for %s (adding %f)", it.fKey.c_str (), digikamDmd.rating.value ());
                        dmd.rating = digikamDmd.rating;
                    }
                }

                if (dmd.tags != digikamDmd.tags) {  // should be assert but can only do release build
                    for (String tag : dmd.tags) {
                        if (not digikamDmd.tags.Contains (tag)) {
                            DbgTrace (L"FOUND TAG MISSING FROM DIGIKAM %s : %s", it.fKey.c_str (), tag.c_str ());
                        }
                    }

                    for (String tag : digikamDmd.tags) {
                        if (not dmd.tags.Contains (tag) and not ignoreMissingFromFileScrapeForNow) {
                            DbgTrace (L"found tag missing from file scrape %s : %s", it.fKey.c_str (), tag.c_str ());
                            dmd.tags.Add (tag);
                        }
                    }               
                }
            }
            else {
                DbgTrace (L"missing digikam dmd for : %s", it.fKey.c_str ());
            }
            masterList.Add (it.fKey, dmd);

        }
        if ((strcmp (kSourceDirectory,"P:/") == 0) and true) {
            for (const auto& it : dbScrape) {
                DocumentMetadata digikamDmd = it.fValue;
                DocumentMetadata dmd;
                if (not fileScrape.Lookup (it.fKey, &dmd)) {
          //          DbgTrace (L"adding missing dmd from digikam: %s", it.fKey.c_str ());
                    masterList.Add (it.fKey, digikamDmd);
                }
            }
        }


        DocumentMetadata::WriteToFileAsJSON (
            fileScrape,
            String::FromNarrowSDKString (kOutputDirectory) + kMergedTagsFileName);
    }

    return EXIT_SUCCESS;
}

