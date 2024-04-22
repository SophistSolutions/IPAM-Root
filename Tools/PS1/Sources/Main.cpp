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
#include "Stroika/Foundation/Execution/CommandLine.h"
#include "Stroika/Foundation/IO/FileSystem/FileOutputStream.h"
#include "Stroika/Foundation/Time/DateTime.h"

#include "Digikam.h"
#include "ImageMetadataExtraction.h"
#include "LibIPAM/Metadata/Document.h"

using namespace Stroika::Foundation::Characters::Literals;

namespace {
    constexpr wstring_view kMyTopLevelDirectory = L"P:/";

    constexpr bool kTallyExtensions  = true;
    constexpr bool kScrapeFileSystem = true;
    constexpr bool kScrapeDigikamDB  = true;
    constexpr bool kCreateMasterFile = true;

    //const String kSourceDirectory = L"P:/2022/May/Costa Rica";
    const String kSourceDirectory = "P:/1900-1909"sv;
    //String    kSourceDirectory                = L"P:/";
    const String kOutputDirectory                = "c:/ssw/mdResults/"sv;
    const String kSampleExtractionFilesDirectory = kOutputDirectory;
    const String kDigikamDatabase                = "c:/Digikam/digikam4.db"sv;

    const wstring kDigikamScrapeFileName  = L"DigikamScrape.json";
    const wstring kFileScrapeFileName     = L"FileScrape.json";
    const wstring kMergedTagsFileName     = L"DocumentMetaData.json";
    const wstring kExtensionTallyFileName = L"ExtenstionTally.json";
}

using namespace std::filesystem;

int main ([[maybe_unused]] int argc, [[maybe_unused]] const char* argv[])
{
    Debug::TraceContextBumper ctx{"main", "argv={}"_f, Execution::CommandLine{argc, argv}};

    path digikamScrapeFilePath = (kOutputDirectory + kDigikamScrapeFileName).c_str ();
    path fileScrapeFilePath    = (kOutputDirectory + kFileScrapeFileName).c_str ();

    Containers::Mapping<String, Metadata::Document> mergedMetaData;

    Containers::Mapping<String, Metadata::Document> fileScrape;
    if (kScrapeFileSystem) {
        {
            DbgTrace ("scraping file system directory at {}"_f, kSourceDirectory);
            Debug::TimingTrace ttrc;
            fileScrape = Metadata::ImageMetadataExtractor ().ExtractAll (path (kSourceDirectory.As<wstring> ().c_str ()));
        }
        {
            DbgTrace ("writing file system scrape to {}"_f, fileScrapeFilePath);
            Debug::TimingTrace ttrc;
            Metadata::Document::WriteToFileAsJSON (fileScrape, fileScrapeFilePath);
        }
        mergedMetaData = fileScrape;
    }

    if (kTallyExtensions) {
        DbgTrace ("tallying extenstions for directory = {}"_f, kSourceDirectory);
        Debug::TimingTrace ttrc;

        Containers::MultiSet<String> extTally =
            Metadata::ImageMetadataExtractor ().TallyExtensions (path (kSourceDirectory.As<wstring> ().c_str ()), kSampleExtractionFilesDirectory);

        DataExchange::ObjectVariantMapper mapper;
        mapper.AddCommonType<Containers::MultiSet<String>> ();
        mapper.AddCommonType<Containers::CountedValue<String>> ();

        path extenstionTallyPath = (kOutputDirectory + kExtensionTallyFileName).c_str ();

        DataExchange::Variant::JSON::Writer{}.Write (mapper.FromObject (extTally), IO::FileSystem::FileOutputStream::New (extenstionTallyPath));
    }

    Containers::Mapping<String, Metadata::Document> dbScrape;
    if (kScrapeDigikamDB) {
        {
            DbgTrace ("scraping digikam database at {}"_f, kDigikamDatabase);
            Debug::TimingTrace ttrc;
            dbScrape = digikam::ScrapeDigikamDB (kDigikamDatabase);
        }
        {
            DbgTrace ("writing digikam scrape to {}"_f, digikamScrapeFilePath);
            Debug::TimingTrace ttrc;
            Metadata::Document::WriteToFileAsJSON (dbScrape, digikamScrapeFilePath);
        }
    }

    if (kCreateMasterFile) {
        if (not kScrapeFileSystem) {
            // read in an old copy
            DbgTrace ("about to read file system metadata"_f);
            Debug::TimingTrace ttrc;
            Metadata::Document::ReadFromJSONFile (&fileScrape, fileScrapeFilePath);
        }
        if (not kScrapeDigikamDB) {
            // read in an old copy
            DbgTrace ("about to read digikam metadata from {}"_f, digikamScrapeFilePath);
            Debug::TimingTrace ttrc;
            Metadata::Document::ReadFromJSONFile (&dbScrape, digikamScrapeFilePath);
        }

        DbgTrace ("merging file and database sources, files length = {}, db length = {}"_f, fileScrape.Keys ().length (), dbScrape.Keys ().length ());
        Containers::Mapping<String, Metadata::Document> masterList;
        for (const auto& it : fileScrape) {
            Metadata::Document dmd = it.fValue;
            Metadata::Document digikamDmd;
            if (dbScrape.Lookup (it.fKey, &digikamDmd)) {
                String ext = String{path (it.fKey.As<wstring> ().c_str ()).extension ().wstring ()}.ToLowerCase ();
                bool   ignoreMissingFromFileScrapeForNow = (ext == L".nef" or ext == L".heic" or ext == L".mov" or ext == L".bmp");

                dmd.album = digikamDmd.album; // digikam does better here at capture correct top of collection when you don't do full file scan
                if (digikamDmd.comment.has_value ()) {
                    if (dmd.comment.has_value ()) {
                        if (dmd.comment.value () != digikamDmd.comment.value ()) { // should just be assert
                            DbgTrace ("COMMENT disagreement for {} ({} vs {})"_f, it.fKey,
                                      Metadata::Document::Comment::ToString (dmd.comment.value ()),
                                      Metadata::Document::Comment::ToString (digikamDmd.comment.value ()));
                        }
                    }
                    else {
                        DbgTrace ("adding missing comment for {} (adding {})"_f, it.fKey,
                                  Metadata::Document::Comment::ToString (digikamDmd.comment.value ()));
                        dmd.comment = digikamDmd.comment;
                    }
                }
                if (digikamDmd.date.has_value ()) {
                    if (dmd.date.has_value ()) {
                        if (dmd.date.value () != digikamDmd.date.value ()) {
                            DbgTrace ("DATE disagreement for {} ({} vs {}"_f, it.fKey, dmd.date.value (), digikamDmd.date.value ());
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
                            DbgTrace ("LOCATION disagreement for {} ({} vs {})"_f, it.fKey, dmd.location.value (), digikamDmd.location.value ());
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
                            DbgTrace ("RATING DISAGREEMENT for {} {} : {}"_f, it.fKey, dmd.rating.value (), digikamDmd.rating.value ());
                        }
                    }
                    else {
                        DbgTrace ("adding missing rating for {} (adding {})"_f, it.fKey, digikamDmd.rating.value ());
                        dmd.rating = digikamDmd.rating;
                    }
                }

                if (dmd.tags != digikamDmd.tags) { // should be assert but can only do release build
                    for (String tag : dmd.tags) {
                        if (not digikamDmd.tags.Contains (tag)) {
                            DbgTrace ("FOUND TAG MISSING FROM DIGIKAM {} : {}"_f, it.fKey, tag);
                        }
                    }

                    for (String tag : digikamDmd.tags) {
                        if (not dmd.tags.Contains (tag) and not ignoreMissingFromFileScrapeForNow) {
                            DbgTrace ("found tag missing from file scrape {} : {}"_f, it.fKey, tag);
                            dmd.tags.Add (tag);
                        }
                    }
                }
            }
            else {
                DbgTrace ("missing digikam dmd for : {}"_f, it.fKey);
            }
            masterList.Add (it.fKey, dmd);
        }
        if ((kSourceDirectory == kMyTopLevelDirectory) and true) {
            DbgTrace ("adding digikam only info to master list"_f);
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
            DbgTrace ("writing processed tag info to {}"_f, outputPath);
            Debug::TimingTrace ttrc;
            Metadata::Document::WriteToFileAsJSON (fileScrape, outputPath);
        }
    }

    return EXIT_SUCCESS;
}
