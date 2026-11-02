/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2026.  All rights reserved
 */

#include "Stroika/Frameworks/StroikaPreComp.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>

#if qStroika_Platform_Windows
#include <Windows.h>
#endif

#include "Stroika/Foundation/Characters/StringBuilder.h"
#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/Containers/SortedSet.h"
#include "Stroika/Foundation/DataExchange/Variant/JSON/Reader.h"
#include "Stroika/Foundation/Database/SQL/SQLite.h"
#include "Stroika/Foundation/Debug/Trace.h"
#include "Stroika/Foundation/Execution/CommandLine.h"
#include "Stroika/Foundation/Execution/Module.h"
#include "Stroika/Foundation/Execution/ProcessRunner.h"
#include "Stroika/Foundation/IO/FileSystem/FileInputStream.h"
#include "Stroika/Foundation/IO/FileSystem/FileSystem.h"
#include "Stroika/Foundation/IO/FileSystem/TemporaryFile.h"
#include "Stroika/Foundation/Time/DateTime.h"

#include "Stroika/Frameworks/Test/TestHarness.h"

#include "LibIPAM/Common/Geolocation.h"
#include "LibIPAM/Metadata/Document.h"

DISABLE_COMPILER_MSC_WARNING_START (4127)
#include "exiv2/exiv2.hpp"
DISABLE_COMPILER_MSC_WARNING_END (4127)

using std::byte;
using std::filesystem::path;
using std::filesystem::perms;

using namespace Stroika::Foundation;
using namespace Stroika::Foundation::Characters;
using namespace Stroika::Foundation::Containers;
using Stroika::Foundation::DataExchange::VariantValue;
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

    // A JPEG (Exiv2's blank one) with the metadata addMetadata gives it - named, as ImportMetadata names a photo to Exiv2, in UTF-8
    void WriteJPEG_ (const path& p, const function<void (Exiv2::Image&)>& addMetadata)
    {
        create_directories (p.parent_path ());
        u8string                name  = p.u8string ();
        Exiv2::Image::UniquePtr image = Exiv2::ImageFactory::create (Exiv2::ImageType::jpeg, string{name.begin (), name.end ()});
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

    // EXIF's date taken, as a camera writes it: local time, in EXIF's format - and its offset from UTC, if the camera knows it
    void AddEXIFDate_ (Exiv2::Image& image, const string& dateTimeOriginal, const optional<string>& offsetTimeOriginal = nullopt)
    {
        image.exifData ()["Exif.Photo.DateTimeOriginal"] = dateTimeOriginal;
        if (offsetTimeOriginal) {
            image.exifData ()["Exif.Photo.OffsetTimeOriginal"] = *offsetTimeOriginal;
        }
    }
    // EXIF's GPS coordinates, as a camera writes them: degrees, minutes and seconds (each a rational), and N/S or E/W (none if empty)
    void AddEXIFGPS_ (Exiv2::Image& image, const string& latitude, const string& latitudeRef, const string& longitude, const string& longitudeRef)
    {
        image.exifData ()["Exif.GPSInfo.GPSLatitude"]  = latitude;
        image.exifData ()["Exif.GPSInfo.GPSLongitude"] = longitude;
        if (not latitudeRef.empty ()) {
            image.exifData ()["Exif.GPSInfo.GPSLatitudeRef"] = latitudeRef;
        }
        if (not longitudeRef.empty ()) {
            image.exifData ()["Exif.GPSInfo.GPSLongitudeRef"] = longitudeRef;
        }
    }

    // The tables and columns of a digiKam database that ImportMetadata reads. digiKam keeps dates in local time, with no
    // offset (so ImportMetadata writes them with none), and an album's path relative to its album root - "/" for the album
    // root itself.
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
        conn.Exec ("INSERT INTO Albums VALUES (1, 1, '/1964 New York'), (2, 1, '/');"
                   "INSERT INTO Images VALUES (1, 1, 'IMG_0001.jpg'), (2, 1, 'IMG_0002.jpg'), (3, 1, 'IMG_0003.jpg'), (4, 2, 'loose.jpg'),"
                   "    (5, 2, 'IMG_0004.jpg'), (6, 1, 'IMG_0005.heic'), (7, 1, 'IMG_0006.jpg');"
                   "INSERT INTO ImageComments VALUES (1, 1, NULL, 'Fred took this one'), (2, 2, 'Sterling', 'Fred took this one'),"
                   "    (3, 3, NULL, 'Screenshot');"
                   "INSERT INTO ImagePositions VALUES (1, 40.7484, -73.9857), (2, 40.6894, -74.0447), (3, 40.7, NULL), (4, NULL, NULL);"
                   "INSERT INTO ImageInformation VALUES (1, 5, '1964-07-04T14:30:00.000'), (2, 1, '1964-07-05T10:00:00.000'),"
                   "    (3, 5, '1964-07-06T09:00:00.000'), (4, 3, '1964-07-07T08:00:00.000'), (5, 2, 'yesterday'), (6, 7, NULL);"
                   "INSERT INTO Tags VALUES (1, 'Fred'), (2, 'Statue of Liberty'), (3, 'Uncle Bob'), (4, 'Color Label Red');"
                   "INSERT INTO TagProperties VALUES (1, 'person', 'Fred'), (3, 'faceEngineId', '7'), (4, 'internalTag', NULL);"
                   "INSERT INTO ImageTags VALUES (1, 1), (1, 2), (1, 3), (1, 4), (2, 1), (3, 1), (4, 3), (5, 1), (6, 1);"sv);
        // digiKam's guess for IMG_0006, which has no date of its own: its file's time, as local time with no zone (digiKam
        // takes the earlier of when it was created and last modified - here last modified, which ImportMetadata knows too)
        using Time::DateTime;
        DateTime fileTime = IO::FileSystem::Default ().GetFileLastModificationDate (albumRoot / "1964 New York" / "IMG_0006.jpg").AsLocalTime ();
        conn.Exec ("INSERT INTO ImageInformation VALUES (7, 3, '{}');"_f(
            DateTime{fileTime.GetDate (), fileTime.GetTimeOfDay ()}.Format (DateTime::kISO8601Format)));
    }

    // A small digiKam collection - photos, and its database - in a temporary directory. Its database knows IMG_0001 to 0003,
    // IMG_0005.heic and IMG_0006 in its album 1964 New York, and loose.jpg and IMG_0004 in its root album (0003 and 0004 have no file);
    // Scans is not in it, and nor is anything ImportMetadata skips (a hidden directory, a text file) or cannot read (damaged
    // photos).
    struct Collection_ {
        IO::FileSystem::ScopedTmpDir tmpDir{"ImportMetadataTest"};
        path                         root{static_cast<path> (tmpDir)};
        path                         images;
        path                         database{root / "digikam4.db"};

        // its photos in root / imagesName
        Collection_ (const path& imagesName = "Images")
            : images{root / imagesName}
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
                // what the camera recorded, which the IPTC and XMP above (set since) win over
                AddEXIFDate_ (image, "1999:01:01 00:00:00");
                AddEXIFGPS_ (image, "10/1 0/1 0/1", "N", "20/1 0/1 0/1", "E");
            });
            // the camera's date, from a clock never set - which the user corrected in digiKam
            WriteJPEG_ (images / "1964 New York" / "IMG_0002.jpg", [] (Exiv2::Image& image) {
                AddIPTCKeywords_ (image, {"Fred"});
                AddEXIFDate_ (image, "1999:01:01 00:00:00");
            });
            // a phone's HEIC photo (a JPEG by that name: the merge goes by the name), whose tags digiKam keeps in a sidecar
            WriteJPEG_ (images / "1964 New York" / "IMG_0005.heic", [] (Exiv2::Image&) {});
            // no date of its own, so digiKam's is a guess (WriteDigikamDB_)
            WriteJPEG_ (images / "1964 New York" / "IMG_0006.jpg", [] (Exiv2::Image&) {});
            WriteJPEG_ (images / "Scans" / "IMG_0100.jpg", [] (Exiv2::Image& image) {
                image.xmpData ()["Xmp.xmp.CreateDate"]        = "1931-05-01T12:00:00+01:00";
                image.xmpData ()["Xmp.acdsee.caption"]        = "Grandma's wedding";
                image.xmpData ()["Xmp.acdsee.notes"]          = "Found in the attic";
                image.xmpData ()["Xmp.acdsee.author"]         = "Sterling";
                image.xmpData ()["Xmp.MicrosoftPhoto.Rating"] = "99";
                image.xmpData ()["Xmp.exif.GPSLatitude"]      = "48,51,29.6N";
                image.xmpData ()["Xmp.exif.GPSLongitude"]     = "2,17,40.2E";
                AddXMPRegionName_ (image, "Grandma");
                // the standard fields, which ACDSee's and MicrosoftPhoto's above win over
                image.xmpData ()["Xmp.dc.title"]       = "lang=\"x-default\" Wedding";
                image.xmpData ()["Xmp.dc.description"] = "lang=\"x-default\" Attic find";
                image.xmpData ()["Xmp.xmp.Rating"]     = "2";
            });
            WriteJPEG_ (images / "Scans" / "IMG_0101.jpg", [] (Exiv2::Image& image) {
                AddIPTCKeywords_ (image, {"Grandma"});
                image.xmpData ()["Xmp.exif.GPSLatitude"]      = "somewhere";
                image.xmpData ()["Xmp.exif.GPSLongitude"]     = "2,17,40.2E";
                image.xmpData ()["Xmp.xmp.CreateDate"]        = "last summer";
                image.xmpData ()["Xmp.MicrosoftPhoto.Rating"] = "five";
                image.xmpData ()["Xmp.xmp.Rating"]            = "lots";
                AddEXIFDate_ (image, "someday");
                AddEXIFGPS_ (image, "48/1 51/1 2960/100", "", "2/1 17/1 402/10", "E");
                image.exifData ()["Exif.Image.Rating"] = uint16_t{9};
            });
            // only EXIF: a date with no zone (the camera's local time), and one with its zone
            WriteJPEG_ (images / "Scans" / "IMG_0102.jpg", [] (Exiv2::Image& image) {
                AddEXIFDate_ (image, "1950:12:25 09:15:00");
                AddEXIFGPS_ (image, "51/1 30/1 2604/100", "N", "0/1 7/1 3954/100", "W");
            });
            WriteJPEG_ (images / "Scans" / "IMG_0103.jpg",
                        [] (Exiv2::Image& image) { AddEXIFDate_ (image, "2019:06:01 18:45:30", "+02:00"); });
            // only EXIF: a rating in stars, and a description
            WriteJPEG_ (images / "Scans" / "IMG_0104.jpg", [] (Exiv2::Image& image) {
                image.exifData ()["Exif.Image.Rating"]           = uint16_t{4};
                image.exifData ()["Exif.Image.ImageDescription"] = "Grandpa's farm";
            });
            // as a camera and Windows write them: a rating as both a percentage and stars, and a description that is the
            // camera's boilerplate
            WriteJPEG_ (images / "Scans" / "IMG_0105.jpg", [] (Exiv2::Image& image) {
                image.exifData ()["Exif.Image.Make"]             = "OLYMPUS IMAGING CORP.";
                image.exifData ()["Exif.Image.ImageDescription"] = "OLYMPUS DIGITAL CAMERA         ";
                image.exifData ()["Exif.Image.RatingPercent"]    = uint16_t{50};
                image.exifData ()["Exif.Image.Rating"]           = uint16_t{1};
            });
            // a rating of 0 - unrated - and a description that is the camera's model
            WriteJPEG_ (images / "Scans" / "IMG_0106.jpg", [] (Exiv2::Image& image) {
                image.xmpData ()["Xmp.MicrosoftPhoto.Rating"]    = "0";
                image.exifData ()["Exif.Image.Model"]            = "Canon EOS 5D";
                image.exifData ()["Exif.Image.ImageDescription"] = "Canon EOS 5D";
            });
            // only XMP's standard fields, as Lightroom writes them: a title and description in the default language, keywords,
            // and a rating in stars
            WriteJPEG_ (images / "Scans" / "IMG_0107.jpg", [] (Exiv2::Image& image) {
                image.xmpData ()["Xmp.dc.title"]       = "lang=\"x-default\" Grandpa's farm";
                image.xmpData ()["Xmp.dc.description"] = "lang=\"x-default\" From the hayloft";
                Exiv2::XmpArrayValue keywords{Exiv2::TypeId::xmpBag};
                keywords.read ("Grandpa");
                keywords.read ("Farm");
                image.xmpData ().add (Exiv2::XmpKey{"Xmp.dc.subject"}, &keywords);
                image.xmpData ()["Xmp.xmp.Rating"] = "3";
            });
            // text that is not ASCII (as UTF-8 bytes, or Windows-1252's): XMP's, always UTF-8
            WriteJPEG_ (images / "Scans" / "IMG_0108.jpg", [] (Exiv2::Image& image) {
                image.xmpData ()["Xmp.acdsee.caption"] = "Zo\xc3\xab's party";
                image.xmpData ()["Xmp.acdsee.notes"]   = "Caf\xc3\xa9 au lait";
                AddXMPRegionName_ (image, "Zo\xc3\xab");
            });
            // IPTC's naming UTF-8 as its character set, as digiKam writes it; and EXIF's in UTF-8
            WriteJPEG_ (images / "Scans" / "IMG_0109.jpg", [] (Exiv2::Image& image) {
                image.iptcData ()["Iptc.Envelope.CharacterSet"] = "\x1b%G";
                AddIPTCKeywords_ (image, {"M\xc3\xbcller"});
                image.iptcData ()["Iptc.Application2.ObjectName"] = "F\xc3\xaate";
                image.exifData ()["Exif.Image.ImageDescription"]  = "Grand-m\xc3\xa8re";
            });
            // IPTC's naming no character set, and EXIF's, in Windows-1252, as older Windows programs wrote them
            WriteJPEG_ (images / "Scans" / "IMG_0110.jpg", [] (Exiv2::Image& image) {
                AddIPTCKeywords_ (image, {"Jos\xe9"});
                image.iptcData ()["Iptc.Application2.ObjectName"] = "\x93Uncle\x94 Bob";
                image.exifData ()["Exif.Image.ImageDescription"]  = "Caf\xe9";
            });
            // a name outside Windows' code page (Windows-1252 here), which Exiv2 0.28 opens only with ThirdPartyComponents/Exiv2's
            // patch: a newer Exiv2 fails here if it has no fix of its own
            WriteJPEG_ (images / "Scans" / u8"\u03a9mega.jpg", [] (Exiv2::Image& image) { AddIPTCKeywords_ (image, {"Omega"}); });
            // a camera whose clock was never set
            WriteJPEG_ (images / "Scans" / "unknown.jpg", [] (Exiv2::Image& image) { AddEXIFDate_ (image, "0000:00:00 00:00:00"); });
            WriteJPEG_ (images / "loose.jpg", [] (Exiv2::Image&) {});
            WriteJPEG_ (images / ".hidden" / "IMG_9999.jpg", [] (Exiv2::Image& image) { AddIPTCKeywords_ (image, {"Hidden"}); });
            std::ofstream{images / "Scans" / "notes.txt"} << "not a photo" << endl;
            // damaged photos: a JPEG's first bytes, then no JPEG; and a raw photo's name on nothing Exiv2 recognizes
            std::ofstream{images / "Scans" / "broken.jpg", ios::binary} << "\xFF\xD8"sv << "then not a JPEG"sv;
            std::ofstream{images / "Scans" / "zeros.nef", ios::binary} << string (64, '\0');
            WriteDigikamDB_ (database, images);
        }
    };

    // What ImportMetadata should write for Collection_
    Mapping<String, Document> ExpectedDocuments_ (const Collection_& c, bool includeDigikamOnly)
    {
        auto                      key = [&] (const path& p) { return String{(c.images / p).generic_wstring ()}; };
        Mapping<String, Document> result;
        {
            // in both: where they differ, digiKam's value wins - the user set it there (comment, location, rating); its date is
            // the file's clock time, so the file's, which says its zone, is kept; and digiKam adds its album and its tags (but
            // not internal ones)
            Document d;
            d.tags     = Set<String>{"Fred"sv, "Statue of Liberty"sv, "Brooklyn Bridge"sv, "Uncle Bob"sv};
            d.date     = "1964-07-04T14:30:00-04:00"sv;
            d.location = Geolocation{40.7484, -73.9857}.ToISOString ();
            d.comment  = Sequence<Document::Comment>{Document::Comment{"Fred took this one"sv}};
            d.title    = "Trip to New York"sv;
            d.rating   = 1.0;
            d.album    = "1964 New York"sv;
            result.Add (key ("1964 New York/IMG_0001.jpg"), d);
        }
        {
            // digiKam fills in what the file lacks, and its date - the user's - wins over the camera's
            Document d;
            d.tags     = Set<String>{"Fred"sv};
            d.date     = "1964-07-05T10:00:00"sv;
            d.location = Geolocation{40.6894, -74.0447}.ToISOString ();
            d.comment  = Sequence<Document::Comment>{Document::Comment{"Fred took this one"sv, "Sterling"sv}};
            d.rating   = 0.01;
            d.album    = "1964 New York"sv;
            result.Add (key ("1964 New York/IMG_0002.jpg"), d);
        }
        {
            // digiKam's tags, though the photo is a HEIC (and its date, none - NULL - is no date; its rating, 7 stars, is left
            // out - kProblems_)
            Document d;
            d.tags  = Set<String>{"Fred"sv};
            d.album = "1964 New York"sv;
            result.Add (key ("1964 New York/IMG_0005.heic"), d);
        }
        {
            // digiKam's date is its guess - the file's time - not one the user set, so none; the rest of digiKam's stays
            Document d;
            d.rating = 0.50;
            d.album  = "1964 New York"sv;
            result.Add (key ("1964 New York/IMG_0006.jpg"), d);
        }
        {
            // the file alone (its location in XMP's other form, DDD,MM,SSk; ACDSee's title and comment, and MicrosoftPhoto's
            // rating, over the standard fields')
            Document d;
            d.tags     = Set<String>{"Grandma"sv};
            d.date     = "1931-05-01T12:00:00+01:00"sv;
            d.location = Geolocation{48 + 51 / 60.0 + 29.6 / 3600, 2 + 17 / 60.0 + 40.2 / 3600}.ToISOString ();
            d.comment  = Sequence<Document::Comment>{Document::Comment{"Found in the attic"sv, "Sterling"sv}};
            d.title    = "Grandma's wedding"sv;
            d.rating   = 1.0;
            d.album    = "Scans"sv;
            result.Add (key ("Scans/IMG_0100.jpg"), d);
        }
        {
            // values that will not parse are left out (and reported - kProblems_), not the rest
            Document d;
            d.tags  = Set<String>{"Grandma"sv};
            d.album = "Scans"sv;
            result.Add (key ("Scans/IMG_0101.jpg"), d);
        }
        {
            // EXIF alone: a date with no zone stays the camera's local time, with no zone (not Z)
            Document d;
            d.date     = "1950-12-25T09:15:00"sv;
            d.location = Geolocation{51 + 30 / 60.0 + 26.04 / 3600, -(0 + 7 / 60.0 + 39.54 / 3600)}.ToISOString ();
            d.album    = "Scans"sv;
            result.Add (key ("Scans/IMG_0102.jpg"), d);
        }
        {
            // one with its zone, which it keeps
            Document d;
            d.date  = "2019-06-01T18:45:30+02:00"sv;
            d.album = "Scans"sv;
            result.Add (key ("Scans/IMG_0103.jpg"), d);
        }
        {
            // EXIF's rating in stars (digiKam's mapping: 4 is 0.75), and its description as the comment
            Document d;
            d.rating  = 0.75;
            d.comment = Sequence<Document::Comment>{Document::Comment{"Grandpa's farm"sv}};
            d.album   = "Scans"sv;
            result.Add (key ("Scans/IMG_0104.jpg"), d);
        }
        {
            // the percentage, over the stars; and no comment from boilerplate
            Document d;
            d.rating = 0.50;
            d.album  = "Scans"sv;
            result.Add (key ("Scans/IMG_0105.jpg"), d);
        }
        {
            // unrated is no rating; and no comment from the camera's model
            Document d;
            d.album = "Scans"sv;
            result.Add (key ("Scans/IMG_0106.jpg"), d);
        }
        {
            // XMP's standard fields: dc:title, dc:description, dc:subject's keywords, and xmp:Rating in stars (3 is 0.50)
            Document d;
            d.tags    = Set<String>{"Grandpa"sv, "Farm"sv};
            d.title   = "Grandpa's farm"sv;
            d.comment = Sequence<Document::Comment>{Document::Comment{"From the hayloft"sv}};
            d.rating  = 0.50;
            d.album   = "Scans"sv;
            result.Add (key ("Scans/IMG_0107.jpg"), d);
        }
        {
            // text that is not ASCII, read as its source writes it: XMP's as UTF-8
            Document d;
            d.tags    = Set<String>{u"Zo\u00eb"sv};
            d.title   = u"Zo\u00eb's party"sv;
            d.comment = Sequence<Document::Comment>{Document::Comment{u"Caf\u00e9 au lait"sv}};
            d.album   = "Scans"sv;
            result.Add (key ("Scans/IMG_0108.jpg"), d);
        }
        {
            // IPTC's as the character set it names; EXIF's as UTF-8, if it is
            Document d;
            d.tags    = Set<String>{u"M\u00fcller"sv};
            d.title   = u"F\u00eate"sv;
            d.comment = Sequence<Document::Comment>{Document::Comment{u"Grand-m\u00e8re"sv}};
            d.album   = "Scans"sv;
            result.Add (key ("Scans/IMG_0109.jpg"), d);
        }
        {
            // else as Windows-1252 (not Latin-1: 0x93 and 0x94 are curly quotes)
            Document d;
            d.tags    = Set<String>{u"Jos\u00e9"sv};
            d.title   = u"\u201cUncle\u201d Bob"sv;
            d.comment = Sequence<Document::Comment>{Document::Comment{u"Caf\u00e9"sv}};
            d.album   = "Scans"sv;
            result.Add (key ("Scans/IMG_0110.jpg"), d);
        }
        {
            // a photo named outside Windows' code page, read like any other
            Document d;
            d.tags  = Set<String>{"Omega"sv};
            d.album = "Scans"sv;
            result.Add (key (u8"Scans/\u03a9mega.jpg"), d);
        }
        {
            Document d;
            d.album = "Scans"sv;
            result.Add (key ("Scans/unknown.jpg"), d);
        }
        {
            // in both, in digiKam's root album (its position in digiKam all NULL: no location)
            Document d;
            d.tags   = Set<String>{"Uncle Bob"sv};
            d.date   = "1964-07-07T08:00:00"sv;
            d.rating = 0.50;
            d.album  = ""sv;
            result.Add (key ("loose.jpg"), d);
        }
        if (includeDigikamOnly) {
            {
                // digiKam alone (and its comment is one ImportMetadata ignores, and its position, half NULL, is left out -
                // kProblems_)
                Document d;
                d.tags   = Set<String>{"Fred"sv};
                d.date   = "1964-07-06T09:00:00"sv;
                d.rating = 1.0;
                d.album  = "1964 New York"sv;
                result.Add (key ("1964 New York/IMG_0003.jpg"), d);
            }
            {
                // digiKam alone, in its root album (its date will not parse - kProblems_)
                Document d;
                d.tags   = Set<String>{"Fred"sv};
                d.rating = 0.25;
                d.album  = ""sv;
                result.Add (key ("IMG_0004.jpg"), d);
            }
        }
        return result;
    }

    // What ImportMetadata should leave out for Collection_, and report, as ProblemsListing_ lists them: values of photos'
    // documents, from files and from digiKam, and whole photos
    constexpr string_view kProblems_ =
        "1964 New York/IMG_0003.jpg ImagePositions.latitudeNumber, ImagePositions.longitudeNumber: 40.7, NULL\n"
        "1964 New York/IMG_0005.heic ImageInformation.rating: 7\n"
        "IMG_0004.jpg ImageInformation.creationDate: yesterday\n"
        "Scans/IMG_0101.jpg Exif.GPSInfo.GPSLatitude: 48/1 51/1 2960/100\n"
        "Scans/IMG_0101.jpg Exif.Image.Rating: 9\n"
        "Scans/IMG_0101.jpg Exif.Photo.DateTimeOriginal: someday\n"
        "Scans/IMG_0101.jpg Xmp.MicrosoftPhoto.Rating: five\n"
        "Scans/IMG_0101.jpg Xmp.exif.GPSLatitude: somewhere\n"
        "Scans/IMG_0101.jpg Xmp.xmp.CreateDate: last summer\n"
        "Scans/IMG_0101.jpg Xmp.xmp.Rating: lots\n"
        "Scans/broken.jpg\n"
        "Scans/zeros.nef";

    // The problems in a Problems.json, sorted, one per line: the photo (relative to images), and the key and the value (or
    // neither, for a whole photo). Each must give a reason too.
    String ProblemsListing_ (const path& problemsFile, const path& images)
    {
        String            prefix = String{images.generic_wstring ()} + "/"sv;
        SortedSet<String> lines;
        VariantValue      problems = DataExchange::Variant::JSON::Reader{}.Read (IO::FileSystem::FileInputStream::New (problemsFile));
        for (const VariantValue& vp : problems.As<Sequence<VariantValue>> ()) {
            Mapping<String, VariantValue> p    = vp.As<Mapping<String, VariantValue>> ();
            String                        file = p.LookupValue ("file"sv).As<String> ();
            EXPECT_TRUE (file.StartsWith (prefix)) << file;
            EXPECT_FALSE (p.LookupValue ("reason"sv).As<String> ().empty ()) << file;
            String photo = file.StartsWith (prefix) ? file.SubString (prefix.length ()) : file;
            String key   = p.LookupValue ("key"sv).As<String> ();
            lines.Add (key.empty () ? photo : "{} {}: {}"_f(photo, key, p.LookupValue ("value"sv).As<String> ()));
        }
        StringBuilder sb;
        for (const String& l : lines) {
            sb << (sb.empty () ? ""sv : "\n"sv) << l;
        }
        return sb;
    }

    // Runs ImportMetadata on the collection (-s naming its images, as given); returns the DocumentMetaData.json it wrote (and,
    // if stdErr, what it wrote to stderr)
    Mapping<String, Document> RunImportMetadata_ (const Collection_& c, const String& images, const path& outputDirectory,
                                                  bool includeDigikamOnly, String* stdErr = nullptr)
    {
        Sequence<String> args{String{ImportMetadataEXE_ ()}, "-s"sv, images, "-d"sv, String{c.database}, "-o"sv, String{outputDirectory}};
        if (includeDigikamOnly) {
            args.Append ("--include-digikam-only"sv);
        }
        // with a time limit, as a Windows Debug build that aborts shows a dialog, and waits for someone to click it
        String err = get<1> (Execution::ProcessRunner{Execution::CommandLine{args}}.Run (""sv, {}, Time::DurationSeconds{5 * 60}));
        if (stdErr != nullptr) {
            *stdErr = err;
        }
        Mapping<String, Document> result;
        Document::ReadFromJSONFile (&result, outputDirectory / "DocumentMetaData.json");
        return result;
    }

    // Runs ImportMetadata with args, expecting it to fail: to end with an exit status other than 0
    void ExpectFails_ (const Sequence<String>& args)
    {
        Sequence<String> commandLine{String{ImportMetadataEXE_ ()}};
        commandLine.AppendAll (args);
        // RunInBackground, not a timed Run: in a Debug build, that asserts when the program fails (Stroika 3.0d25) - once a
        // Stroika upgrade brings https://github.com/SophistSolutions/Stroika/issues/1261's fix, EXPECT_ANY_THROW a timed Run
        using Execution::ProcessRunner;
        ProcessRunner                    runner{Execution::CommandLine{commandLine}};
        ProcessRunner::BackgroundProcess bp = runner.RunInBackground ();
        bp.Join (Time::DurationSeconds{5 * 60});
        optional<ProcessRunner::ProcessResultType> result = bp.GetProcessResult ();
        ASSERT_TRUE (result.has_value ()) << Characters::ToString (commandLine);
        EXPECT_NE (result->fExitStatus.value_or (ProcessRunner::ExitStatusType{}), ProcessRunner::ExitStatusType{})
            << Characters::ToString (commandLine);
    }

    // Makes dir unreadable - what is in it cannot be listed - while it lives. fWorks is false if it is not: root reads any
    // directory, and so, in a Windows container, does ContainerAdministrator (CI's, both).
    struct ScopedUnreadableDirectory_ {
        ScopedUnreadableDirectory_ (const path& dir)
            : fDir{dir}
        {
#if qStroika_Platform_POSIX
            permissions (dir, perms::none);
            fRestricted_ = true;
#elif qStroika_Platform_Windows
            // an access list with no entries: no one may do anything with it, but its owner restore it
            DWORD size{};
            ::GetFileSecurityW (dir.c_str (), DACL_SECURITY_INFORMATION, nullptr, 0, &size);
            fSavedSecurity_.resize (size);
            ACL                 noEntries;
            SECURITY_DESCRIPTOR sd;
            if (::GetFileSecurityW (dir.c_str (), DACL_SECURITY_INFORMATION, fSavedSecurity_.data (), size, &size) and
                ::InitializeAcl (&noEntries, sizeof (noEntries), ACL_REVISION) and
                ::InitializeSecurityDescriptor (&sd, SECURITY_DESCRIPTOR_REVISION) and ::SetSecurityDescriptorDacl (&sd, true, &noEntries, false)) {
                fRestricted_ = ::SetFileSecurityW (dir.c_str (), DACL_SECURITY_INFORMATION, &sd) != 0;
            }
#endif
            // whether it worked is whether it can still be listed - by this program, and so by the one it runs
            error_code                                           ec;
            [[maybe_unused]] std::filesystem::directory_iterator listing{dir, ec};
            fWorks = fRestricted_ and ec;
        }
        ~ScopedUnreadableDirectory_ ()
        {
            if (fRestricted_) {
#if qStroika_Platform_POSIX
                permissions (fDir, perms::owner_all);
#elif qStroika_Platform_Windows
                ::SetFileSecurityW (fDir.c_str (), DACL_SECURITY_INFORMATION, fSavedSecurity_.data ());
#endif
            }
        }
        path fDir;
        bool fWorks{false};

    private:
        bool fRestricted_{false};
#if qStroika_Platform_Windows
        vector<byte> fSavedSecurity_;
#endif
    };

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
        String      stdErr;
        ExpectSameDocuments_ (RunImportMetadata_ (c, String{c.images}, out, false, &stdErr), ExpectedDocuments_ (c, false));
        EXPECT_EQ (ProblemsListing_ (out / "Problems.json", c.images), kProblems_);
        // and counts them, by kind, on stderr
        EXPECT_TRUE (stdErr.Contains ("Left out (could not read): 10 values, 2 photos - see "sv)) << stdErr;
        // it writes only into its output directory
        EXPECT_EQ (Listing_ (c.root), "Images digikam4.db out"sv);
        EXPECT_EQ (Listing_ (out), "DigikamScrape.json DocumentMetaData.json ExtenstionTally.json FileScrape.json Problems.json heic.txt jpg.txt"sv);
    }

    GTEST_TEST (ImportMetadata, IncludeDigikamOnly_)
    {
        Collection_ c;
        // and -s with a trailing separator makes no difference
        String images = String{c.images / ""};
        ExpectSameDocuments_ (RunImportMetadata_ (c, images, c.root / "out", true), ExpectedDocuments_ (c, true));
    }

#if qStroika_Platform_Windows
    // Windows only: elsewhere Stroika's CommandLine decodes a narrow argument in the C++ global locale - "C", ASCII, unless
    // the program sets it (https://github.com/SophistSolutions/Stroika/issues/1266)
    GTEST_TEST (ImportMetadata, ArgumentsOutsideTheCodePage_)
    {
        // its photos and the output directory named outside Windows' code page (Windows-1252 here), so -s's and -o's
        // arguments are too; not the database: Stroika 3.0d25's SQLite opens a database by its name in the code page
        // (https://github.com/SophistSolutions/Stroika/issues/1265)
        Collection_ c{u8"\u03a9Images"};
        path        out = c.root / u8"\u03a9out";
        ExpectSameDocuments_ (RunImportMetadata_ (c, String{c.images}, out, false), ExpectedDocuments_ (c, false));
        EXPECT_EQ (ProblemsListing_ (out / "Problems.json", c.images), kProblems_);
    }
#endif

    GTEST_TEST (ImportMetadata, UnreadableDirectoryIsReported_)
    {
        Collection_ c;
        // an album it cannot read: left out, and reported - and the rest read, those it comes to after it (Scans) too
        path locked = c.images / "1964 New York" / "Locked";
        WriteJPEG_ (locked / "IMG_0200.jpg", [] (Exiv2::Image& image) { AddIPTCKeywords_ (image, {"Locked"}); });
        ScopedUnreadableDirectory_ unreadable{locked};
        if (not unreadable.fWorks) {
            GTEST_SKIP () << "cannot make a directory unreadable here (root, or ContainerAdministrator, reads any)";
        }
        path   out = c.root / "out";
        String stdErr;
        ExpectSameDocuments_ (RunImportMetadata_ (c, String{c.images}, out, false, &stdErr), ExpectedDocuments_ (c, false));
        String problems = ProblemsListing_ (out / "Problems.json", c.images);
        EXPECT_TRUE (problems.Contains ("1964 New York/Locked\n"sv)) << problems;
        EXPECT_EQ (problems.ReplaceAll ("1964 New York/Locked\n"sv, ""sv), kProblems_);
        EXPECT_TRUE (stdErr.Contains ("Left out (could not read): 10 values, 2 photos, 1 directory - see "sv)) << stdErr;
    }

    GTEST_TEST (ImportMetadata, MissingSourceDirectoryFails_)
    {
        Collection_ c;
        // a source directory it cannot read is an error - not a run that finds no photos
        ExpectFails_ ({"-s"sv, String{c.root / "missing"}, "-o"sv, String{c.root / "out"}});
        ExpectFails_ ({"-s"sv, String{c.database}, "-o"sv, String{c.root / "out"}});
    }

    GTEST_TEST (ImportMetadata, UnreadableDigikamDatabaseFails_)
    {
        Collection_ c;
        // a database it cannot read is an error - not a run that quietly lacks digiKam's data
        ExpectFails_ ({"-s"sv, String{c.images}, "-d"sv, String{c.root / "missing.db"}, "-o"sv, String{c.root / "out"}});
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
