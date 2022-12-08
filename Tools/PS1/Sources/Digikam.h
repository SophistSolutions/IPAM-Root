#pragma once

#include "Stroika/Foundation/Characters/String.h"
#include "Stroika/Foundation/Containers/Mapping.h"
#include "Stroika/Foundation/Containers/Set.h"
#include "Stroika/Foundation/DataExchange/ObjectVariantMapper.h"

#include "LibIPAM/Common/DocumentMetadata.h"

namespace digikam {
    extern Containers::Mapping<String, Metadata::DocumentMetadata> ScrapeDigikamDB (String dbPath);
}
