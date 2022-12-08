/*
 * Copyright(c) Sophist Solutions, Inc. 2022.  All rights reserved
 */
#ifndef __IPAM_LibIPAM_Metadata_Document_inl__
#define __IPAM_LibIPAM_Metadata_Document_inl__ 1

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

#endif /*__IPAM_LibIPAM_Metadata_Document_inl__*/
