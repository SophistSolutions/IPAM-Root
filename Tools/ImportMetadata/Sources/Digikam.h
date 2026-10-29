#ifndef __IPAM_Tools_ImportMetadata_Digikam_h__
#define __IPAM_Tools_ImportMetadata_Digikam_h__ 1

#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Characters/String.h"
#include "Stroika/Foundation/Containers/Mapping.h"
#include "Stroika/Foundation/Containers/Set.h"
#include "Stroika/Foundation/DataExchange/ObjectVariantMapper.h"

#include "LibIPAM/Metadata/Document.h"

#include "Problem.h"

namespace digikam {
    /**
     *  Each image's Document, as the digiKam database at dbPath has it. A value that will not parse is left out, and given
     *  to problemSink; a database it cannot read throws.
     */
    Containers::Mapping<String, IPAM::LibIPAM::Metadata::Document> ScrapeDigikamDB (std::filesystem::path        dbPath,
                                                                                    const Metadata::ProblemSink& problemSink = nullptr);
}

#endif /*__IPAM_Tools_ImportMetadata_Digikam_h__*/
