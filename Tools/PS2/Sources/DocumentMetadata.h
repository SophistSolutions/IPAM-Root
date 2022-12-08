#pragma once

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
namespace Metadata {

class DocumentMetadata {
    public:
        struct Comment {
            Comment(String comment_, optional<String> author_);
            Comment(String comment_);
            Comment() = default;

            nonvirtual bool operator== (const Comment& rhs) const;
            nonvirtual auto operator<=> (const Comment& rhs) const;


            nonvirtual String ToString() const;
            static String     ToString(Containers::Sequence<Comment>);

            String           comment;
            optional<String> author;
        };

    public:
        DocumentMetadata() = default;

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
        static void WriteToFileAsJSON(Containers::Mapping<String, DocumentMetadata> mds, const std::filesystem::path& filePath);
        static void ReadFromJSONFile(Containers::Mapping<String, DocumentMetadata>* mds, const std::filesystem::path& filePath);
};

/*
 ********************************************************************************
 ***************************** Implementation Details ***************************
 ********************************************************************************
 */

    inline DocumentMetadata::Comment::Comment(String comment_, optional<String> author_)
        : comment(comment_)
        , author(author_)
    {
    }

    inline DocumentMetadata::Comment::Comment(String comment_)
        : comment(comment_)
    {
    }

    inline bool DocumentMetadata::Comment::operator== (const Comment& rhs) const
    {
        return comment == rhs.comment and author == rhs.author;
    }

    inline auto DocumentMetadata::Comment::operator<=> (const DocumentMetadata::Comment& rhs) const
    {
        auto ans = comment <=> rhs.comment;
        if (ans == 0) {
            return author <=> rhs.author;
        }
        return ans;
    }

} // namespace
