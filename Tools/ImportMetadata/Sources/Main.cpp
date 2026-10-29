/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2022.  All rights reserved
 */
#include "Stroika/Frameworks/StroikaPreComp.h"

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
#include "Stroika/Foundation/IO/FileSystem/PathName.h"
#include "Stroika/Foundation/Time/DateTime.h"

#include "Digikam.h"
#include "ImageMetadataExtraction.h"
#include "LibIPAM/Metadata/Document.h"

using namespace std::filesystem;

using namespace Stroika::Foundation;
using namespace Stroika::Foundation::Characters;
using namespace Stroika::Foundation::Containers;
using namespace Stroika::Foundation::Execution;

namespace {
    constexpr bool kTallyExtensions  = true;
    constexpr bool kScrapeFileSystem = true; // false reuses the FileScrape.json an earlier run left in the output directory
    constexpr bool kCreateMasterFile = true;

    const path kDigikamScrapeFileName  = "DigikamScrape.json";
    const path kFileScrapeFileName     = "FileScrape.json";
    const path kMergedTagsFileName     = "DocumentMetaData.json";
    const path kExtensionTallyFileName = "ExtenstionTally.json";
    const path kProblemsFileName       = "Problems.json";

    const CommandLine::Option kSourceDirectoryO_{.fSingleCharName   = 's',
                                                 .fLongName         = "source-directory"sv,
                                                 .fSupportsArgument = true,
                                                 .fRequired         = true,
                                                 .fHelpArgName      = "DIR"sv,
                                                 .fHelpOptionText   = "the images to scrape: every file under DIR"sv};
    const CommandLine::Option kDigikamDatabaseO_{.fSingleCharName   = 'd',
                                                 .fLongName         = "digikam-database"sv,
                                                 .fSupportsArgument = true,
                                                 .fHelpArgName      = "FILE"sv,
                                                 .fHelpOptionText   = "a digiKam database (digikam4.db) to merge in"sv};
    const CommandLine::Option kIncludeDigikamOnlyO_{
        .fLongName = "include-digikam-only"sv,
        .fHelpOptionText = "also list the images only the digiKam database knows - for when the source directory is its whole collection"sv};
    const CommandLine::Option kOutputDirectoryO_{.fSingleCharName   = 'o',
                                                 .fLongName         = "output-directory"sv,
                                                 .fSupportsArgument = true,
                                                 .fRequired         = true,
                                                 .fHelpArgName      = "DIR"sv,
                                                 .fHelpOptionText = "where to write the JSON files, and a sample file of each extension"sv};
    const Sequence<CommandLine::Option> kAllOptions_{StandardCommandLineOptions::kHelp, kSourceDirectoryO_, kDigikamDatabaseO_,
                                                     kIncludeDigikamOnlyO_, kOutputDirectoryO_};

    // The same date and time of day, whatever zone either says: digiKam keeps a date as its local time, with no zone, so a
    // file's date that says its zone is digiKam's when its clock time is. A date that will not parse matches only itself.
    bool SameClockTime_ (const String& a, const String& b)
    {
        using Time::DateTime;
        optional<DateTime> da = DateTime::ParseQuietly (a, DateTime::kISO8601Format);
        optional<DateTime> db = DateTime::ParseQuietly (b, DateTime::kISO8601Format);
        if (da and db) {
            return da->GetDate () == db->GetDate () and da->GetTimeOfDay () == db->GetTimeOfDay ();
        }
        return a == b;
    }

    // How Sterling ran it while these paths were built in: one directory of the collection, or all of it
    constexpr string_view kExamples_ = "Examples:\n"
                                       "    -s P:/1900-1909 -d c:/Digikam/digikam4.db -o c:/ssw/mdResults/\n"
                                       "    -s P:/ -d c:/Digikam/digikam4.db --include-digikam-only -o c:/ssw/mdResults/";
}

// main's work, once its arguments are checked
static void Import_ (const CommandLine& commandLine)
{
    const path     sourceDirectory = commandLine.GetArgument (kSourceDirectoryO_)->As<path> ();
    const path     outputDirectory = commandLine.GetArgument (kOutputDirectoryO_)->As<path> ();
    optional<path> digikamDatabase;
    if (optional<String> arg = commandLine.GetArgument (kDigikamDatabaseO_)) {
        digikamDatabase = arg->As<path> ();
    }
    create_directories (outputDirectory);

    path digikamScrapeFilePath = outputDirectory / kDigikamScrapeFileName;
    path fileScrapeFilePath    = outputDirectory / kFileScrapeFileName;

    // each value left out of a Document, for Problems.json
    Sequence<Metadata::Problem> problems;
    Metadata::ProblemSink       problemSink = [&] (const Metadata::Problem& p) { problems.Append (p); };

    Mapping<String, Metadata::Document> fileScrape;
    if (kScrapeFileSystem) {
        {
            DbgTrace ("scraping file system directory at {}"_f, sourceDirectory);
            Debug::TimingTrace ttrc;
            fileScrape = Metadata::ImageMetadataExtractor{problemSink}.ExtractAll (sourceDirectory);
        }
        {
            DbgTrace ("writing file system scrape to {}"_f, fileScrapeFilePath);
            Debug::TimingTrace ttrc;
            Metadata::Document::WriteToFileAsJSON (fileScrape, fileScrapeFilePath);
        }
    }

    if (kTallyExtensions) {
        DbgTrace ("tallying extenstions for directory = {}"_f, sourceDirectory);
        Debug::TimingTrace ttrc;

        MultiSet<String> extTally = Metadata::ImageMetadataExtractor ().TallyExtensions (sourceDirectory, String{outputDirectory});

        DataExchange::ObjectVariantMapper mapper;
        mapper.AddCommonType<MultiSet<String>> ();
        mapper.AddCommonType<CountedValue<String>> ();

        path extenstionTallyPath = outputDirectory / kExtensionTallyFileName;

        DataExchange::Variant::JSON::Writer{}.Write (mapper.FromObject (extTally), IO::FileSystem::FileOutputStream::New (extenstionTallyPath));
    }

    Mapping<String, Metadata::Document> dbScrape;
    if (digikamDatabase) {
        {
            DbgTrace ("scraping digikam database at {}"_f, *digikamDatabase);
            Debug::TimingTrace ttrc;
            dbScrape = digikam::ScrapeDigikamDB (*digikamDatabase, problemSink);
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

        DbgTrace ("merging file and database sources, files length = {}, db length = {}"_f, fileScrape.Keys ().length (), dbScrape.Keys ().length ());
        // Where the file and digiKam differ, digiKam's value wins: it is what the user set there, which reaches the file only
        // if digiKam writes it back - never into a RAW or HEIC file, whose edits go to a sidecar. Where they agree, the file's
        // is kept: its date may say its zone, as digiKam's never does. Tags are both sets.
        Mapping<String, Metadata::Document> masterList;
        for (const auto& it : fileScrape) {
            Metadata::Document dmd = it.fValue;
            Metadata::Document digikamDmd;
            if (dbScrape.Lookup (it.fKey, &digikamDmd)) {
                dmd.album = digikamDmd.album; // digikam does better here at capture correct top of collection when you don't do full file scan
                if (digikamDmd.comment and digikamDmd.comment != dmd.comment) {
                    if (dmd.comment) {
                        DbgTrace ("COMMENT disagreement for {} ({} vs digiKam's {})"_f, it.fKey, Metadata::Document::Comment::ToString (*dmd.comment),
                                  Metadata::Document::Comment::ToString (*digikamDmd.comment));
                    }
                    dmd.comment = digikamDmd.comment;
                }
                if (digikamDmd.date and not(dmd.date and SameClockTime_ (*dmd.date, *digikamDmd.date))) {
                    if (dmd.date) {
                        DbgTrace ("DATE disagreement for {} ({} vs digiKam's {})"_f, it.fKey, dmd.date, digikamDmd.date);
                    }
                    dmd.date = digikamDmd.date;
                }
                if (digikamDmd.location and digikamDmd.location != dmd.location) {
                    if (dmd.location) {
                        DbgTrace ("LOCATION disagreement for {} ({} vs digiKam's {})"_f, it.fKey, dmd.location, digikamDmd.location);
                    }
                    dmd.location = digikamDmd.location;
                }
                if (digikamDmd.rating and digikamDmd.rating != dmd.rating) {
                    if (dmd.rating) {
                        DbgTrace ("RATING disagreement for {} ({} vs digiKam's {})"_f, it.fKey, dmd.rating, digikamDmd.rating);
                    }
                    dmd.rating = digikamDmd.rating;
                }
                for (const String& tag : dmd.tags) {
                    if (not digikamDmd.tags.Contains (tag)) {
                        DbgTrace ("tag missing from digiKam {} : {}"_f, it.fKey, tag);
                    }
                }
                for (const String& tag : digikamDmd.tags) {
                    if (not dmd.tags.Contains (tag)) {
                        DbgTrace ("tag missing from file scrape {} : {}"_f, it.fKey, tag);
                        dmd.tags.Add (tag);
                    }
                }
            }
            else if (digikamDatabase) {
                DbgTrace ("missing digikam dmd for : {}"_f, it.fKey);
            }
            masterList.Add (it.fKey, dmd);
        }
        if (commandLine.Has (kIncludeDigikamOnlyO_)) {
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
            auto outputPath = outputDirectory / kMergedTagsFileName;
            DbgTrace ("writing processed tag info to {}"_f, outputPath);
            Debug::TimingTrace ttrc;
            Metadata::Document::WriteToFileAsJSON (masterList, outputPath);
        }
    }

    {
        path problemsFilePath = outputDirectory / kProblemsFileName;
        DbgTrace ("writing {} problems to {}"_f, problems.size (), problemsFilePath);
        DataExchange::ObjectVariantMapper mapper;
        mapper.AddClass<Metadata::Problem> ({
            {"file"sv, &Metadata::Problem::fFile},
            {"key"sv, &Metadata::Problem::fKey},
            {"value"sv, &Metadata::Problem::fValue},
            {"reason"sv, &Metadata::Problem::fReason},
        });
        mapper.AddCommonType<Sequence<Metadata::Problem>> ();
        DataExchange::Variant::JSON::Writer{}.Write (mapper.FromObject (problems), IO::FileSystem::FileOutputStream::New (problemsFilePath));
        if (not problems.empty ()) {
            cerr << "{} values could not be read, and were left out - see {}"_f(problems.size (), problemsFilePath) << endl;
        }
    }
}

int main (int argc, const char* argv[])
{
    CommandLine               commandLine{argc, argv};
    Debug::TraceContextBumper ctx{"main", "argv={}"_f, commandLine};

    if (commandLine.Has (StandardCommandLineOptions::kHelp)) {
        cerr << commandLine.GenerateUsage (kAllOptions_) << endl;
        cerr << kExamples_ << endl;
        return EXIT_SUCCESS;
    }
    try {
        commandLine.Validate (kAllOptions_);
    }
    catch (...) {
        cerr << Characters::ToString (current_exception ()) << endl;
        cerr << commandLine.GenerateUsage (kAllOptions_) << endl;
        return EXIT_FAILURE;
    }

    try {
        Import_ (commandLine);
    }
    catch (...) {
        DbgTrace ("got exception={}"_f, current_exception ());
        cerr << Characters::ToString (current_exception ()) << endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
