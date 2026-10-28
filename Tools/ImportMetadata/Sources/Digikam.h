#ifndef __IPAM_Tools_ImportMetadata_Digikam_h__
#define __IPAM_Tools_ImportMetadata_Digikam_h__ 1

#include "Stroika/Foundation/Characters/String.h"
#include "Stroika/Foundation/Containers/Mapping.h"
#include "Stroika/Foundation/Containers/Set.h"
#include "Stroika/Foundation/DataExchange/ObjectVariantMapper.h"

#include "LibIPAM/Metadata/Document.h"

namespace digikam {
    Containers::Mapping<String, IPAM::LibIPAM::Metadata::Document> ScrapeDigikamDB (std::filesystem::path dbPath);
}

#endif /*__IPAM_Tools_ImportMetadata_Digikam_h__*/
