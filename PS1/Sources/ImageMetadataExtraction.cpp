#include <iostream>
#include <filesystem>
#include <cstring>
#include <iomanip>
#include <cassert>
#include <fstream>

#include "Stroika/Foundation/Characters/RegularExpression.h"
#include "Stroika/Foundation/Characters/String.h"
#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/Debug/Trace.h"
#include "Stroika/Foundation/IO/FileSystem/PathName.h"
#include "Stroika/Foundation/Time/Date.h"
#include "Stroika/Foundation/Time/DateTime.h"


#include "Geolocation.h"
#include "ImageMetadataExtraction.h"

#include "exiv2/exiv2.hpp"


using namespace std::filesystem;

using namespace Stroika::Foundation;
using Characters::String;

using Metadata::DocumentMetadata;



namespace Metadata {
    ImageMetadataExtractor::ImageMetadataExtractor ()
    {
    }

    DocumentMetadata ImageMetadataExtractor::Extract (const path& pictFile)
    {
        DocumentMetadata ms;

        Exiv2::Image::UniquePtr image = Exiv2::ImageFactory::open (pictFile.string ());
        if (image.get () != 0) {
            image->readMetadata ();

            auto extract_ipc = [&] () {
                Exiv2::IptcData& iptcData = image->iptcData ();
                if (not iptcData.empty ()) {
                    const char* kTagKeyword     = "Iptc.Application2.Keywords";
                    const char* kDateKeyword    = "Iptc.Application2.DateCreated";
                    const char* kTimeKeyword    = "Iptc.Application2.TimeCreated";
                    const char* kTitleKeyword   = "Iptc.Application2.ObjectName";
                    const char* kCaptionKeyword = "Iptc.Application2.Caption";
                    optional<String> foundDate;
                    optional<String> foundTime;

                    Exiv2::IptcData::iterator end = iptcData.end ();
                    for (Exiv2::IptcData::iterator md = iptcData.begin (); md != end; ++md) {
                        if (strcmp (md->key ().c_str (), kTagKeyword) == 0) {
                            if (md->value ().toString ().length () > 0 and strcmp (md->value ().toString ().c_str (), "Ignored") != 0) {
                                ms.tags.Add (String::FromNarrowSDKString (md->value ().toString ()));
                            }
                        }
                        else if (strcmp (md->key ().c_str (), kTitleKeyword) == 0) {
                            if (md->value ().toString ().length () > 0) {
                                ms.title = String::FromNarrowSDKString (md->value ().toString ());
                            }
                        }
                        else if (strcmp (md->key ().c_str (), kCaptionKeyword) == 0) {
                            if (md->value ().toString ().length () > 0) {
                                    // only one comment supported by metadata so don't need to check if already created
                                    ms.comment = Containers::Sequence<DocumentMetadata::Comment> ();
                                // comment author not supported by IPTC
                                ms.comment.value ().Append (DocumentMetadata::Comment (String::FromNarrowSDKString (md->value ().toString ())));
                            }
                        }
                        else if (strcmp (md->key ().c_str (), kDateKeyword) == 0) {
                            if (md->value ().toString ().length () > 0) {
                                foundDate = String::FromNarrowSDKString (md->value ().toString ());
                            }
                        }
                        else if (strcmp (md->key ().c_str (), kTimeKeyword) == 0) {
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
                                return DateTime::Parse (foundDate.value () + L"T" + foundTime.value (), Time::DateTime::kISO8601Format);
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
                    const char* kRatingKeyword      = "Xmp.MicrosoftPhoto.Rating";
                    const char* kTitleKeyword       = "Xmp.acdsee.caption";
                    const char* kCommentKeyword     = "Xmp.acdsee.notes";
                    const char* kCommentAuthor      = "Xmp.acdsee.author";
                    const char* kDateKeyword        = "Xmp.xmp.CreateDate";
                    const char* kAltitudeKeyword    = "Xmp.exif.GPSAltitude";
                    const char* kLatitudeKeyword    = "Xmp.exif.GPSLatitude";
                    const char* kLongitudeKeyword   = "Xmp.exif.GPSLongitude";
                        

                    Characters::RegularExpression tagEntry{L"^Xmp\\.mwg-rs\\.Regions\\/mwg-rs:RegionList\\[(?:[0-9]*)?\\]\\/mwg-rs:Name"};
                    optional<String>              foundLongitude;
                    optional<String>              foundLatitude;
                    optional<String>              foundAltitude;
                    optional<String>              foundComment;
                    optional<String>              foundAuthor;
                    for (Exiv2::XmpData::const_iterator md = xmpData.begin (); md != xmpData.end (); ++md) {
                        String key = String::FromNarrowSDKString (md->key ().c_str ());
                        if (strcmp (md->key ().c_str (), kRatingKeyword) == 0) {
                            if (strcmp (md->value ().toString ().c_str (), "99") == 0) {
                                ms.rating = 1.0;    // metadata apparently only allows 2 digits
                           }
                           else {
                               ms.rating = std::round ((stoi (md->value ().toString ()) / 100.0) * 100.0) / 100.0;
                           }
                        }
                        else if (strcmp (md->key ().c_str (), kTitleKeyword) == 0) {
                            ms.title = String::FromNarrowSDKString (md->value ().toString ());
                        }
                        else if (strcmp (md->key ().c_str (), kCommentKeyword) == 0) {
                            foundComment = String::FromNarrowSDKString (md->value ().toString ());
                        }
                        else if (strcmp (md->key ().c_str (), kCommentAuthor) == 0) {
                            foundAuthor = String::FromNarrowSDKString (md->value ().toString ());
                        }
                        else if (strcmp (md->key ().c_str (), kAltitudeKeyword) == 0) {
                            foundAltitude = String::FromNarrowSDKString (md->value ().toString ());
                        }
                        else if (strcmp (md->key ().c_str (), kLatitudeKeyword) == 0) {
                            foundLatitude = String::FromNarrowSDKString (md->value ().toString ());
                        }
                        else if (strcmp (md->key ().c_str (), kLongitudeKeyword) == 0) {
                            foundLongitude = String::FromNarrowSDKString (md->value ().toString ());
                        }
                        else if (strcmp (md->key ().c_str (), kDateKeyword) == 0) {
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
                    if (foundComment.has_value () and foundComment.value ()!= L"Ignored") {
                        // only one comment supported by metadata so don't need to check if already created
                        ms.comment = Containers::Sequence<DocumentMetadata::Comment> ();
                        ms.comment.value ().Append (DocumentMetadata::Comment (String (foundComment.value ().c_str ()), foundAuthor));
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
        Exiv2::IptcData& iptcData = image->iptcData ();
        if (iptcData.empty ()) {
            outfile << "none found" << endl;
        }
        else {
            const char*               kTagKeyword = "Iptc.Application2.Keywords";
            Exiv2::IptcData::iterator end         = iptcData.end ();
            for (Exiv2::IptcData::iterator md = iptcData.begin (); md != end; ++md) {
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
                if (strcmp (md->key ().c_str (), kTagKeyword) == 0) {
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

    string wstring2string (const wstring& s)
    {
        int len;
        int slength = (int)s.length () + 1;
        len         = WideCharToMultiByte (CP_ACP, 0, s.c_str (), slength, 0, 0, 0, 0);
        std::string r (len, '\0');
        WideCharToMultiByte (CP_ACP, 0, s.c_str (), slength, &r[0], len, 0, 0);
        return r;
    }

    void ReadImageMetaData (const path& pictFile, const char* outputDirectoryForSampleFiles)
    {
        try {
            Exiv2::Image::UniquePtr image = Exiv2::ImageFactory::open (pictFile.string ());
            if (image.get () != 0) {
                std::ofstream outfile (outputDirectoryForSampleFiles + pictFile.extension ().string ().substr (1) + ".txt");

                image->readMetadata ();
                outfile << "metadata of " << pictFile.string () << endl;
                Containers::Set<String> tags;
                tags += DumpEXIF (image, outfile);
                tags += DumpIPTC (image, outfile);
                tags += DumpXMP (image, outfile);
                outfile << "************** Found Tags *******************" << endl;
                if (tags.length () > 0) {
                    for (String t : tags) {
                        outfile << wstring2string (t.c_str ()) << endl;
                    }
                }
                else {
                    outfile << "none" << endl;
                }
            }
            else {
                throw exception ("unsupported file type");
            }
        }
        catch (...) {
            DbgTrace (L"got exception=%s, file=%s", Characters::ToString (current_exception ()).c_str (), String::FromNarrowSDKString (pictFile.string ()).c_str ());
        }
    }

    Containers::MultiSet<String> ImageMetadataExtractor::TallyExtensions (const std::filesystem::path& topDir, const char* outputDirectoryForSampleFiles)
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
                        if (outputDirectoryForSampleFiles != nullptr) {
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

    Containers::Mapping<String, DocumentMetadata> ImageMetadataExtractor::ExtractAll (const std::filesystem::path& topDir)
    {
        Containers::Mapping<String, DocumentMetadata> imageMetaData;

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
                        DocumentMetadata ms = Extract (p);
                        ms.album            = IO::FileSystem::FromPath (path (p).remove_filename ()).SubString (topDirLength).SubString (0, -1).ReplaceAll (L"\\", L"/");
                        imageMetaData.Add (IO::FileSystem::FromPath (p).ReplaceAll (L"\\", L"/"), ms);
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



}   // namespace