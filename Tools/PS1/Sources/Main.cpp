/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2022.  All rights reserved
 */
#include <filesystem>
#include <fstream>
#include <iostream>

#include "Stroika/Foundation/DataExchange/ObjectVariantMapper.h"
#include "Stroika/Foundation/DataExchange/StructFieldMetaInfo.h"
#include "Stroika/Foundation/DataExchange/Variant/JSON/Reader.h"
#include "Stroika/Foundation/DataExchange/Variant/JSON/Writer.h"
#include "Stroika/Foundation/Debug/TimingTrace.h"
#include "Stroika/Foundation/Debug/Trace.h"
#include "Stroika/Foundation/IO/FileSystem/FileOutputStream.h"
#include "Stroika/Foundation/Time/DateTime.h"

#include "Digikam.h"
#include "ImageMetadataExtraction.h"
#include "LibIPAM/Metadata/Document.h"

namespace {
    constexpr wstring_view kMyTopLevelDirectory = L"P:/";

    const bool kTallyExtensions  = true;
    const bool kScrapeFileSystem = true;
    const bool kScrapeDigikamDB  = true;
    const bool kCreateMasterFile = true;

    //const String kSourceDirectory = L"P:/2022/May/Costa Rica";
    const String kSourceDirectory = L"P:/1900-1909";
    //String    kSourceDirectory                = L"P:/";
    const String kOutputDirectory                = L"c:/ssw/mdResults/";
    const String kSampleExtractionFilesDirectory = kOutputDirectory;
    const String kDigikamDatabase                = L"c:/Digikam/digikam4.db";

    const wstring kDigikamScrapeFileName  = L"DigikamScrape.json";
    const wstring kFileScrapeFileName     = L"FileScrape.json";
    const wstring kMergedTagsFileName     = L"DocumentMetaData.json";
    const wstring kExtensionTallyFileName = L"ExtenstionTally.json";
}

using namespace std::filesystem;

int main ([[maybe_unused]] int argc, [[maybe_unused]] const char* argv[])
{
    Debug::TraceContextBumper ctx{Stroika_Foundation_Debug_OptionalizeTraceArgs (L"main", L"argv=%s", Characters::ToString (vector<const char*>{argv, argv + argc}).c_str ())};

    path digikamScrapeFilePath = (kOutputDirectory + kDigikamScrapeFileName).c_str ();
    path fileScrapeFilePath    = (kOutputDirectory + kFileScrapeFileName).c_str ();

    Containers::Mapping<String, Metadata::Document> mergedMetaData;

    Containers::Mapping<String, Metadata::Document> fileScrape;
    if (kScrapeFileSystem) {
        {
            DbgTrace (L"scraping file system directory at %s", kSourceDirectory.c_str ());
            Debug::TimingTrace ttrc;
            fileScrape = Metadata::ImageMetadataExtractor ().ExtractAll (path (kSourceDirectory.c_str ()));
        }
        {
            DbgTrace (L"writing file system scrape to %s", fileScrapeFilePath.c_str ());
            Debug::TimingTrace ttrc;
            Metadata::Document::WriteToFileAsJSON (fileScrape, fileScrapeFilePath);
        }
        mergedMetaData = fileScrape;
    }

    if (kTallyExtensions) {
        DbgTrace (L"tallying extenstions for directory = %s", kSourceDirectory.c_str ());
        Debug::TimingTrace ttrc;

        Containers::MultiSet<String> extTally = Metadata::ImageMetadataExtractor ().TallyExtensions (path (kSourceDirectory.c_str ()), kSampleExtractionFilesDirectory);

        DataExchange::ObjectVariantMapper mapper;
        mapper.AddCommonType<Containers::MultiSet<String>> ();
        mapper.AddCommonType<Containers::CountedValue<String>> ();

        path extenstionTallyPath = (kOutputDirectory + kExtensionTallyFileName).c_str ();

        DataExchange::Variant::JSON::Writer{}.Write (mapper.FromObject (extTally), IO::FileSystem::FileOutputStream::New (extenstionTallyPath));
    }

    Containers::Mapping<String, Metadata::Document> dbScrape;
    if (kScrapeDigikamDB) {
        {
            DbgTrace (L"scraping digikam database at %s", kDigikamDatabase.c_str ());
            Debug::TimingTrace ttrc;
            dbScrape = digikam::ScrapeDigikamDB (kDigikamDatabase);
        }
        {
            DbgTrace (L"writing digikam scrape to %s", digikamScrapeFilePath.c_str ());
            Debug::TimingTrace ttrc;
            Metadata::Document::WriteToFileAsJSON (dbScrape, digikamScrapeFilePath);
        }
    }

    if (kCreateMasterFile) {
        if (not kScrapeFileSystem) {
            // read in an old copy
            DbgTrace (L"about to read file system metadata");
            Debug::TimingTrace ttrc;
            Metadata::Document::ReadFromJSONFile (&fileScrape, fileScrapeFilePath);
        }
        if (not kScrapeDigikamDB) {
            // read in an old copy
            DbgTrace (L"about to read digikam metadata from %s", digikamScrapeFilePath.c_str ());
            Debug::TimingTrace ttrc;
            Metadata::Document::ReadFromJSONFile (&dbScrape, digikamScrapeFilePath);
        }

        DbgTrace (L"merging file and database sources, files length = %d, db length = %d", fileScrape.Keys ().length (), dbScrape.Keys ().length ());
        Containers::Mapping<String, Metadata::Document> masterList;
        for (const auto& it : fileScrape) {
            Metadata::Document dmd = it.fValue;
            Metadata::Document digikamDmd;
            if (dbScrape.Lookup (it.fKey, &digikamDmd)) {
                String ext                               = String{path (it.fKey.c_str ()).extension ().wstring ()}.ToLowerCase ();
                bool   ignoreMissingFromFileScrapeForNow = (ext == L".nef" or ext == L".heic" or ext == L".mov" or ext == L".bmp");

                dmd.album = digikamDmd.album; // digikam does better here at capture correct top of collection when you don't do full file scan
                if (digikamDmd.comment.has_value ()) {
                    if (dmd.comment.has_value ()) {
                        if (dmd.comment.value () != digikamDmd.comment.value ()) { // should just be assert
                            DbgTrace (L"COMMENT disagreement for %s (%s vs %s)", it.fKey.c_str (), Metadata::Document::Comment::ToString (dmd.comment.value ()).c_str (), Metadata::Document::Comment::ToString (digikamDmd.comment.value ()).c_str ());
                        }
                    }
                    else {
                        DbgTrace (L"adding missing comment for %s (adding %s)", it.fKey.c_str (), Metadata::Document::Comment::ToString (digikamDmd.comment.value ()).c_str ());
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

                if (dmd.tags != digikamDmd.tags) { // should be assert but can only do release build
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
        if ((kSourceDirectory == kMyTopLevelDirectory) and true) {
            DbgTrace (L"adding digikam only info to master list");
            Debug::TimingTrace ttrc;
            for (const auto& it : dbScrape) {
                Metadata::Document digikamDmd = it.fValue;
                Metadata::Document dmd;
                if (not fileScrape.Lookup (it.fKey, &dmd)) {
                    //          DbgTrace (L"adding missing dmd from digikam: %s", it.fKey.c_str ());
                    masterList.Add (it.fKey, digikamDmd);
                }
            }
        }

        {
            auto outputPath = filesystem::path ((kOutputDirectory + kMergedTagsFileName).c_str ());
            DbgTrace (L"writing processed tag info to %s", outputPath.c_str ());
            Debug::TimingTrace ttrc;
            Metadata::Document::WriteToFileAsJSON (fileScrape, outputPath);
        }
    }

    return EXIT_SUCCESS;
}
