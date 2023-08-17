#pragma once

#include "Stroika/Foundation/Characters/String.h"
#include "Stroika/Foundation/Containers/Mapping.h"
#include "Stroika/Foundation/Containers/MultiSet.h"
#include "Stroika/Foundation/DataExchange/ObjectVariantMapper.h"

using namespace Stroika::Foundation;
using Characters::String;

namespace Metadata {

    struct TagInfo_Serialize;
    struct TagInfo {
        Containers::Set<String>      photosContaining;
        Containers::MultiSet<String> siblingTagsCount;

        static void SupportVariantMapping (DataExchange::ObjectVariantMapper& mapper);
        static void WriteToFileAsJSON (Containers::Mapping<String, TagInfo_Serialize> mds, const std::filesystem::path filePath);
        static Containers::Mapping<String, TagInfo_Serialize> ReadFromJSONFile (const std::filesystem::path filePath);

        static Containers::Mapping<String, TagInfo*> ProcessMetadata (Containers::Mapping<String, DocumentMetadata> dm);
        static Containers::Mapping<String, TagInfo_Serialize> PrepareForSerialization (Containers::Mapping<String, TagInfo*> fullTagInfo_ptr);
    };

    // sorted multiset doesn't currently do what I want (sorts by key, not value)
    // so do the sorting by hand
    struct TagInfoHelper {
        String       key;
        unsigned int value;
    };

    inline bool operator< (const TagInfoHelper& lhs, const TagInfoHelper& rhs)
    {
        if (lhs.value > rhs.value) {
            return true;
        }
        else if (lhs.value < rhs.value) {
            return false;
        }
        return lhs.key < rhs.key;
    }

    struct TagInfo_Serialize {
        Containers::Set<String>                     photosContaining;
        Containers::SortedCollection<TagInfoHelper> siblingTagsCount;
    };
}
