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
#include "Stroika/Foundation/IO/FileSystem/PathName.h"
#include "Stroika/Foundation/Streams/iostream/OutputStreamFromStdOStream.h"
#include "Stroika/Foundation/Time/DateTime.h"

#include "LibIPAM/Common/Geolocation.h"
#include "LibIPAM/Metadata/Document.h"

#include "Digikam.h"

using namespace Stroika::Foundation;
using namespace Stroika::Foundation::Characters;
using namespace Stroika::Foundation::Containers;

using namespace IPAM;

using namespace LibIPAM::Common;

using namespace LibIPAM;

/*
    Digikam SQL mappings
    _comment -> ImageComments.comment (+ ImageComments.imageid)
    _location -> ImagePositions.latitude, ImagePositions.longitude, ImagePositions.altitude (+ ImagePositions.imageid)

    tags: ImageTags.tagid  (+ ImageTags.imageid)

    to find images need Images.id, Images.name, Images.album, Albums.relativePath
    to find useful tags: TagProperties.property (== persion|faceEngineId and has non-null value)
*/

namespace digikam {

    using namespace Stroika::Foundation::Database;
    using namespace Stroika::Foundation::Database::SQL;

    Mapping<String, Metadata::Document> ScrapeDigikamDB (String dbPath)
    {
        Mapping<String, Metadata::Document> scrapedMetadata;
        try {
            auto conn = SQLite::Connection::New (SQLite::Options{.fDBPath = dbPath.c_str (), .fThreadingMode = SQLite::Options::ThreadingMode::eMultiThread, .fReadOnly = true, .fBusyTimeout = 1s});

            // step one: build imageID map to image path (so can access in masterListOfTags and because is our primary key)
            // album paths are done by reference so first need to build map of those
            // I *think* only one AlbumRoot is supported -- if not will have to rework this somewhat (keep track of rootPathLength separately for each)
            ptrdiff_t rootPathLength = -1;

            Mapping<int, String> albumRoots;
            for (const auto& ari : conn.mkStatement ("Select id,identifier,specificPath from AlbumRoots;"sv).GetAllRows (0, 1, 2)) {
                // example identier: volumeid:?path=P:
                // from internet example: volumeid:?path=%/home/user1/Photo
                int    id           = std::get<0> (ari).As<int> ();
                String path         = std::get<1> (ari).As<String> ();
                String specificPath = std::get<2> (ari).As<String> ();
                /*+ std::get<2> (ari).As<String> ()*/;

                static const auto albumLocation{"^(?:volumeid:\\?path=)(.*)"_RegEx};
                Sequence<String>  matches;
                if (path.Matches (albumLocation, &matches)) {
                    path = matches[0];
                }
                rootPathLength = path.length () + specificPath.length ();
                //DbgTrace (L"root: %d, %s", id, path.c_str ());
                albumRoots.Add (id, path);
            }

            Mapping<int, String> albumPaths;
            for (const auto& ai : conn.mkStatement ("Select id,albumRoot,relativePath from Albums;"sv).GetAllRows (0, 1, 2)) {
                int    id        = std::get<0> (ai).As<int> ();
                int    albumRoot = std::get<1> (ai).As<int> ();
                String name      = albumRoots[albumRoot] + std::get<2> (ai).As<String> ();

                // DbgTrace (L"albumPaths: %d, %s", id,name.c_str ());
                albumPaths.Add (id, name);
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
                    Metadata::Document ms;
                    ms.album =
                        (IO::FileSystem::FromPath (std::filesystem::path{imagePath.c_str ()}.remove_filename ())).SubString (rootPathLength).SubString (0, -1);
                    scrapedMetadata.Add (imagePath, ms);
                };
            }

            // comments
            for (const auto& ii : conn.mkStatement (L"Select imageid,comment,author from ImageComments;").GetAllRows (0, 1, 2)) {
                int                                 id      = std::get<0> (ii).As<int> ();
                String                              comment = std::get<1> (ii).As<String> ();
                String                              author  = std::get<2> (ii).As<String> ();
                static const auto kIgnoreComment_ = "^(\\n*|Screenshot|AppleMark|Maker.*E-ve|CREATOR:.*)"_RegEx;
                if (comment.length () > 0 and not comment.Matches (kIgnoreComment_)) {
                    String path;
                    if (imageIDToImagePath.Lookup (id, &path)) {
                        Metadata::Document ms;
                        scrapedMetadata.Lookup (path, &ms);
                        // only one comment supported by digikam so don't need to check if already created
                        ms.comment = Sequence<Metadata::Document::Comment>{};
                        if (author.length () > 0) {
                            ms.comment.value ().Append (Metadata::Document::Comment{comment, author});
                        }
                        else {
                            ms.comment.value ().Append (Metadata::Document::Comment{comment});
                        }
                        //DbgTrace (L"comment: %d, %s", id, Metadata::Document::Comment::ToString (ms.comment.value ()).c_str ());
                        scrapedMetadata.Add (path, ms);
                    }
                }
            }

            // not sure how to get titles -- perhaps in metadata?

            // location
            for (const auto& ii : conn.mkStatement ("Select imageid,latitudeNumber,longitudeNumber/*,altitude*/ from ImagePositions;"sv).GetAllRows (0, 1, 2)) {
                int    id    = std::get<0> (ii).As<int> ();
                double lat   = std::get<1> (ii).As<double> ();
                double longi = std::get<2> (ii).As<double> ();
                //  double  alt     = std::get<3> (ii).As<double> ();
                String path;
                if (imageIDToImagePath.Lookup (id, &path)) {
                    Metadata::Document ms;
                    scrapedMetadata.Lookup (path, &ms);
                    ms.location = Geolocation (lat, longi /*, alt*/).ToISOString ();
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
                    Metadata::Document ms;
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
                            DbgTrace (L"rating, unknown value = : %d for %s", id, path.c_str ());
                    }
                    if (date.length () > 0) {
                        date = Time::DateTime::Parse (date, Time::DateTime::kISO8601Format).AsUTC ().Format (Time::DateTime::kISO8601Format);
                    }
                    ms.date = date;
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
                if (property == L"person" or property == L"faceEngineId") {
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
                        Metadata::Document ms;
                        scrapedMetadata.Lookup (path, &ms);
                        ms.tags.Add (value);
                        //   DbgTrace (L"date: %d, %s", id, ms.date.value ().c_str ());
                        scrapedMetadata.Add (path, ms);
                    }
                }
            }
        }
        catch (...) {
            DbgTrace (L"ScrapeDigikamDB: got exception=%s", Characters::ToString (current_exception ()).c_str ());
        }
        return scrapedMetadata;
    }
}