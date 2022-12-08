/*
 * Copyright(c) Sophist Solutions, Inc. 2022.  All rights reserved
 */
#ifndef __IPAM_LibIPAM_Metadata_Document_inl__
#define __IPAM_LibIPAM_Metadata_Document_inl__ 1

namespace IPAM::LibIPAM::Metadata {


    /*
     ********************************************************************************
     ***************************** Implementation Details ***************************
     ********************************************************************************
     */

    inline Document::Comment::Comment (String comment_, optional<String> author_)
        : comment (comment_)
        , author (author_)
    {
    }

    inline Document::Comment::Comment (String comment_)
        : comment (comment_)
    {
    }

    inline bool Document::Comment::operator== (const Comment& rhs) const
    {
        return comment == rhs.comment and author == rhs.author;
    }

    inline auto Document::Comment::operator<=> (const Document::Comment& rhs) const
    {
        auto ans = comment <=> rhs.comment;
        if (ans == 0) {
            return author <=> rhs.author;
        }
        return ans;
    }


}

#endif /*__IPAM_LibIPAM_Metadata_Document_inl__*/
