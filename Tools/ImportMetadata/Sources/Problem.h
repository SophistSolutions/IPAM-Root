/*
 * Copyright(c) Sophist Solutions, Inc. 2026.  All rights reserved
 */
#ifndef __IPAM_Tools_ImportMetadata_Problem_h__
#define __IPAM_Tools_ImportMetadata_Problem_h__ 1

#include "Stroika/Frameworks/StroikaPreComp.h"

#include <functional>

#include "Stroika/Foundation/Characters/String.h"

namespace Metadata {

    /**
     *  A value an importer could not read, and so left out of its photo's Document - for whoever cares (ImportMetadata
     *  lists them in Problems.json).
     */
    struct Problem {
        Stroika::Foundation::Characters::String fFile;   ///< the photo, as its Document is keyed
        Stroika::Foundation::Characters::String fKey;    ///< where the value was: Exiv2's key (Xmp.exif.GPSLatitude, ...)
        Stroika::Foundation::Characters::String fValue;  ///< the value, as text
        Stroika::Foundation::Characters::String fReason; ///< why it would not do: what reading it threw
    };

    /**
     *  Told of each Problem, as it is found
     */
    using ProblemSink = std::function<void (const Problem&)>;

}

#endif /*__IPAM_Tools_ImportMetadata_Problem_h__*/
