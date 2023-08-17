#include <cassert>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>

#include "Stroika/Foundation/Characters/RegularExpression.h"
#include "Stroika/Foundation/Characters/String.h"
#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/Debug/Trace.h"
#include "Stroika/Foundation/IO/FileSystem/PathName.h"
#include "Stroika/Foundation/Time/Date.h"
#include "Stroika/Foundation/Time/DateTime.h"

#include "ImageMetadataExtraction.h"
#include "LibIPAM/Common/Geolocation.h"

DISABLE_COMPILER_MSC_WARNING_START (4127)
#include "exiv2/exiv2.hpp"
DISABLE_COMPILER_MSC_WARNING_END (4127)

using namespace std::filesystem;

using namespace Stroika::Foundation;
using Characters::String;

using namespace IPAM::LibIPAM::Common;

namespace Metadata {
    ImageMetadataExtractor::ImageMetadataExtractor ()
    {
        bool bmffSupported = Exiv2::enableBMFF (true);
        DbgTrace (L"bmffSupported = %s", (bmffSupported) ? L"true" : L"false");
    }

    Metadata::Document ImageMetadataExtractor::Extract (const path& pictFile)
    {
        Metadata::Document ms;

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
                        DateTime dt = [&] () {
                            if (foundTime) {
                                return DateTime::Parse (foundDate.value () + "T"sv + foundTime.value (), Time::DateTime::kISO8601Format);
                            }
                            else {
                                return DateTime{Date::Parse (foundDate.value ())};
                            }
                        }();
                        ms.date = dt.AsUTC ().Format (Time::DateTime::kISO8601Format);
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
                    optional<String>              foundLongitude;
                    optional<String>              foundLatitude;
                    optional<String>              foundAltitude;
                    optional<String>              foundComment;
                    optional<String>              foundAuthor;
                    for (Exiv2::XmpData::const_iterator md = xmpData.begin (); md != xmpData.end (); ++md) {
                        String key = String::FromNarrowSDKString (md->key ());
                        if (key == kRatingKeyword) {
                            if (String::FromNarrowSDKString (md->value ().toString ()) == kRatingNearOne) {
                                ms.rating = 1.0; // metadata apparently only allows 2 digits
                            }
                            else {
                                ms.rating = std::round ((stoi (md->value ().toString ()) / 100.0) * 100.0) / 100.0;
                            }
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
                            ms.date = Time::DateTime::Parse (String::FromNarrowSDKString (md->value ().toString ()), Time::DateTime::kISO8601Format).AsUTC ().Format (Time::DateTime::kISO8601Format);
                        }
                        else {
                            if (key.Matches (tagEntry)) {
                                ms.tags.Add (String::FromNarrowSDKString (md->value ().toString ()));
                            }
                        }
                    }
                    if (foundLatitude.has_value () and foundLongitude.has_value ()) {
                        if (foundAltitude.has_value ()) {
                            ms.location = Geolocation (Geolocation::Coordinate::GPSCoordStringToValue (foundLatitude.value ()), Geolocation::Coordinate::GPSCoordStringToValue (foundLongitude.value ()) /*, double _altitude*/).ToISOString ();
                        }
                        else {
                            ms.location = Geolocation (Geolocation::Coordinate::GPSCoordStringToValue (foundLatitude.value ()), Geolocation::Coordinate::GPSCoordStringToValue (foundLongitude.value ())).ToISOString ();
                        }
                    }
                    if (foundComment.has_value () and foundComment.value () != "Ignored") {
                        // only one comment supported by metadata so don't need to check if already created
                        ms.comment = Containers::Sequence<Metadata::Document::Comment> ();
                        ms.comment.value ().Append (Metadata::Document::Comment{foundComment.value (), foundAuthor});
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
                outfile << std::setw (44) << std::setfill (' ') << std::left
                        << i->key () << " "
                        << "0x" << std::setw (4) << std::setfill ('0') << std::right
                        << std::hex << i->tag () << " "
                        << std::setw (9) << std::setfill (' ') << std::left
                        << (tn ? tn : "Unknown") << " "
                        << std::dec << std::setw (3)
                        << std::setfill (' ') << std::right
                        << i->count () << "  "
                        << std::dec << i->value ()
                        << "\n";
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
                outfile << std::setw (44) << std::setfill (' ') << std::left
                        << md->key () << " "
                        << "0x" << std::setw (4) << std::setfill ('0') << std::right
                        << std::hex << md->tag () << " "
                        << std::setw (9) << std::setfill (' ') << std::left
                        << md->typeName () << " "
                        << std::dec << std::setw (3)
                        << std::setfill (' ') << std::right
                        << md->count () << "  "
                        << std::dec << md->value ()
                        << std::endl;
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
                outfile << std::setfill (' ') << std::left
                        << std::setw (44)
                        << md->key () << " "
                        << std::setw (9) << std::setfill (' ') << std::left
                        << md->typeName () << " "
                        << std::dec << std::setw (3)
                        << std::setfill (' ') << std::right
                        << md->count () << "  "
                        << std::dec << md->value ()
                        << std::endl;
            }
        }
        return tags;
    }

    void ReadImageMetaData (const path& pictFile, String outputDirectoryForSampleFiles)
    {
        try {
            Exiv2::Image::UniquePtr image = Exiv2::ImageFactory::open (pictFile.string ());
            if (image != nullptr) {
                std::ofstream outfile (outputDirectoryForSampleFiles.AsNarrowSDKString () + pictFile.extension ().string ().substr (1) + ".txt");

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
                Execution::Throw (Execution::RuntimeErrorException{L"unsupported file type"sv});
            }
        }
        catch (...) {
            DbgTrace (L"got exception=%s, file=%s", Characters::ToString (current_exception ()).c_str (), String::FromNarrowSDKString (pictFile.string ()).c_str ());
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
                        DbgTrace (L"found '%s' at %s", ext.c_str (), p.c_str ());
                        if (outputDirectoryForSampleFiles.length () > 0) {
                            ReadImageMetaData (p, outputDirectoryForSampleFiles);
                        }
                    }
                }
            }
        }
        catch (...) {
            DbgTrace (L"got exception=%s", Characters::ToString (current_exception ()).c_str ());
        }

        return extTally;
    }

    Containers::Mapping<String, Metadata::Document> ImageMetadataExtractor::ExtractAll (const std::filesystem::path& topDir)
    {
        Containers::Mapping<String, Metadata::Document> imageMetaData;

        size_t topDirLength = IO::FileSystem::FromPath (topDir).size ();
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
                        ms.album              = IO::FileSystem::FromPath (path (p).remove_filename ()).SubString (topDirLength).SubString (0, -1).ReplaceAll ("\\"sv, "/"sv);
                        imageMetaData.Add (IO::FileSystem::FromPath (p).ReplaceAll ("\\"sv, "/"sv), ms);
                    }
                    catch (...) {
                        DbgTrace (L"failed to find metadata for  %s", p.c_str ());
                    }
                }
            }
        }
        catch (...) {
            DbgTrace (L"got exception=%s", Characters::ToString (current_exception ()).c_str ());
        }

        return imageMetaData;
    }

} // namespace