#include "Stroika/Frameworks/StroikaPreComp.h"

#include <cassert>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>

#if qStroika_Platform_Windows
#include <Windows.h>
#endif

#include "Stroika/Foundation/Characters/CodeCvt.h"
#include "Stroika/Foundation/Characters/CodePage.h"
#include "Stroika/Foundation/Characters/RegularExpression.h"
#include "Stroika/Foundation/Characters/String.h"
#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/Containers/Set.h"
#include "Stroika/Foundation/DataExchange/BadFormatException.h"
#include "Stroika/Foundation/Debug/Trace.h"
#include "Stroika/Foundation/Execution/Throw.h"
#include "Stroika/Foundation/IO/FileSystem/PathName.h"
#include "Stroika/Foundation/Time/Date.h"
#include "Stroika/Foundation/Time/DateTime.h"

#include "Boilerplate.h"
#include "ImageMetadataExtraction.h"
#include "LibIPAM/Common/Geolocation.h"

DISABLE_COMPILER_MSC_WARNING_START (4127)
#include "exiv2/exiv2.hpp"
DISABLE_COMPILER_MSC_WARNING_END (4127)

// ImportMetadata depends on BMFF (HEIC/AVIF) metadata.
#ifndef EXV_ENABLE_BMFF
#error "exiv2 must be built with EXIV2_ENABLE_BMFF=ON - see ThirdPartyComponents/Exiv2/Makefile"
#endif

using namespace std::filesystem;

using namespace Stroika::Foundation;
using namespace Stroika::Foundation::Characters;

using namespace IPAM::LibIPAM::Common;

namespace Metadata {
    namespace {
        // as ExtractAll keys a photo's Document
        String DocumentKey_ (const path& p)
        {
            return String{p}.ReplaceAll ("\\"sv, "/"sv);
        }

        // p as Exiv2 opens it: in UTF-8 (on Windows, with ThirdPartyComponents/Exiv2's patch: Exiv2 0.28 otherwise reads a
        // name in the code page, which cannot name every file); elsewhere, the name's own bytes
        string Exiv2Path_ (const path& p)
        {
            u8string s = p.u8string ();
            return string{s.begin (), s.end ()};
        }

        // ext (lower case) is that of a photo format Exiv2 reads (its ImageType): a file with one that Exiv2 cannot read is a
        // damaged photo, and one with another is some other kind of file (a video, a text file). Exiv2's own guess at the
        // type cannot tell them apart: it takes a text file for some format, and then fails to read it ("corrupted image
        // metadata"), as it does a damaged photo.
        bool IsPhotoExtension_ (const String& ext)
        {
            static const Containers::Set<String> kPhotoExtensions_{
                ".jpg"sv, ".jpeg"sv, ".jpe"sv,  ".tif"sv,  ".tiff"sv, ".png"sv, ".webp"sv, ".gif"sv, ".bmp"sv, ".jp2"sv,
                ".psd"sv, ".heic"sv, ".heif"sv, ".avif"sv, ".jxl"sv,  ".dng"sv, ".cr2"sv,  ".cr3"sv, ".crw"sv, ".nef"sv,
                ".nrw"sv, ".arw"sv,  ".sr2"sv,  ".srf"sv,  ".srw"sv,  ".orf"sv, ".pef"sv,  ".raf"sv, ".rw2"sv, ".mrw"sv};
            return kPhotoExtensions_.Contains (ext);
        }

#if qStroika_Platform_Windows
        // A directory Windows keeps for itself, marked both hidden and system - as Explorer's "protected operating system files"
        // are ($RECYCLE.BIN, System Volume Information): a folder a user customized can be system, but not hidden too
        bool IsWindowsOwnDirectory_ (const path& dir)
        {
            DWORD attributes = ::GetFileAttributesW (dir.c_str ());
            return attributes != INVALID_FILE_ATTRIBUTES and (attributes & FILE_ATTRIBUTE_HIDDEN) != 0 and (attributes & FILE_ATTRIBUTE_SYSTEM) != 0;
        }
#endif

        // Calls onFile for each file under topDir (a symbolic link to one too) but hidden ones - a name starting with '.' -
        // and those in a hidden directory: a directory's files, then its subdirectories'. A directory Windows keeps for itself
        // (its recycle bin, at a drive's root) is skipped too, and given to onSkipped. A directory under topDir that cannot be
        // read, or not to its end, is left out (what of it was not listed), and given to onUnreadable inside the catch, so
        // current_exception () says why; topDir itself throws.
        void ForEachFile_ (const path& topDir, const function<void (const path&)>& onFile, const function<void (const path&)>& onSkipped,
                           const function<void (const path&)>& onUnreadable)
        {
            function<void (const path&)> walk = [&] (const path& dir) {
                vector<path> files;
                vector<path> subdirectories;
                vector<path> skipped;
                try {
                    for (const directory_entry& e : directory_iterator{dir}) {
                        if (e.path ().filename ().native ().starts_with (path::value_type{'.'})) {
                            continue;
                        }
                        if (e.is_directory () and not e.is_symlink ()) {
#if qStroika_Platform_Windows
                            if (IsWindowsOwnDirectory_ (e.path ())) {
                                skipped.push_back (e.path ());
                                continue;
                            }
#endif
                            subdirectories.push_back (e.path ());
                        }
                        else if (e.is_regular_file ()) {
                            files.push_back (e.path ());
                        }
                    }
                }
                catch (...) {
                    if (dir == topDir) {
                        throw;
                    }
                    onUnreadable (dir);
                }
                for (const path& d : skipped) {
                    onSkipped (d);
                }
                for (const path& f : files) {
                    onFile (f);
                }
                for (const path& d : subdirectories) {
                    walk (d);
                }
            };
            walk (topDir);
        }

        // For ForEachFile_'s onUnreadable: a directory whose photos were left out (its contents unknown) - why is current_exception ()
        void ReportUnreadableDirectory_ (const ProblemSink& problemSink, const path& dir)
        {
            Report (problemSink, Problem{.fFile = DocumentKey_ (dir), .fReason = Characters::ToString (current_exception ()), .fDirectory = true});
        }

        // For ForEachFile_'s onSkipped: a directory left out on purpose - a warning, not a failure
        void ReportSkippedDirectory_ (const ProblemSink& problemSink, const path& dir)
        {
            Report (problemSink, Problem{.fFile      = DocumentKey_ (dir),
                                         .fReason    = "skipped: one of Windows' own directories (hidden and system)"sv,
                                         .fDirectory = true,
                                         .fWarning   = true});
        }

        // Text in IPTC or EXIF: UTF-8 if it is valid UTF-8 (IPTC naming UTF-8 as its character set, as digiKam writes it;
        // EXIF's, in practice), else Windows-1252 - as ExifTool reads IPTC that names no character set. Not the machine's
        // code page: a photo's text does not depend on the machine reading it. (XMP's text is always UTF-8.)
        String Text_ (const string& bytes)
        {
            try {
                return String::FromUTF8 (bytes);
            }
            catch (const runtime_error&) {
                static const CodeCvt<wchar_t> kWindows1252_{WellKnownCodePages::kANSI};
                return String{kWindows1252_.Bytes2String<wstring> (as_bytes (span{bytes}))};
            }
        }

        // EXIF's date taken ("1964:07:04 14:30:00"): with its offset from UTC ("-04:00") if the camera knows it, else in the
        // camera's local time, its zone unknown
        Time::DateTime ParseEXIFDateTime_ (const String& dateTime, const optional<String>& offset)
        {
            static const RegularExpression kDateTime_{"^([0-9]{4}):([0-9]{2}):([0-9]{2}) ([0-9]{2}:[0-9]{2}:[0-9]{2})$"sv};
            static const RegularExpression kOffset_{"^[+-][0-9]{2}:[0-9]{2}$"sv};
            Containers::Sequence<String>   m;
            if (not dateTime.Matches (kDateTime_, &m) or (offset and not offset->Matches (kOffset_))) {
                Execution::Throw (DataExchange::BadFormatException{"Invalid EXIF date and time"sv});
            }
            return Time::DateTime::Parse ("{}-{}-{}T{}{}"_f(m[0], m[1], m[2], m[3], offset.value_or (String{})), Time::DateTime::kISO8601Format);
        }

        // An XMP language alternative's text (dc:title's, dc:description's): in the default language, else the first it has
        String LangAltText_ (const Exiv2::Xmpdatum& d)
        {
            if (const auto* langAlt = dynamic_cast<const Exiv2::LangAltValue*> (&d.value ())) {
                auto i = langAlt->value_.find ("x-default");
                if (i == langAlt->value_.end ()) {
                    i = langAlt->value_.begin ();
                }
                return i == langAlt->value_.end () ? String{} : String::FromUTF8 (i->second);
            }
            return String::FromUTF8 (d.toString ());
        }

        // EXIF's GPS coordinate - degrees, minutes and seconds, each a rational - signed by ref: positive or negative
        double EXIFCoordinate_ (const Exiv2::Exifdatum& dms, const Exiv2::Exifdatum* ref, string_view positive, string_view negative)
        {
            if (dms.count () != 3) {
                Execution::Throw (DataExchange::BadFormatException{"Invalid EXIF GPS coordinate (not degrees, minutes and seconds)"sv});
            }
            double units[3];
            for (size_t i = 0; i < 3; ++i) {
                Exiv2::Rational r = dms.toRational (i);
                if (r.second == 0) {
                    Execution::Throw (DataExchange::BadFormatException{"Invalid EXIF GPS coordinate (a zero denominator)"sv});
                }
                units[i] = static_cast<double> (r.first) / r.second;
            }
            if (units[1] >= 60 or units[2] >= 60) {
                Execution::Throw (DataExchange::BadFormatException{"Invalid EXIF GPS coordinate (minutes or seconds >= 60)"sv});
            }
            string direction = ref == nullptr ? string{} : ref->toString ();
            if (direction != positive and direction != negative) {
                Execution::Throw (DataExchange::BadFormatException{"Invalid EXIF GPS coordinate (no N, S, E or W)"sv});
            }
            return (direction == positive ? 1 : -1) * (units[0] + units[1] / 60 + units[2] / 3600);
        }
    }

    ImageMetadataExtractor::ImageMetadataExtractor (const ProblemSink& problemSink)
        : fProblemSink_{problemSink}
    {
    }

    /*
     *  Extract reads a photo's metadata into a Document from three sources in turn - IPTC, XMP, then EXIF - each its own
     *  block below, reading these keys (Exiv2's, less their source's prefix) into these fields:
     *
     *      field       IPTC (Iptc.)                     XMP (Xmp.)                          EXIF (Exif.)
     *      tags        Application2.Keywords            dc.subject, and mwg-rs face         -
     *                                                       region names
     *      title       Application2.ObjectName          acdsee.caption, else dc.title       -
     *      comment     Application2.Caption             acdsee.notes (+ acdsee.author),     Image.ImageDescription
     *                                                       else dc.description
     *      date        Application2.DateCreated         xmp.CreateDate                      Photo.DateTimeOriginal
     *                      (+ TimeCreated)                                                      (+ OffsetTimeOriginal)
     *      location    -                                exif.GPSLatitude, GPSLongitude      GPSInfo.GPSLatitude, GPSLongitude
     *                                                                                           (+ their Refs)
     *      rating      -                                MicrosoftPhoto.Rating,              Image.RatingPercent, else Rating
     *                                                       else xmp.Rating
     *
     *  XMP's value replaces IPTC's (tags add to them); EXIF's - what the camera recorded - only fills in what IPTC and XMP
     *  lack, as they are what someone set since. Within XMP, the standard fields (dc, xmp) fill in what ACDSee's and
     *  MicrosoftPhoto's lack: digiKam writes both, the same.
     *
     *  To read another key: add it to its source's block, read its value inside readValue (so a value that will not parse
     *  is left out and reported, not the photo) - its text with String::FromUTF8 (XMP) or Text_ (IPTC, EXIF), never the
     *  machine's code page - and add it to this table. A value no field holds (EXIF's Artist, XMP's
     *  dc.creator - the photographer) needs a field first: Document.h says how to add one. Not read yet, though a field
     *  holds it: XMP's other dates (photoshop.DateCreated, exif.DateTimeOriginal), hierarchical keywords
     *  (lr.hierarchicalSubject, digiKam.TagsList), and a video's (video.*, https://github.com/SophistSolutions/IPAM-Root/issues/11).
     */
    Metadata::Document ImageMetadataExtractor::Extract (const path& pictFile)
    {
        Metadata::Document ms;

        // runs read, which reads key's value into ms: if it throws, just that value is left out - and reported
        auto readValue = [&] (const String& key, const String& value, const function<void ()>& read) {
            ReadOrReport (fProblemSink_, DocumentKey_ (pictFile), key, value, read);
        };

        Exiv2::Image::UniquePtr image = Exiv2::ImageFactory::open (Exiv2Path_ (pictFile));
        if (image != nullptr) {
            image->readMetadata ();

            auto extract_ipc = [&] () {
                const Exiv2::IptcData& iptcData = image->iptcData ();
                if (not iptcData.empty ()) {
                    constexpr string_view kTagKeyword     = "Iptc.Application2.Keywords";
                    constexpr string_view kDateKeyword    = "Iptc.Application2.DateCreated";
                    constexpr string_view kTimeKeyword    = "Iptc.Application2.TimeCreated";
                    constexpr string_view kTitleKeyword   = "Iptc.Application2.ObjectName";
                    constexpr string_view kCaptionKeyword = "Iptc.Application2.Caption";
                    constexpr string_view kIgnoredValue   = "Ignored";
                    optional<String>      foundDate;
                    optional<String>      foundTime;

                    Exiv2::IptcData::const_iterator end = iptcData.end ();
                    for (Exiv2::IptcData::const_iterator md = iptcData.begin (); md != end; ++md) {
                        if (md->key () == kTagKeyword) {
                            if (md->value ().toString ().length () > 0 and (md->value ().toString () != kIgnoredValue)) {
                                ms.tags.Add (Text_ (md->value ().toString ()));
                            }
                        }
                        else if (md->key () == kTitleKeyword) {
                            if (md->value ().toString ().length () > 0) {
                                ms.title = Text_ (md->value ().toString ());
                            }
                        }
                        else if (md->key () == kCaptionKeyword) {
                            if (md->value ().toString ().length () > 0) {
                                // only one comment supported by metadata so don't need to check if already created
                                ms.comment = Containers::Sequence<Metadata::Document::Comment> ();
                                // comment author not supported by IPTC
                                ms.comment.value ().Append (Metadata::Document::Comment (Text_ (md->value ().toString ())));
                            }
                        }
                        else if (md->key () == kDateKeyword) {
                            if (md->value ().toString ().length () > 0) {
                                foundDate = Text_ (md->value ().toString ());
                            }
                        }
                        else if (md->key () == kTimeKeyword) {
                            if (md->value ().toString ().length () > 0) {
                                foundTime = Text_ (md->value ().toString ());
                            }
                        }
                    }
                    if (foundDate) {
                        using Time::Date;
                        using Time::DateTime;
                        // DbgTrace (L"foundDate =%s, foundTime=%s", Characters::ToString (foundDate).c_str (), Characters::ToString (foundTime).c_str ());
                        String key   = foundTime ? String{kDateKeyword} + ", "sv + String{kTimeKeyword} : String{kDateKeyword};
                        String value = foundTime ? foundDate.value () + "T"sv + foundTime.value () : foundDate.value ();
                        readValue (key, value, [&] () {
                            DateTime dt = foundTime ? DateTime::Parse (value, Time::DateTime::kISO8601Format) : DateTime{Date::Parse (value)};
                            ms.date = Document::FormatDate (dt);
                        });
                        //DbgTrace (L"parsed date = %s", ms.date.value ().c_str ());
                    }
                    return true;
                }
                return false;
            };

            // we do this first. Although considered obsolete and not always there, it handles keywords slightly better
            extract_ipc ();

            {
                Exiv2::XmpData& xmpData = image->xmpData ();
                if (not xmpData.empty ()) {
                    static const String kRatingKeyword    = "Xmp.MicrosoftPhoto.Rating"sv;
                    static const String kTitleKeyword     = "Xmp.acdsee.caption"sv;
                    static const String kCommentKeyword   = "Xmp.acdsee.notes"sv;
                    static const String kCommentAuthor    = "Xmp.acdsee.author"sv;
                    static const String kDateKeyword      = "Xmp.xmp.CreateDate"sv;
                    static const String kAltitudeKeyword  = "Xmp.exif.GPSAltitude"sv;
                    static const String kLatitudeKeyword  = "Xmp.exif.GPSLatitude"sv;
                    static const String kLongitudeKeyword = "Xmp.exif.GPSLongitude"sv;
                    // the standard fields
                    static const String kStandardTagsKeyword    = "Xmp.dc.subject"sv;
                    static const String kStandardTitleKeyword   = "Xmp.dc.title"sv;
                    static const String kStandardCommentKeyword = "Xmp.dc.description"sv;
                    static const String kStandardRatingKeyword  = "Xmp.xmp.Rating"sv;

                    static const Characters::RegularExpression tagEntry{"^Xmp\\.mwg-rs\\.Regions\\/mwg-rs:RegionList\\[(?:[0-9]*)?\\]\\/mwg-rs:Name"sv};
                    optional<String> foundLongitude;
                    optional<String> foundLatitude;
                    optional<String> foundAltitude;
                    optional<String> foundComment;
                    optional<String> foundAuthor;
                    bool             foundTitle{false};
                    optional<String> foundStandardTitle;
                    optional<String> foundStandardComment;
                    optional<string> foundStandardRating;
                    for (Exiv2::XmpData::const_iterator md = xmpData.begin (); md != xmpData.end (); ++md) {
                        String key = String::FromUTF8 (md->key ());
                        if (key == kRatingKeyword) {
                            String value = String::FromUTF8 (md->value ().toString ());
                            readValue (key, value, [&] () { ms.rating = Document::RatingFromPercent (stoi (md->value ().toString ())); });
                        }
                        else if (key == kTitleKeyword) {
                            ms.title   = String::FromUTF8 (md->value ().toString ());
                            foundTitle = true;
                        }
                        else if (key == kStandardTagsKeyword) {
                            for (size_t i = 0; i < md->count (); ++i) {
                                String tag = String::FromUTF8 (md->toString (i));
                                if (not tag.empty () and tag != "Ignored") {
                                    ms.tags.Add (tag);
                                }
                            }
                        }
                        else if (key == kStandardTitleKeyword) {
                            foundStandardTitle = LangAltText_ (*md);
                        }
                        else if (key == kStandardCommentKeyword) {
                            foundStandardComment = LangAltText_ (*md);
                        }
                        else if (key == kStandardRatingKeyword) {
                            foundStandardRating = md->toString ();
                        }
                        else if (key == kCommentKeyword) {
                            foundComment = String::FromUTF8 (md->value ().toString ());
                        }
                        else if (key == kCommentAuthor) {
                            foundAuthor = String::FromUTF8 (md->value ().toString ());
                        }
                        else if (key == kAltitudeKeyword) {
                            foundAltitude = String::FromUTF8 (md->value ().toString ());
                        }
                        else if (key == kLatitudeKeyword) {
                            foundLatitude = String::FromUTF8 (md->value ().toString ());
                        }
                        else if (key == kLongitudeKeyword) {
                            foundLongitude = String::FromUTF8 (md->value ().toString ());
                        }
                        else if (key == kDateKeyword) {
                            String value = String::FromUTF8 (md->value ().toString ());
                            readValue (key, value, [&] () {
                                ms.date = Document::FormatDate (Time::DateTime::Parse (value, Time::DateTime::kISO8601Format));
                            });
                        }
                        else {
                            if (key.Matches (tagEntry)) {
                                ms.tags.Add (String::FromUTF8 (md->value ().toString ()));
                            }
                        }
                    }
                    if (foundLatitude.has_value () and foundLongitude.has_value ()) {
                        // (foundAltitude is not used yet)
                        optional<double> latitude;
                        optional<double> longitude;
                        readValue (kLatitudeKeyword, foundLatitude.value (),
                                   [&] () { latitude = Geolocation::Coordinate::GPSCoordStringToValue (foundLatitude.value ()); });
                        readValue (kLongitudeKeyword, foundLongitude.value (),
                                   [&] () { longitude = Geolocation::Coordinate::GPSCoordStringToValue (foundLongitude.value ()); });
                        if (latitude and longitude) {
                            ms.location = Geolocation (latitude.value (), longitude.value ()).ToISOString ();
                        }
                    }
                    if (foundComment.has_value () and foundComment.value () != "Ignored") {
                        // only one comment supported by metadata so don't need to check if already created
                        ms.comment = Containers::Sequence<Metadata::Document::Comment> ();
                        ms.comment.value ().Append (Metadata::Document::Comment{foundComment.value (), foundAuthor});
                    }
                    else if (foundStandardComment and not foundStandardComment->empty () and *foundStandardComment != "Ignored") {
                        ms.comment = Containers::Sequence<Metadata::Document::Comment>{Metadata::Document::Comment{*foundStandardComment}};
                    }
                    // the standard fields fill in what ACDSee's and MicrosoftPhoto's lack
                    if (not foundTitle and foundStandardTitle and not foundStandardTitle->empty ()) {
                        ms.title = *foundStandardTitle;
                    }
                    if (not ms.rating and foundStandardRating) {
                        readValue (kStandardRatingKeyword, String::FromUTF8 (*foundStandardRating),
                                   [&] () { ms.rating = Document::RatingFromStars (stoi (*foundStandardRating)); });
                    }
                }
            }

            // EXIF - what the camera recorded - fills in only what IPTC and XMP lack: they are what someone set since
            {
                const Exiv2::ExifData& exifData = image->exifData ();
                auto                   find     = [&] (const char* key) -> const Exiv2::Exifdatum* {
                    auto i = exifData.findKey (Exiv2::ExifKey{key});
                    return i == exifData.end () ? nullptr : &*i;
                };
                auto text = [] (const Exiv2::Exifdatum& d) { return Text_ (d.toString ()); };
                if (not ms.date) {
                    if (const Exiv2::Exifdatum* dateTime = find ("Exif.Photo.DateTimeOriginal")) {
                        // a camera whose clock was never set writes zeros, or spaces: no date, rather than a bad one
                        static const RegularExpression kUnset_{"^[0: ]*$"sv};
                        if (not text (*dateTime).Matches (kUnset_)) {
                            const Exiv2::Exifdatum* offset = find ("Exif.Photo.OffsetTimeOriginal");
                            String                  key    = String::FromUTF8 (dateTime->key ());
                            String                  value  = text (*dateTime);
                            if (offset != nullptr) {
                                key   = "{}, {}"_f(key, String::FromUTF8 (offset->key ()));
                                value = "{} {}"_f(value, text (*offset));
                            }
                            readValue (key, value, [&] () {
                                ms.date = Document::FormatDate (
                                    ParseEXIFDateTime_ (text (*dateTime), offset == nullptr ? optional<String>{} : text (*offset)));
                            });
                        }
                    }
                }
                if (not ms.location) {
                    const Exiv2::Exifdatum* latitude  = find ("Exif.GPSInfo.GPSLatitude");
                    const Exiv2::Exifdatum* longitude = find ("Exif.GPSInfo.GPSLongitude");
                    if (latitude != nullptr and longitude != nullptr) {
                        optional<double> lat;
                        optional<double> lon;
                        readValue (String::FromUTF8 (latitude->key ()), text (*latitude),
                                   [&] () { lat = EXIFCoordinate_ (*latitude, find ("Exif.GPSInfo.GPSLatitudeRef"), "N", "S"); });
                        readValue (String::FromUTF8 (longitude->key ()), text (*longitude),
                                   [&] () { lon = EXIFCoordinate_ (*longitude, find ("Exif.GPSInfo.GPSLongitudeRef"), "E", "W"); });
                        if (lat and lon) {
                            ms.location = Geolocation (lat.value (), lon.value ()).ToISOString ();
                        }
                    }
                }
                if (not ms.rating) {
                    // Windows writes both, the same; a percentage says more than stars
                    auto readRating = [&] (const Exiv2::Exifdatum& d, const function<optional<double> (int)>& toRating) {
                        readValue (String::FromUTF8 (d.key ()), text (d), [&] () {
                            if (d.count () != 1) {
                                Execution::Throw (DataExchange::BadFormatException{"Invalid EXIF rating (not one number)"sv});
                            }
                            ms.rating = toRating (static_cast<int> (d.toInt64 ()));
                        });
                    };
                    if (const Exiv2::Exifdatum* percent = find ("Exif.Image.RatingPercent")) {
                        readRating (*percent, Document::RatingFromPercent);
                    }
                    else if (const Exiv2::Exifdatum* stars = find ("Exif.Image.Rating")) {
                        readRating (*stars, Document::RatingFromStars);
                    }
                }
                if (not ms.comment) {
                    // the description: a person's, or the camera's boilerplate
                    if (const Exiv2::Exifdatum* description = find ("Exif.Image.ImageDescription")) {
                        const Exiv2::Exifdatum* make  = find ("Exif.Image.Make");
                        const Exiv2::Exifdatum* model = find ("Exif.Image.Model");
                        if (not IsBoilerplateComment (text (*description), make == nullptr ? String{} : text (*make),
                                                      model == nullptr ? String{} : text (*model))) {
                            ms.comment = Containers::Sequence<Document::Comment>{Document::Comment{text (*description).Trim ()}};
                        }
                    }
                }
            }
        }

        return ms;
    }

    Containers::Set<String> DumpEXIF (Exiv2::Image::UniquePtr& image, std::ofstream& outfile)
    {
        Containers::Set<String> tags;

        outfile << "************** EXIF *******************" << endl;
        Exiv2::ExifData& exifData = image->exifData ();
        if (exifData.empty ()) {
            outfile << "none found" << endl;
        }
        else {
            Exiv2::ExifData::const_iterator end = exifData.end ();
            for (Exiv2::ExifData::const_iterator i = exifData.begin (); i != end; ++i) {
                const char* tn = i->typeName ();
                outfile << std::setw (44) << std::setfill (' ') << std::left << i->key () << " "
                        << "0x" << std::setw (4) << std::setfill ('0') << std::right << std::hex << i->tag () << " " << std::setw (9)
                        << std::setfill (' ') << std::left << (tn ? tn : "Unknown") << " " << std::dec << std::setw (3)
                        << std::setfill (' ') << std::right << i->count () << "  " << std::dec << i->value () << "\n";
            }
        }
        return tags;
    }

    Containers::Set<String> DumpIPTC (Exiv2::Image::UniquePtr& image, std::ofstream& outfile)
    {
        Containers::Set<String> tags;

        outfile << "************** IPTC *******************" << endl;
        const Exiv2::IptcData& iptcData = image->iptcData ();
        if (iptcData.empty ()) {
            outfile << "none found" << endl;
        }
        else {
            constexpr string_view           kTagKeyword = "Iptc.Application2.Keywords";
            Exiv2::IptcData::const_iterator end         = iptcData.end ();
            for (Exiv2::IptcData::const_iterator md = iptcData.begin (); md != end; ++md) {
                outfile << std::setw (44) << std::setfill (' ') << std::left << md->key () << " "
                        << "0x" << std::setw (4) << std::setfill ('0') << std::right << std::hex << md->tag () << " " << std::setw (9)
                        << std::setfill (' ') << std::left << md->typeName () << " " << std::dec << std::setw (3) << std::setfill (' ')
                        << std::right << md->count () << "  " << std::dec << md->value () << std::endl;
                if (md->key () == kTagKeyword) {
                    if (md->value ().toString ().length () > 0) {
                        tags.Add (Text_ (md->value ().toString ()));
                    }
                }
            }
        }
        return tags;
    }

    //Xmp.mwg-rs.Regions/mwg-rs:RegionList[1]/mwg-rs:Name

    Containers::Set<String> DumpXMP (Exiv2::Image::UniquePtr& image, std::ofstream& outfile)
    {
        Containers::Set<String> tags;
        outfile << "************** XMP *******************" << endl;
        Exiv2::XmpData& xmpData = image->xmpData ();
        if (xmpData.empty ()) {
            outfile << "none found" << endl;
        }
        else {
            for (Exiv2::XmpData::const_iterator md = xmpData.begin (); md != xmpData.end (); ++md) {
                outfile << std::setfill (' ') << std::left << std::setw (44) << md->key () << " " << std::setw (9) << std::setfill (' ')
                        << std::left << md->typeName () << " " << std::dec << std::setw (3) << std::setfill (' ') << std::right
                        << md->count () << "  " << std::dec << md->value () << std::endl;
            }
        }
        return tags;
    }

    void ReadImageMetaData (const path& pictFile, String outputDirectoryForSampleFiles)
    {
        try {
            Exiv2::Image::UniquePtr image = Exiv2::ImageFactory::open (Exiv2Path_ (pictFile));
            if (image != nullptr) {
                std::ofstream outfile{outputDirectoryForSampleFiles.As<path> () / (pictFile.extension ().wstring ().substr (1) + L".txt")};

                image->readMetadata ();
                outfile << "metadata of " << String{pictFile}.AsUTF8<string> () << endl;
                Containers::Set<String> tags;
                tags += DumpEXIF (image, outfile);
                tags += DumpIPTC (image, outfile);
                tags += DumpXMP (image, outfile);
                outfile << "************** Found Tags *******************" << endl;
                if (tags.length () > 0) {
                    for (const String& t : tags) {
                        outfile << t.AsUTF8<string> () << endl;
                    }
                }
                else {
                    outfile << "none" << endl;
                }
            }
            else {
                Execution::Throw (Execution::Exception<runtime_error>{"unsupported file type"sv});
            }
        }
        catch (...) {
            DbgTrace ("got exception={}, file={}"_f, current_exception (), pictFile);
        }
    }

    Containers::MultiSet<String> ImageMetadataExtractor::TallyExtensions (const std::filesystem::path& topDir, String outputDirectoryForSampleFiles)
    {
        Containers::MultiSet<String> extTally;
        ForEachFile_ (
            topDir,
            [&] (const path& p) {
                String ext = String{p.extension ().wstring ()}.ToLowerCase ();
                extTally.Add (ext);
                if (extTally.OccurrencesOf (ext) == 1) {
                    DbgTrace ("found '{}' at {}"_f, ext, p);
                    if (outputDirectoryForSampleFiles.length () > 0) {
                        ReadImageMetaData (p, outputDirectoryForSampleFiles);
                    }
                }
            },
            [&] (const path& dir) { ReportSkippedDirectory_ (fProblemSink_, dir); },
            [&] (const path& dir) { ReportUnreadableDirectory_ (fProblemSink_, dir); });
        return extTally;
    }

    Containers::Mapping<String, Metadata::Document> ImageMetadataExtractor::ExtractAll (const std::filesystem::path& topDir)
    {
        Containers::Mapping<String, Metadata::Document> imageMetaData;
        ForEachFile_ (
            topDir,
            [&] (const path& p) {
                try {
                    Metadata::Document ms = Extract (p);
                    // its directory relative to topDir, as digiKam's albums are to their album root: "" for topDir itself
                    path albumDirectory = p.parent_path ().lexically_relative (topDir);
                    ms.album            = albumDirectory == "." ? String{} : String{albumDirectory.generic_wstring ()};
                    imageMetaData.Add (DocumentKey_ (p), ms);
                }
                catch (...) {
                    if (IsPhotoExtension_ (String{p.extension ().wstring ()}.ToLowerCase ())) {
                        Report (fProblemSink_, Problem{.fFile = DocumentKey_ (p), .fReason = Characters::ToString (current_exception ())});
                    }
                    else {
                        DbgTrace ("not a photo Exiv2 reads: {} ({})"_f, p, current_exception ());
                    }
                }
            },
            [&] (const path& dir) { ReportSkippedDirectory_ (fProblemSink_, dir); },
            [&] (const path& dir) { ReportUnreadableDirectory_ (fProblemSink_, dir); });
        return imageMetaData;
    }

}