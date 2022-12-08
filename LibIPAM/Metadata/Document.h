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
    ToDo:

*/
namespace IPAM::LibIPAM::Metadata {

    class Document {
    public:
        struct Comment {
            Comment (const String& comment_, const optional<String>& author_);
            Comment (const String& comment_);
            Comment() = default;

            nonvirtual bool operator== (const Comment& rhs) const = default;
            nonvirtual auto operator<=> (const Comment& rhs) const = default;

            nonvirtual String ToString() const;
            static String     ToString(Containers::Sequence<Comment>);

            String           comment;
            optional<String> author;
        };

    public:
        Document() = default;

    public:
        Containers::Set<String> tags;
        optional<String>        date;
        optional<String>        location;
        /*
         * Metadata (and digikam) only support a single comment. We instead want a collection of them, as it is natural for
         * everyone to share a collection of comments with different authors for the same document. We will probably
         * have to write out own UI to support adding additional comments (and adding comments to non-photos which don't have metadata)
         */
        optional<Containers::Sequence<Comment>> comment;
        optional<String>                        title;
        optional<double>                        rating; // 0->1.0, mapped to/from various software rating styles
        optional<String>                        album;  // for file system, owning directory path from root

    public:
        static void SupportVariantMapping(DataExchange::ObjectVariantMapper& mapper);
        static void WriteToFileAsJSON (Containers::Mapping<String, Document> mds, const std::filesystem::path& filePath);
        static void ReadFromJSONFile (Containers::Mapping<String, Document>* mds, const std::filesystem::path& filePath);
    };

}

/*
 ********************************************************************************
 ***************************** Implementation Details ***************************
 ********************************************************************************
 */
#include "Document.inl"

#endif /*__IPAM_LibIPAM_Metadata_Document_h__*/
