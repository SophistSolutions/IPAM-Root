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
     *  A value an importer could not read, and so left out of its photo's Document - or a whole photo, left out - for
     *  whoever cares (ImportMetadata lists them in Problems.json).
     */
    struct Problem {
        Stroika::Foundation::Characters::String fFile;   ///< the photo, as its Document is keyed
        Stroika::Foundation::Characters::String fKey;    ///< where it was: Exiv2's key, or digiKam's table.column; empty: the whole photo
        Stroika::Foundation::Characters::String fValue;  ///< the value, as text; empty for a whole photo
        Stroika::Foundation::Characters::String fReason; ///< why it would not do: what reading it threw
    };

    /**
     *  Told of each Problem, as it is found
     */
    using ProblemSink = std::function<void (const Problem&)>;

    /**
     *  Traces problem, and gives it to problemSink (if any)
     */
    void Report (const ProblemSink& problemSink, const Problem& problem);

    /**
     *  Runs read, which reads file's value of key: if it throws, Report()s that, so the caller goes on without it
     */
    void ReadOrReport (const ProblemSink& problemSink, const Stroika::Foundation::Characters::String& file,
                       const Stroika::Foundation::Characters::String& key, const Stroika::Foundation::Characters::String& value,
                       const std::function<void ()>& read);

}

#endif /*__IPAM_Tools_ImportMetadata_Problem_h__*/
