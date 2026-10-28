/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2026.  All rights reserved
 */

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>

#include "Stroika/Foundation/Characters/StringBuilder.h"
#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/Containers/SortedSet.h"
#include "Stroika/Foundation/Database/SQL/SQLite.h"
#include "Stroika/Foundation/Debug/Trace.h"
#include "Stroika/Foundation/Execution/CommandLine.h"
#include "Stroika/Foundation/Execution/Module.h"
#include "Stroika/Foundation/Execution/ProcessRunner.h"
#include "Stroika/Foundation/IO/FileSystem/TemporaryFile.h"
#include "Stroika/Foundation/Time/DateTime.h"

#include "Stroika/Frameworks/Test/TestHarness.h"

#include "LibIPAM/Common/Geolocation.h"
#include "LibIPAM/Metadata/Document.h"

DISABLE_COMPILER_MSC_WARNING_START (4127)
#include "exiv2/exiv2.hpp"
DISABLE_COMPILER_MSC_WARNING_END (4127)

using std::filesystem::path;

using namespace Stroika::Foundation;
using namespace Stroika::Foundation::Characters;
using namespace Stroika::Foundation::Containers;
using namespace Stroika::Frameworks;

using namespace IPAM::LibIPAM::Common;
using namespace IPAM::LibIPAM::Metadata;

#if qHasFeature_GoogleTest
namespace {
    // ImportMetadata, built beside this test: Builds/<CONFIG>/IPAM/Tools/ImportMetadata/, and Tests/ImportMetadataTool/
    path ImportMetadataEXE_ ()
    {
        path thisEXE = Execution::GetEXEPath ();
        return thisEXE.parent_path ().parent_path ().parent_path () / "Tools" / "ImportMetadata" / ("ImportMetadata" + thisEXE.extension ().string ());
    }

    // A JPEG (Exiv2's blank one) with the metadata addMetadata gives it
    void WriteJPEG_ (const path& p, const function<void (Exiv2::Image&)>& addMetadata)
    {
        create_directories (p.parent_path ());
        Exiv2::Image::UniquePtr image = Exiv2::ImageFactory::create (Exiv2::ImageType::jpeg, p.string ());
        addMetadata (*image);
        image->writeMetadata ();
    }
    void AddIPTCKeywords_ (Exiv2::Image& image, initializer_list<string> keywords)
    {
        for (const string& k : keywords) {
            Exiv2::StringValue v{k};
            image.iptcData ().add (Exiv2::IptcKey{"Iptc.Application2.Keywords"}, &v);
        }
    }
    // A face region with a person's name, as digiKam writes one: an XMP struct, in a bag, in a struct
    void AddXMPRegionName_ (Exiv2::Image& image, const string& name)
    {
        Exiv2::XmpTextValue aStruct;
        aStruct.setXmpStruct ();
        Exiv2::XmpTextValue aBag;
        aBag.setXmpArrayType (Exiv2::XmpValue::xaBag);
        image.xmpData ().add (Exiv2::XmpKey{"Xmp.mwg-rs.Regions"}, &aStruct);
        image.xmpData ().add (Exiv2::XmpKey{"Xmp.mwg-rs.Regions/mwg-rs:RegionList"}, &aBag);
        image.xmpData ().add (Exiv2::XmpKey{"Xmp.mwg-rs.Regions/mwg-rs:RegionList[1]"}, &aStruct);
        image.xmpData ()["Xmp.mwg-rs.Regions/mwg-rs:RegionList[1]/mwg-rs:Name"] = name;
    }

    // The tables and columns of a digiKam database that ImportMetadata reads. digiKam keeps dates in local time, with no
    // offset, and an album's path relative to its album root.
    void WriteDigikamDB_ (const path& dbFile, const path& albumRoot)
    {
        using namespace Database::SQL;
        auto conn = SQLite::Connection::New (SQLite::Connection::Options{.fDBPath = dbFile});
        conn.Exec ("CREATE TABLE AlbumRoots (id INTEGER PRIMARY KEY, identifier TEXT, specificPath TEXT);"
                   "CREATE TABLE Albums (id INTEGER PRIMARY KEY, albumRoot INTEGER NOT NULL, relativePath TEXT NOT NULL);"
                   "CREATE TABLE Images (id INTEGER PRIMARY KEY, album INTEGER, name TEXT NOT NULL);"
                   "CREATE TABLE ImageComments (id INTEGER PRIMARY KEY, imageid INTEGER, author TEXT, comment TEXT);"
                   "CREATE TABLE ImagePositions (imageid INTEGER PRIMARY KEY, latitudeNumber REAL, longitudeNumber REAL);"
                   "CREATE TABLE ImageInformation (imageid INTEGER PRIMARY KEY, rating INTEGER, creationDate DATETIME);"
                   "CREATE TABLE Tags (id INTEGER PRIMARY KEY, name TEXT NOT NULL);"
                   "CREATE TABLE TagProperties (tagid INTEGER, property TEXT, value TEXT);"
                   "CREATE TABLE ImageTags (imageid INTEGER NOT NULL, tagid INTEGER NOT NULL);"sv);
        conn.Exec ("INSERT INTO AlbumRoots VALUES (1, 'volumeid:?path={}', '/');"_f(String{albumRoot.generic_wstring ()}.ReplaceAll ("'"sv, "''"sv)));
        conn.Exec ("INSERT INTO Albums VALUES (1, 1, '/1964 New York');"
                   "INSERT INTO Images VALUES (1, 1, 'IMG_0001.jpg'), (2, 1, 'IMG_0002.jpg'), (3, 1, 'IMG_0003.jpg');"
                   "INSERT INTO ImageComments VALUES (1, 1, NULL, 'Fred took this one'), (2, 2, 'Sterling', 'Fred took this one'),"
                   "    (3, 3, NULL, 'Screenshot');"
                   "INSERT INTO ImagePositions VALUES (2, 40.6894, -74.0447);"
                   "INSERT INTO ImageInformation VALUES (1, 4, '1964-07-04T15:30:00.000'), (2, 1, '1964-07-05T10:00:00.000'),"
                   "    (3, 5, '1964-07-06T09:00:00.000');"
                   "INSERT INTO Tags VALUES (1, 'Fred'), (2, 'Statue of Liberty'), (3, 'Uncle Bob'), (4, 'Color Label Red');"
                   "INSERT INTO TagProperties VALUES (1, 'person', 'Fred'), (3, 'faceEngineId', '7'), (4, 'internalTag', NULL);"
                   "INSERT INTO ImageTags VALUES (1, 1), (1, 2), (1, 3), (1, 4), (2, 1), (3, 1);"sv);
    }

    // A small digiKam collection - photos, and its database - in a temporary directory. Its database knows IMG_0001 to 0003
    // (0003 has no file); Scans is not in it, and nor is anything ImportMetadata skips (a hidden directory, a text file).
    struct Collection_ {
        IO::FileSystem::ScopedTmpDir tmpDir{"ImportMetadataTest"};
        path                         root{static_cast<path> (tmpDir)};
        path                         images{root / "Images"};
        path                         database{root / "digikam4.db"};

        Collection_ ()
        {
            WriteJPEG_ (images / "1964 New York" / "IMG_0001.jpg", [] (Exiv2::Image& image) {
                AddIPTCKeywords_ (image, {"Fred", "Statue of Liberty", "Brooklyn Bridge", "Ignored"});
                image.iptcData ()["Iptc.Application2.ObjectName"]  = "Trip to New York";
                image.iptcData ()["Iptc.Application2.Caption"]     = "Our first trip to New York";
                image.iptcData ()["Iptc.Application2.DateCreated"] = "1964-07-04";
                image.iptcData ()["Iptc.Application2.TimeCreated"] = "14:30:00-04:00";
                image.xmpData ()["Xmp.MicrosoftPhoto.Rating"]      = "75";
                image.xmpData ()["Xmp.exif.GPSLatitude"]           = "40,41.364N";
                image.xmpData ()["Xmp.exif.GPSLongitude"]          = "74,2.682W";
            });
            WriteJPEG_ (images / "1964 New York" / "IMG_0002.jpg", [] (Exiv2::Image& image) { AddIPTCKeywords_ (image, {"Fred"}); });
            WriteJPEG_ (images / "Scans" / "IMG_0100.jpg", [] (Exiv2::Image& image) {
                image.xmpData ()["Xmp.xmp.CreateDate"]        = "1931-05-01T12:00:00+01:00";
                image.xmpData ()["Xmp.acdsee.caption"]        = "Grandma's wedding";
                image.xmpData ()["Xmp.acdsee.notes"]          = "Found in the attic";
                image.xmpData ()["Xmp.acdsee.author"]         = "Sterling";
                image.xmpData ()["Xmp.MicrosoftPhoto.Rating"] = "99";
                AddXMPRegionName_ (image, "Grandma");
            });
            WriteJPEG_ (images / "Scans" / "unknown.jpg", [] (Exiv2::Image&) {});
            WriteJPEG_ (images / "loose.jpg", [] (Exiv2::Image&) {});
            WriteJPEG_ (images / ".hidden" / "IMG_9999.jpg", [] (Exiv2::Image& image) { AddIPTCKeywords_ (image, {"Hidden"}); });
            std::ofstream{images / "Scans" / "notes.txt"} << "not a photo" << endl;
            WriteDigikamDB_ (database, images);
        }
    };

    // What ImportMetadata should write for Collection_
    Mapping<String, Document> ExpectedDocuments_ (const Collection_& c, bool includeDigikamOnly)
    {
        auto key = [&] (const path& p) { return String{(c.images / p).generic_wstring ()}; };
        // digiKam's dates are local time, so in UTC they depend on this machine's time zone
        auto fromDigikamDate = [] (const String& d) {
            return Time::DateTime::Parse (d, Time::DateTime::kISO8601Format).AsUTC ().Format (Time::DateTime::kISO8601Format);
        };
        Mapping<String, Document> result;
        {
            // in both: the file's metadata wins, and digiKam adds its album and its tags (but not internal ones)
            Document d;
            d.tags     = Set<String>{"Fred"sv, "Statue of Liberty"sv, "Brooklyn Bridge"sv, "Uncle Bob"sv};
            d.date     = "1964-07-04T18:30:00Z"sv;
            d.location = Geolocation{40 + 41.364 / 60, -(74 + 2.682 / 60)}.ToISOString ();
            d.comment  = Sequence<Document::Comment>{Document::Comment{"Our first trip to New York"sv}};
            d.title    = "Trip to New York"sv;
            d.rating   = 0.75;
            d.album    = "1964 New York"sv;
            result.Add (key ("1964 New York/IMG_0001.jpg"), d);
        }
        {
            // digiKam fills in what the file lacks
            Document d;
            d.tags     = Set<String>{"Fred"sv};
            d.date     = fromDigikamDate ("1964-07-05T10:00:00.000"sv);
            d.location = Geolocation{40.6894, -74.0447}.ToISOString ();
            d.comment  = Sequence<Document::Comment>{Document::Comment{"Fred took this one"sv, "Sterling"sv}};
            d.rating   = 0.01;
            d.album    = "1964 New York"sv;
            result.Add (key ("1964 New York/IMG_0002.jpg"), d);
        }
        {
            // the file alone
            Document d;
            d.tags    = Set<String>{"Grandma"sv};
            d.date    = "1931-05-01T11:00:00Z"sv;
            d.comment = Sequence<Document::Comment>{Document::Comment{"Found in the attic"sv, "Sterling"sv}};
            d.title   = "Grandma's wedding"sv;
            d.rating  = 1.0;
            d.album   = "Scans"sv;
            result.Add (key ("Scans/IMG_0100.jpg"), d);
        }
        {
            Document d;
            d.album = "Scans"sv;
            result.Add (key ("Scans/unknown.jpg"), d);
        }
        {
            Document d;
            d.album = ""sv;
            result.Add (key ("loose.jpg"), d);
        }
        if (includeDigikamOnly) {
            // digiKam alone (and its comment is one ImportMetadata ignores)
            Document d;
            d.tags   = Set<String>{"Fred"sv};
            d.date   = fromDigikamDate ("1964-07-06T09:00:00.000"sv);
            d.rating = 1.0;
            d.album  = "1964 New York"sv;
            result.Add (key ("1964 New York/IMG_0003.jpg"), d);
        }
        return result;
    }

    // Runs ImportMetadata on the collection (-s naming its images, as given); returns the DocumentMetaData.json it wrote
    Mapping<String, Document> RunImportMetadata_ (const Collection_& c, const String& images, const path& outputDirectory, bool includeDigikamOnly)
    {
        Sequence<String> args{String{ImportMetadataEXE_ ()}, "-s"sv, images, "-d"sv, String{c.database}, "-o"sv, String{outputDirectory}};
        if (includeDigikamOnly) {
            args.Append ("--include-digikam-only"sv);
        }
        // with a time limit, as a Windows Debug build that aborts shows a dialog, and waits for someone to click it
        Execution::ProcessRunner{Execution::CommandLine{args}}.Run (""sv, {}, Time::DurationSeconds{5 * 60});
        Mapping<String, Document> result;
        Document::ReadFromJSONFile (&result, outputDirectory / "DocumentMetaData.json");
        return result;
    }

    // Document by document, so a failure names the one that differs (and EXPECT_EQ on whole Mappings has gtest print them as
    // raw bytes - they have no ToString () member - which aborts under ASan with MSVC)
    void ExpectSameDocuments_ (const Mapping<String, Document>& actual, const Mapping<String, Document>& expected)
    {
        EXPECT_EQ (actual.size (), expected.size ());
        for (const auto& kv : expected) {
            EXPECT_EQ (actual.Lookup (kv.fKey), kv.fValue) << kv.fKey;
        }
    }

    // The names in dir, sorted
    String Listing_ (const path& dir)
    {
        SortedSet<String> names;
        for (const auto& e : std::filesystem::directory_iterator{dir}) {
            names.Add (String{e.path ().filename ().wstring ()});
        }
        StringBuilder sb;
        for (const String& n : names) {
            sb << (sb.empty () ? ""sv : " "sv) << n;
        }
        return sb;
    }

    GTEST_TEST (ImportMetadata, MergesFilesWithDigikam_)
    {
        Collection_ c;
        path        out = c.root / "out";
        ExpectSameDocuments_ (RunImportMetadata_ (c, String{c.images}, out, false), ExpectedDocuments_ (c, false));
        // it writes only into its output directory
        EXPECT_EQ (Listing_ (c.root), "Images digikam4.db out"sv);
        EXPECT_EQ (Listing_ (out), "DigikamScrape.json DocumentMetaData.json ExtenstionTally.json FileScrape.json jpg.txt"sv);
    }

    GTEST_TEST (ImportMetadata, IncludeDigikamOnly_)
    {
        Collection_ c;
        // and -s with a trailing separator makes no difference
        String images = String{c.images / ""};
        ExpectSameDocuments_ (RunImportMetadata_ (c, images, c.root / "out", true), ExpectedDocuments_ (c, true));
    }
}
#endif

int main (int argc, const char* argv[])
{
    Debug::TraceContextBumper ctx{"main", "argv={}"_f, Execution::CommandLine{argc, argv}};
    Test::Setup (argc, argv);
#if qHasFeature_GoogleTest
    return RUN_ALL_TESTS ();
#else
    cerr << "IPAM regression tests require building with google test feature" << endl;
#endif
}
