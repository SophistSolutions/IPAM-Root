#include <iostream>
#include <fstream>

#include "Stroika/Foundation/Characters/String.h"
#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/Containers/Mapping.h"
#include "Stroika/Foundation/Containers/MultiSet.h"
#include "Stroika/Foundation/Containers/Set.h"
#include "Stroika/Foundation/DataExchange/StructFieldMetaInfo.h"
#include "Stroika/Foundation/DataExchange/Variant/JSON/Reader.h"
#include "Stroika/Foundation/DataExchange/Variant/JSON/Writer.h"
#include "Stroika/Foundation/IO/FileSystem/FileInputStream.h"
#include "Stroika/Foundation/IO/FileSystem/FileOutputStream.h"


#include "DocumentMetadata.h"
#include "TagInfo.h"


using namespace Stroika::Foundation;
using Characters::String;

namespace Metadata {

    void TagInfo::SupportVariantMapping (DataExchange::ObjectVariantMapper& mapper)
    {
        using DataExchange::ObjectVariantMapper;
        using DataExchange::StructFieldMetaInfo;
        mapper.AddCommonType<Containers::Set<String>> ();
        mapper.AddClass<TagInfoHelper> ({
            ObjectVariantMapper::StructFieldInfo{L"key", StructFieldMetaInfo{&TagInfoHelper::key}},
            ObjectVariantMapper::StructFieldInfo{L"value", StructFieldMetaInfo{&TagInfoHelper::value}},
        });
        mapper.AddCommonType<Containers::SortedCollection<TagInfoHelper>> ();
        mapper.AddClass<TagInfo_Serialize> ({
            ObjectVariantMapper::StructFieldInfo{L"photosContaining", StructFieldMetaInfo{&TagInfo_Serialize::photosContaining}},
            ObjectVariantMapper::StructFieldInfo{L"siblingTagsCount", StructFieldMetaInfo{&TagInfo_Serialize::siblingTagsCount}},
        });
    }

    void TagInfo::WriteToFileAsJSON (Containers::Mapping<String, TagInfo_Serialize> mds, const std::filesystem::path filePath)
    {
        DataExchange::ObjectVariantMapper mapper;
        TagInfo::SupportVariantMapping (mapper);
        mapper.AddCommonType<Containers::Mapping<String, TagInfo_Serialize>> ();

        DataExchange::Variant::JSON::Writer{}.Write (mapper.FromObject (mds), IO::FileSystem::FileOutputStream::New (filePath));
    }

    Containers::Mapping<String, TagInfo_Serialize> TagInfo::ReadFromJSONFile (const std::filesystem::path filePath)
    {
        DataExchange::ObjectVariantMapper mapper;
        TagInfo::SupportVariantMapping (mapper);
        mapper.AddCommonType<Containers::Mapping<String, TagInfo_Serialize>> ();
        DataExchange::VariantValue                     xxx = DataExchange::Variant::JSON::Reader{}.Read (IO::FileSystem::FileInputStream::New (filePath));
        Containers::Mapping<String, TagInfo_Serialize> mds;
        mapper.ToObject (xxx, &mds);
        return mds;
    }

    Containers::Mapping<String, TagInfo*> TagInfo::ProcessMetadata (Containers::Mapping<String, DocumentMetadata> dm)
    {
        Containers::Mapping<String, TagInfo*> fullTagInfo_ptr;
        for (auto pi : dm) {
            String key = pi.fKey;
            // DbgTrace (L"processing %s", key.c_str ());

            // guarantee they are added to our tag list
            for (String t : pi.fValue.tags) {
                if (not fullTagInfo_ptr.ContainsKey (t)) {
                    fullTagInfo_ptr.Add (t, new TagInfo ());
                }
            }

            int skipCount = 1;
            for (auto it = pi.fValue.tags.begin (); it != pi.fValue.tags.end (); ++it) {
                auto tinfo = fullTagInfo_ptr.LookupValue (*it);
                tinfo->photosContaining.Add (key);
                Containers::Set<String> rest (pi.fValue.tags.begin () + skipCount, pi.fValue.tags.end ());
                for (auto it1 : rest) {
                    //   DbgTrace (L"  matched pair (%s,%s)", it->c_str (), it1.c_str ());
                    tinfo->siblingTagsCount.Add (it1);
                    fullTagInfo_ptr.LookupValue (it1)->siblingTagsCount.Add (*it);
                }
                ++skipCount;
                //  DbgTrace (L"  finished tag %s, photo count = %d, sibling count = %d", it->c_str (), fullTagInfo.LookupValue (*it)->photosContaining.Count (), fullTagInfo.LookupValue (*it)->siblingTagsCount.Count ());
            }
        }
        return fullTagInfo_ptr;
    }

    Containers::Mapping<String, TagInfo_Serialize> TagInfo::PrepareForSerialization (Containers::Mapping<String, TagInfo*> fullTagInfo_ptr)
    {
        // sorted multiset doesn't currently do what I want (sorts by key, not value)
        // so do the sorting by hand

        Containers::Mapping<String, TagInfo_Serialize> fullTagInfo;
        for (auto it = fullTagInfo_ptr.begin (); it != fullTagInfo_ptr.end (); ++it) {
            TagInfo_Serialize ti;
            ti.photosContaining = it->fValue->photosContaining;
            for (auto it1 : it->fValue->siblingTagsCount) {
                ti.siblingTagsCount.Add (TagInfoHelper (it1.fValue, it1.fCount));
            }

            fullTagInfo.Add (it->fKey, ti);
        }
        return fullTagInfo;
    }
}
