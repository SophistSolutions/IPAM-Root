/*
 * Copyright(c) Sophist Solutions, Inc. 2022.  All rights reserved
 */

namespace IPAM::LibIPAM::Metadata {

    /*
     ********************************************************************************
     ************************ Metadata::Document::Comment ***************************
     ********************************************************************************
     */
    inline Document::Comment::Comment (const String& comment_, const optional<String>& author_)
        : comment{comment_}
        , author{author_}
    {
    }
    inline Document::Comment::Comment (const String& comment_)
        : comment{comment_}
    {
    }

}
