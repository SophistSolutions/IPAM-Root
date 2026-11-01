/*
 * Copyright(c) Sophist Solutions, Inc. 2026.  All rights reserved
 */
#ifndef __IPAM_Tools_ImportMetadata_Boilerplate_h__
#define __IPAM_Tools_ImportMetadata_Boilerplate_h__ 1

#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Characters/RegularExpression.h"
#include "Stroika/Foundation/Characters/String.h"

namespace Metadata {

    /**
     *  \brief Whether a photo's comment - from a file's metadata, or digiKam, which reads it from there - is text a camera or
     *  program wrote, not a person: blank, the camera's make or model, or one of kBoilerplate_'s. Such a comment is left out,
     *  quietly. Extend kBoilerplate_ as more turn up.
     */
    inline bool IsBoilerplateComment (const Stroika::Foundation::Characters::String& comment,
                                      const Stroika::Foundation::Characters::String& make  = {},
                                      const Stroika::Foundation::Characters::String& model = {})
    {
        using namespace std::string_view_literals;
        using namespace Stroika::Foundation::Characters;
        // each the whole comment (trimmed), in any case
        static const RegularExpression kBoilerplate_{
            "Screenshot|AppleMark|Maker.*E-ve|CREATOR:.*" // seen in LGP's and Sterling's digiKam database
            "|OLYMPUS DIGITAL CAMERA|SONY DSC|KODAK Digital Still Camera|MINOLTA DIGITAL CAMERA|SAMSUNG CAMERA PICTURES"
            "|Exif_JPEG_PICTURE|LEAD Technologies Inc\\. V1\\.01"sv,
            eCaseInsensitive};
        static constexpr String::EqualsComparer kSame_{eCaseInsensitive};
        String                                  c = comment.Trim ();
        return c.empty () or c.Matches (kBoilerplate_) or (not make.Trim ().empty () and kSame_ (c, make.Trim ())) or
               (not model.Trim ().empty () and kSame_ (c, model.Trim ()));
    }

}

#endif /*__IPAM_Tools_ImportMetadata_Boilerplate_h__*/
