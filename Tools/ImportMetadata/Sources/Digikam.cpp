#include "Stroika/Frameworks/StroikaPreComp.h"

#include <cmath>

#include "Stroika/Foundation/Common/StroikaVersion.h"

// Stroika's IO::FileSystem::Ptr::GetCreationTime, from its 3.0d26 release (https://github.com/SophistSolutions/Stroika/issues/1264
// - not the 3.0d26x builds before it, which may lack it); before it, GetCreationTime_'s own
#define qIPAM_StroikaHasGetCreationTime_                                                                                                   \
    (qStroika_Version_FullVersion >= Stroika_Make_FULL_VERSION (3, 0, qStroika_Version_Stage_Dev, 26, 1))

#if not qIPAM_StroikaHasGetCreationTime_
#if qStroika_Platform_Linux
#include <fcntl.h>
#include <sys/stat.h>
#elif qStroika_Platform_MacOS
#include <sys/stat.h>
#elif qStroika_Platform_Windows
#include <Windows.h>
#endif
#endif

#include "Stroika/Foundation/Characters/RegularExpression.h"
#include "Stroika/Foundation/Characters/String.h"
#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/Containers/Mapping.h"
#include "Stroika/Foundation/Containers/MultiSet.h"
#include "Stroika/Foundation/Containers/Set.h"
#include "Stroika/Foundation/DataExchange/ObjectVariantMapper.h"
#include "Stroika/Foundation/DataExchange/StructFieldMetaInfo.h"
#include "Stroika/Foundation/DataExchange/Variant/JSON/Writer.h"
#include "Stroika/Foundation/Database/SQL/ODBC.h"
#include "Stroika/Foundation/Database/SQL/SQLite.h"
#include "Stroika/Foundation/Database/SQL/Statement.h"
#include "Stroika/Foundation/Debug/Trace.h"
#include "Stroika/Foundation/Execution/Activity.h"
#include "Stroika/Foundation/IO/FileSystem/FileSystem.h"
#include "Stroika/Foundation/IO/FileSystem/PathName.h"
#include "Stroika/Foundation/Streams/iostream/OutputStreamFromStdOStream.h"
#include "Stroika/Foundation/Time/DateTime.h"

#include "LibIPAM/Common/Geolocation.h"
#include "LibIPAM/Metadata/Document.h"

#include "Digikam.h"

using namespace Stroika::Foundation;
using namespace Stroika::Foundation::Characters;
using namespace Stroika::Foundation::Containers;
using namespace Stroika::Foundation::Database;
using namespace Stroika::Foundation::Database::SQL;
using Stroika::Foundation::Time::DateTime;

using namespace IPAM;

using namespace LibIPAM::Common;

// not using namespace LibIPAM: its Metadata would make Metadata:: (Problem.h's) ambiguous
using LibIPAM::Metadata::Document;

/*
    Digikam SQL mappings
    _comment -> ImageComments.comment (+ ImageComments.imageid)
    _location -> ImagePositions.latitude, ImagePositions.longitude, ImagePositions.altitude (+ ImagePositions.imageid)

    tags: ImageTags.tagid  (+ ImageTags.imageid)

    to find images need Images.id, Images.name, Images.album, Albums.relativePath
    to find useful tags: TagProperties.property (== persion|faceEngineId and has non-null value)
*/

namespace {
    // When file was created, if its file system records that (on Linux, only some do); nullopt too if it cannot be read
    optional<DateTime> GetCreationTime_ (const filesystem::path& file)
    {
#if qIPAM_StroikaHasGetCreationTime_
        return IO::FileSystem::Default ().GetCreationTime (file);
#elif qStroika_Platform_Linux
        struct statx s{};
        if (::statx (AT_FDCWD, file.c_str (), 0, STATX_BTIME, &s) == 0 and (s.stx_mask & STATX_BTIME)) {
            return DateTime{static_cast<time_t> (s.stx_btime.tv_sec)};
        }
        return nullopt;
#elif qStroika_Platform_MacOS
        struct stat s{};
        if (::stat (file.c_str (), &s) == 0) {
            return DateTime{static_cast<time_t> (s.st_birthtimespec.tv_sec)};
        }
        return nullopt;
#elif qStroika_Platform_Windows
        WIN32_FILE_ATTRIBUTE_DATA attributes{};
        if (::GetFileAttributesExW (file.c_str (), GetFileExInfoStandard, &attributes)) {
            return DateTime{attributes.ftCreationTime};
        }
        return nullopt;
#else
        return nullopt;
#endif
    }

    // Whether a digiKam date is one of its file's times - when created or last modified - to the second: digiKam's scanner,
    // finding no date in a photo's metadata, takes the earlier of the two (ItemScanner::creationDateFromFilesystem) - a guess,
    // not a date the user set. Which digiKam keeps, with no zone - the file's local time or UTC - is not known, so either
    // matches. False if the file cannot be read (digiKam's path names another machine's disk, say).
    // A date set with digiKam's Adjust Time & Date, its "File last modified" box checked, matches too: the file then has it
    // in its metadata as well - except a RAW or HEIC file, whose metadata goes to a sidecar.
    bool IsFileTime_ (const DateTime& digikamDate, const filesystem::path& file)
    {
        Sequence<DateTime> fileTimes;
        try {
            fileTimes.Append (IO::FileSystem::Default ().GetFileLastModificationDate (file).AsUTC ());
            if (optional<DateTime> created = GetCreationTime_ (file)) {
                fileTimes.Append (created->AsUTC ());
            }
        }
        catch (...) {
            return false;
        }
        Sequence<DateTime> asUTC;
        if (digikamDate.GetTimezone ()) {
            asUTC.Append (digikamDate.AsUTC ());
        }
        else {
            asUTC.Append (DateTime{digikamDate.GetDate (), digikamDate.GetTimeOfDay (), Time::Timezone::kLocalTime}.AsUTC ());
            asUTC.Append (DateTime{digikamDate.GetDate (), digikamDate.GetTimeOfDay (), Time::Timezone::kUTC});
        }
        return fileTimes.Any ([&] (const DateTime& t) { return asUTC.Contains (t); });
    }
}

namespace digikam {

    Mapping<String, Document> ScrapeDigikamDB (std::filesystem::path dbPath, const Metadata::ProblemSink& problemSink)
    {
        auto activity = Execution::LazyEvalActivity ([&] () -> String { return "reading the digiKam database {}"_f(dbPath); });
        Execution::DeclareActivity declareActivity{&activity};
        Mapping<String, Document>  scrapedMetadata;
        try {
            // fCreateDBPathIfDoesNotExist defaults true, which read-only must not have (https://github.com/SophistSolutions/Stroika/issues/1262)
            auto conn = SQLite::Connection::New (SQLite::Connection::Options{.fDBPath                     = dbPath,
                                                                             .fCreateDBPathIfDoesNotExist = false,
                                                                             .fThreadingMode = SQLite::Connection::Options::ThreadingMode::eMultiThread,
                                                                             .fReadOnly    = true,
                                                                             .fBusyTimeout = 1s});

            // step one: build imageID map to image path (so can access in masterListOfTags and because is our primary key)
            // album paths are done by reference so first need to build map of those
            Mapping<int, String> albumRoots;
            for (const auto& ari : conn.mkStatement ("Select id,identifier from AlbumRoots;"sv).GetAllRows (0, 1)) {
                // example identier: volumeid:?path=P:
                // from internet example: volumeid:?path=%/home/user1/Photo
                int    id   = std::get<0> (ari).As<int> ();
                String path = std::get<1> (ari).As<String> ();

                static const auto albumLocation{"^(?:volumeid:\\?path=)(.*)"_RegEx};
                Sequence<String>  matches;
                if (path.Matches (albumLocation, &matches)) {
                    path = matches[0];
                }
                //DbgTrace (L"root: %d, %s", id, path.c_str ());
                albumRoots.Add (id, path);
            }

            // An album's relativePath is its directory relative to its album root, after a "/" - and "/" for the album root
            // itself. Without the "/" it is the album's name, as ImageMetadataExtractor names one: "" for the album root.
            Mapping<int, String> albumPaths;
            Mapping<int, String> albumNames;
            for (const auto& ai : conn.mkStatement ("Select id,albumRoot,relativePath from Albums;"sv).GetAllRows (0, 1, 2)) {
                int    id           = std::get<0> (ai).As<int> ();
                int    albumRoot    = std::get<1> (ai).As<int> ();
                String relativePath = std::get<2> (ai).As<String> ();
                String name         = relativePath.StartsWith ("/"sv) ? relativePath.SubString (1) : relativePath;

                // DbgTrace (L"albumPaths: %d, %s", id,name.c_str ());
                albumPaths.Add (id, name.empty () ? albumRoots[albumRoot] : albumRoots[albumRoot] + "/" + name);
                albumNames.Add (id, name);
            }
            Mapping<int, String> imageIDToImagePath;

            for (const auto& ii : conn.mkStatement ("Select id,album,name from Images;"sv).GetAllRows (0, 1, 2)) {
                int    id    = std::get<0> (ii).As<int> ();
                int    album = std::get<1> (ii).As<int> ();
                String name  = std::get<2> (ii).As<String> ();

                if (album != 0) {
                    String imagePath = albumPaths[album] + "/" + name;
                    //DbgTrace (L"got: %d, %d, %s, %s", id, album, name.c_str (), imagePath.c_str ());
                    imageIDToImagePath.Add (id, imagePath);
                    Document ms;
                    ms.album = albumNames[album];
                    scrapedMetadata.Add (imagePath, ms);
                };
            }

            // comments
            for (const auto& ii : conn.mkStatement ("Select imageid,comment,author from ImageComments;").GetAllRows (0, 1, 2)) {
                int               id              = std::get<0> (ii).As<int> ();
                String            comment         = std::get<1> (ii).As<String> ();
                String            author          = std::get<2> (ii).As<String> ();
                static const auto kIgnoreComment_ = "^(\\n*|Screenshot|AppleMark|Maker.*E-ve|CREATOR:.*)"_RegEx;
                if (comment.length () > 0 and not comment.Matches (kIgnoreComment_)) {
                    String path;
                    if (imageIDToImagePath.Lookup (id, &path)) {
                        Document ms;
                        scrapedMetadata.Lookup (path, &ms);
                        // only one comment supported by digikam so don't need to check if already created
                        ms.comment = Sequence<Document::Comment>{};
                        if (author.length () > 0) {
                            ms.comment.value ().Append (Document::Comment{comment, author});
                        }
                        else {
                            ms.comment.value ().Append (Document::Comment{comment});
                        }
                        //DbgTrace (L"comment: %d, %s", id, Document::Comment::ToString (ms.comment.value ()).c_str ());
                        scrapedMetadata.Add (path, ms);
                    }
                }
            }

            // not sure how to get titles -- perhaps in metadata?

            // location: a NULL coordinate reads as NaN, VariantValue's no value - none is no position, and half of one cannot be
            // used, so is left out (and reported)
            for (const auto& ii :
                 conn.mkStatement ("Select imageid,latitudeNumber,longitudeNumber/*,altitude*/ from ImagePositions;"sv).GetAllRows (0, 1, 2)) {
                int    id    = std::get<0> (ii).As<int> ();
                double lat   = std::get<1> (ii).As<double> ();
                double longi = std::get<2> (ii).As<double> ();
                //  double  alt     = std::get<3> (ii).As<double> ();
                String path;
                if (imageIDToImagePath.Lookup (id, &path)) {
                    if (std::isnan (lat) or std::isnan (longi)) {
                        if (not(std::isnan (lat) and std::isnan (longi))) {
                            auto text = [] (double d) { return std::isnan (d) ? "NULL"_k : "{}"_f(d); };
                            Metadata::Report (problemSink, Metadata::Problem{.fFile = path,
                                                                             .fKey = "ImagePositions.latitudeNumber, ImagePositions.longitudeNumber"sv,
                                                                             .fValue = "{}, {}"_f(text (lat), text (longi)),
                                                                             .fReason = std::isnan (lat) ? "no latitude"sv : "no longitude"sv});
                        }
                        continue;
                    }
                    Document ms;
                    scrapedMetadata.Lookup (path, &ms);
                    ms.location = Geolocation{lat, longi /*, alt*/}.ToISOString ();
                    //   DbgTrace (L"geoloc: %d, %s", id, ms.location.value ().c_str ());
                    scrapedMetadata.Add (path, ms);
                }
            }

            // date, rating
            for (const auto& ii : conn.mkStatement ("Select imageid,rating,creationDate from ImageInformation;"sv).GetAllRows (0, 1, 2)) {
                int    id     = std::get<0> (ii).As<int> ();
                int    rating = std::get<1> (ii).As<int> ();
                String date   = std::get<2> (ii).As<String> ();
                String path;
                if (imageIDToImagePath.Lookup (id, &path)) {
                    Document ms;
                    scrapedMetadata.Lookup (path, &ms);

                    // digikam does an unusual mapping of its user selected stars to percentile value (as shown in stored metadata in files)
                    // I choose to respect their mapping, rather than the most obvious, where 1 star would be 20% instead of 1%
                    switch (rating) {
                        case -1:
                        case 0: // for me at least, these are the same (visually same, probably no way back to -1 if accidentally set)
                            break;
                        case 1:
                            ms.rating = 0.01;
                            break;
                        case 2:
                            ms.rating = 0.25;
                            break;
                        case 3:
                            ms.rating = 0.50;
                            break;
                        case 4:
                            ms.rating = 0.75;
                            break;
                        case 5:
                            ms.rating = 1.0;
                            break;
                        default:
                            DbgTrace ("rating, unknown value = : {} for {}"_f, id, path);
                    }
                    // a date that will not parse is left out (and reported), not the rest of the row; none (NULL) is no date, and
                    // nor is digiKam's guess, its file's time
                    if (not date.empty ()) {
                        Metadata::ReadOrReport (problemSink, path, "ImageInformation.creationDate"sv, date, [&] () {
                            DateTime d = DateTime::Parse (date, DateTime::kISO8601Format);
                            if (IsFileTime_ (d, path.As<filesystem::path> ())) {
                                DbgTrace ("digiKam's date for {} is its file's time, not a date the user set: {}"_f, path, date);
                            }
                            else {
                                ms.date = Document::FormatDate (d);
                            }
                        });
                    }
                    //   DbgTrace (L"date: %d, %s", id, ms.date.value ().c_str ());
                    scrapedMetadata.Add (path, ms);
                }
            }

            // not all tags are meaningful -- we want only those that tag 'people'. These are marked as either 'person' or 'faceEngineId'
            // unfortunately, they can also have other entries on this list AND there are tags with no entries at all
            // (these are tags that people create but that are not attached to a rectangle in the image)
            Set<int> meaningfulTags;
            Set<int> badTags;
            for (const auto& ii : conn.mkStatement ("Select tagid,property from TagProperties;"sv).GetAllRows (0, 1)) {
                int    id       = std::get<0> (ii).As<int> ();
                String property = std::get<1> (ii).As<String> ();
                if (property == "person" or property == "faceEngineId") {
                    //DbgTrace (L"meaningfulTags added: %d, %s", id, property.c_str ());
                    meaningfulTags.Add (id);
                    badTags.RemoveIf (id);
                }
                else if (not meaningfulTags.Contains (id)) {
                    //DbgTrace (L"bad tags added: %d, %s", id, property.c_str ());
                    badTags.Add (id);
                }
            }
            Mapping<int, String> tagValues;
            for (const auto& ii : conn.mkStatement ("Select id,name from Tags;"sv).GetAllRows (0, 1)) {
                int    id   = std::get<0> (ii).As<int> ();
                String name = std::get<1> (ii).As<String> ();
                if (not badTags.Contains (id)) {
                    tagValues.Add (id, name);
                    //DbgTrace (L"tagValues added: %d, %s", id, name.c_str ());
                }
            }
            for (const auto& ii : conn.mkStatement ("Select imageid,tagid from ImageTags;"sv).GetAllRows (0, 1)) {
                int    imageid = std::get<0> (ii).As<int> ();
                int    tagid   = std::get<1> (ii).As<int> ();
                String value;
                if (tagValues.Lookup (tagid, &value)) {
                    String path;
                    if (imageIDToImagePath.Lookup (imageid, &path)) {
                        Document ms;
                        scrapedMetadata.Lookup (path, &ms);
                        ms.tags.Add (value);
                        //   DbgTrace (L"date: %d, %s", id, ms.date.value ().c_str ());
                        scrapedMetadata.Add (path, ms);
                    }
                }
            }
        }
        catch (...) {
            DbgTrace ("ScrapeDigikamDB: got exception={}"_f, current_exception ());
            throw; // a database it cannot read is an error, not a scrape that stops part-way
        }
        return scrapedMetadata;
    }
}