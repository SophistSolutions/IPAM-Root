#include "Stroika/Frameworks/StroikaPreComp.h"

#include <cassert>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>

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

    Metadata::Document ImageMetadataExtractor::Extract (const path& pictFile)
    {
        Metadata::Document ms;

        // runs read, which reads key's value into ms: if it throws, just that value is left out - and reported
        auto readValue = [&] (const String& key, const String& value, const function<void ()>& read) {
            ReadOrReport (fProblemSink_, DocumentKey_ (pictFile), key, value, read);
        };

        Exiv2::Image::UniquePtr image = Exiv2::ImageFactory::open (pictFile.string ());
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
                                ms.tags.Add (String::FromNarrowSDKString (md->value ().toString ()));
                            }
                        }
                        else if (md->key () == kTitleKeyword) {
                            if (md->value ().toString ().length () > 0) {
                                ms.title = String::FromNarrowSDKString (md->value ().toString ());
                            }
                        }
                        else if (md->key () == kCaptionKeyword) {
                            if (md->value ().toString ().length () > 0) {
                                // only one comment supported by metadata so don't need to check if already created
                                ms.comment = Containers::Sequence<Metadata::Document::Comment> ();
                                // comment author not supported by IPTC
                                ms.comment.value ().Append (Metadata::Document::Comment (String::FromNarrowSDKString (md->value ().toString ())));
                            }
                        }
                        else if (md->key () == kDateKeyword) {
                            if (md->value ().toString ().length () > 0) {
                                foundDate = String::FromNarrowSDKString (md->value ().toString ());
                            }
                        }
                        else if (md->key () == kTimeKeyword) {
                            if (md->value ().toString ().length () > 0) {
                                foundTime = String::FromNarrowSDKString (md->value ().toString ());
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

                    static const String kRatingNearOne = "99"sv;

                    static const Characters::RegularExpression tagEntry{"^Xmp\\.mwg-rs\\.Regions\\/mwg-rs:RegionList\\[(?:[0-9]*)?\\]\\/mwg-rs:Name"sv};
                    optional<String> foundLongitude;
                    optional<String> foundLatitude;
                    optional<String> foundAltitude;
                    optional<String> foundComment;
                    optional<String> foundAuthor;
                    for (Exiv2::XmpData::const_iterator md = xmpData.begin (); md != xmpData.end (); ++md) {
                        String key = String::FromNarrowSDKString (md->key ());
                        if (key == kRatingKeyword) {
                            String value = String::FromNarrowSDKString (md->value ().toString ());
                            readValue (key, value, [&] () {
                                if (value == kRatingNearOne) {
                                    ms.rating = 1.0; // metadata apparently only allows 2 digits
                                }
                                else {
                                    ms.rating = std::round ((stoi (md->value ().toString ()) / 100.0) * 100.0) / 100.0;
                                }
                            });
                        }
                        else if (key == kTitleKeyword) {
                            ms.title = String::FromNarrowSDKString (md->value ().toString ());
                        }
                        else if (key == kCommentKeyword) {
                            foundComment = String::FromNarrowSDKString (md->value ().toString ());
                        }
                        else if (key == kCommentAuthor) {
                            foundAuthor = String::FromNarrowSDKString (md->value ().toString ());
                        }
                        else if (key == kAltitudeKeyword) {
                            foundAltitude = String::FromNarrowSDKString (md->value ().toString ());
                        }
                        else if (key == kLatitudeKeyword) {
                            foundLatitude = String::FromNarrowSDKString (md->value ().toString ());
                        }
                        else if (key == kLongitudeKeyword) {
                            foundLongitude = String::FromNarrowSDKString (md->value ().toString ());
                        }
                        else if (key == kDateKeyword) {
                            String value = String::FromNarrowSDKString (md->value ().toString ());
                            readValue (key, value, [&] () {
                                ms.date = Document::FormatDate (Time::DateTime::Parse (value, Time::DateTime::kISO8601Format));
                            });
                        }
                        else {
                            if (key.Matches (tagEntry)) {
                                ms.tags.Add (String::FromNarrowSDKString (md->value ().toString ()));
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
                }
            }

            // EXIF - what the camera recorded - fills in only what IPTC and XMP lack: they are what someone set since
            {
                const Exiv2::ExifData& exifData = image->exifData ();
                auto                   find     = [&] (const char* key) -> const Exiv2::Exifdatum* {
                    auto i = exifData.findKey (Exiv2::ExifKey{key});
                    return i == exifData.end () ? nullptr : &*i;
                };
                auto text = [] (const Exiv2::Exifdatum& d) { return String::FromNarrowSDKString (d.toString ()); };
                if (not ms.date) {
                    if (const Exiv2::Exifdatum* dateTime = find ("Exif.Photo.DateTimeOriginal")) {
                        // a camera whose clock was never set writes zeros, or spaces: no date, rather than a bad one
                        static const RegularExpression kUnset_{"^[0: ]*$"sv};
                        if (not text (*dateTime).Matches (kUnset_)) {
                            const Exiv2::Exifdatum* offset = find ("Exif.Photo.OffsetTimeOriginal");
                            String                  key    = String::FromNarrowSDKString (dateTime->key ());
                            String                  value  = text (*dateTime);
                            if (offset != nullptr) {
                                key   = "{}, {}"_f(key, String::FromNarrowSDKString (offset->key ()));
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
                        readValue (String::FromNarrowSDKString (latitude->key ()), text (*latitude),
                                   [&] () { lat = EXIFCoordinate_ (*latitude, find ("Exif.GPSInfo.GPSLatitudeRef"), "N", "S"); });
                        readValue (String::FromNarrowSDKString (longitude->key ()), text (*longitude),
                                   [&] () { lon = EXIFCoordinate_ (*longitude, find ("Exif.GPSInfo.GPSLongitudeRef"), "E", "W"); });
                        if (lat and lon) {
                            ms.location = Geolocation (lat.value (), lon.value ()).ToISOString ();
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
                        tags.Add (String::FromNarrowSDKString (md->value ().toString ()));
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
            Exiv2::Image::UniquePtr image = Exiv2::ImageFactory::open (pictFile.string ());
            if (image != nullptr) {
                std::ofstream outfile (outputDirectoryForSampleFiles.As<path> () / (pictFile.extension ().string ().substr (1) + ".txt"));

                image->readMetadata ();
                outfile << "metadata of " << pictFile.string () << endl;
                Containers::Set<String> tags;
                tags += DumpEXIF (image, outfile);
                tags += DumpIPTC (image, outfile);
                tags += DumpXMP (image, outfile);
                outfile << "************** Found Tags *******************" << endl;
                if (tags.length () > 0) {
                    for (const String& t : tags) {
                        outfile << t.AsNarrowSDKString () << endl;
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
            DbgTrace ("got exception={}, file={}"_f, current_exception (), String::FromNarrowSDKString (pictFile.string ()));
        }
    }

    Containers::MultiSet<String> ImageMetadataExtractor::TallyExtensions (const std::filesystem::path& topDir, String outputDirectoryForSampleFiles)
    {
        Containers::MultiSet<String> extTally;
        try {
            for (recursive_directory_iterator end, dirEntry (topDir); dirEntry != end; ++dirEntry) {
                const path& p    = dirEntry->path ();
                String      name = p.filename ().wstring ();

                if (not name.empty () and name[0] == '.') {
                    if (is_directory (p)) {
                        dirEntry.disable_recursion_pending ();
                    }
                    continue;
                }
                if (is_regular_file (p)) {
                    String ext = String{p.extension ().wstring ()}.ToLowerCase ();
                    extTally.Add (ext);
                    if (extTally.OccurrencesOf (ext) == 1) {
                        DbgTrace ("found '{}' at {}"_f, ext, p);
                        if (outputDirectoryForSampleFiles.length () > 0) {
                            ReadImageMetaData (p, outputDirectoryForSampleFiles);
                        }
                    }
                }
            }
        }
        catch (...) {
            DbgTrace ("got exception={}"_f, current_exception ());
        }

        return extTally;
    }

    Containers::Mapping<String, Metadata::Document> ImageMetadataExtractor::ExtractAll (const std::filesystem::path& topDir)
    {
        Containers::Mapping<String, Metadata::Document> imageMetaData;
        try {
            for (recursive_directory_iterator end, dirEntry (topDir); dirEntry != end; ++dirEntry) {
                const path& p    = dirEntry->path ();
                String      name = p.filename ().wstring ();

                if (not name.empty () and name[0] == '.') {
                    if (is_directory (p)) {
                        dirEntry.disable_recursion_pending ();
                    }
                    continue;
                }

                if (is_regular_file (p)) {
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
                }
            }
        }
        catch (...) {
            DbgTrace ("got exception={}"_f, current_exception ());
        }

        return imageMetaData;
    }

}