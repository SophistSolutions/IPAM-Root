/*
 * Copyright(c) Sophist Solutions, Inc. 2022.  All rights reserved
 */
#ifndef __IPAM_LibIPAM_Metadata_Document_h__
#define __IPAM_LibIPAM_Metadata_Document_h__ 1

#include "Stroika/Foundation/Characters/String.h"
#include "Stroika/Foundation/Containers/Mapping.h"
#include "Stroika/Foundation/Containers/Sequence.h"
#include "Stroika/Foundation/Containers/Set.h"
#include "Stroika/Foundation/DataExchange/ObjectVariantMapper.h"

using namespace Stroika::Foundation;
using Characters::String;

/*
    The information associated with documents shared with IPAM. The canonical example of a shared document is a photographic image, and existing metadata standards for images 
    influenced the design here. However, since the different photo metadata standards support different sets of information (and are designed in part for needs other than what 
    ordinary viewers of images are interested in) and crucially because IPAM supports documents other than images, for which there is no metadata standard, the metadata IPAM stores
    is separate from the documents themselves. 

    The IPAM metadata is saved separated for different "owners" of a shared document. Viewers of the document receive all of the metadata different owners of the document have created 
    iff they have permitted access to the metadata, which each owner can separately specify. Viewers can get both a "rolled up" single version of the metadata, or a mapping of the metadas
    by creator.

    The supported data is mostly information that can already set by (some) image organizing software, and that can be stored in (some) image format metadata.

    date:
    There are many dates saved in image metadata, but here we mean whatever date the user thinks of the document representing. This is often the "creation" date
    for an image, but not always (scanned images will have a creation date very different from the date the image was actually taken, which is what we would want).
    We store the dates in UTC time in ISO 8601 string format.

    title:
    an optional piece of descriptive text to explain what the document is about. Useful to keep separate from tags for UI display purposes. One implication of it not being
    a tag is that it probably won't be supported for the automatic mapping that IPAM viewers can set up

    location:
    a geolocation stored in ISO 6709 format. 

    rating:
    some kind of numeric ranking of image importance/quality. We use a number between zero and one, which is mapped from the systems supported by various UIs.

    album:
    a path in a heirarchically organized set of semantic categories. An example might be "1964/Summer/Trip to New York". Sometimes these are created in a UI, at other
    times it can be automatically scraped from how documents are stored in a file system.

    comment:
    arbitrary text describing a document, with an (optional) associated author. The author is not assumed to be part of the IPAM system, and is not part of the 
    security model (rather the person who records the comment and is the 'owner' of the document controls access to all of the comments in their documents). But 
    the author can still be valuable, since someone can solicit comments from people such as their relatives who are not interested in setting up IPAM but 
    may have valuable information about documents that are being shared. Metadata (and digikam) only support a single comment. We instead want a collection of them, 
    as it is natural for everyone to share a collection of comments with different people for the same document. 

    tag:
    a set of strings associated with the document. Traditionally most tags in image metadata have a recording rectangle within the image. We do not save that here, to
    support that you will want to make sure that the image metadata saves this information. A feature of IPAM is that image viewers can specify preference mappings from a 
    particular owner's tag names to their own, so that "Frederick Barbarossa" can be transformed to "Uncle Fred" without changing any of the saved information.
    A useful feature of most image management software is the ability to support a small 'thumbnail' image with tags for UI display purposes. We will probably want to 
    support that, but it would be in a separate data stream.
*/

/*
    ToDo:
        Is there a good way to make the location be a Geolocation rather than a string here
        Similary, should date be a DateTime data type rather than a string? Do we then want to independently document import/export from usage format?
        
        Should we support multiple albums for a document so that single creators can include it in multiple collections (most people achieve this via the tag system, but
        it requires creating tags that are probably conceptually album names, and is flat rather than heirarchical, so it might be cleaner to to it here)

        How to handle tag thumbnails?
*/

namespace IPAM::LibIPAM::Metadata {

    class Document {
    public:
        struct Comment {
            Comment (const String& comment_, const optional<String>& author_);
            Comment (const String& comment_);
            Comment () = default;

            nonvirtual bool operator== (const Comment& rhs) const  = default;
            nonvirtual auto operator<=> (const Comment& rhs) const = default;

            nonvirtual String ToString () const;
            static String     ToString (Containers::Sequence<Comment>);

            String           comment;
            optional<String> author;
        };

    public:
        Document () = default;

    public:
        Containers::Set<String> tags;
        optional<String>        date;
        optional<String>        location;

        optional<Containers::Sequence<Comment>> comment;
        optional<String>                        title;
        optional<double>                        rating; // 0->1.0, mapped to/from various software rating styles
        optional<String>                        album;  // for file system, owning directory path from root

    public:
        static void SupportVariantMapping (DataExchange::ObjectVariantMapper& mapper);
        static void WriteToFileAsJSON (Containers::Mapping<String, Document> mds, const std::filesystem::path& filePath);
        static void ReadFromJSONFile (Containers::Mapping<String, Document>* mds, const std::filesystem::path& filePath);
    };

} //namespace IPAM::LibIPAM::Metadata

/*
 ********************************************************************************
 ***************************** Implementation Details ***************************
 ********************************************************************************
 */
#include "Document.inl"

#endif /*__IPAM_LibIPAM_Metadata_Document_h__*/
