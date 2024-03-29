/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2022.  All rights reserved
 */
#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/Containers/Mapping.h"
#include "Stroika/Foundation/Containers/SortedMultiSet.h"
#include "Stroika/Foundation/DataExchange/ObjectVariantMapper.h"
#include "Stroika/Foundation/DataExchange/StructFieldMetaInfo.h"
#include "Stroika/Foundation/DataExchange/Variant/JSON/Writer.h"
#include "Stroika/Foundation/Debug/TimingTrace.h"
#include "Stroika/Foundation/Debug/Trace.h"
#include "Stroika/Foundation/IO/FileSystem/FileOutputStream.h"

#include "LibIPAM/Common/Geolocation.h"
#include "LibIPAM/Metadata/Document.h"

using namespace std::filesystem;

using namespace Stroika::Foundation;

using namespace IPAM::LibIPAM;
using namespace IPAM::LibIPAM::Common;

using Characters::String;

const path kDocumentMetaDataFile = L"c:\\ssw\\mdResults\\DocumentMetaData.json";
const path kTagInfoOutputFile    = L"c:\\ssw\\mdResults\\DocumentMetaDataTagInfo.json";

namespace {
    struct TagInfo {
        Containers::Set<String>      photosContaining;
        Containers::MultiSet<String> siblingTagsCount;
    };

    // sorted multiset doesn't currently do what I want (sorts by key, not value)
    // so do the sorting by hand
    struct TagInfoHelper {
        String       key;
        unsigned int value;

        auto operator<=> (const TagInfoHelper& rhs) const = default;
    };

    void LoadMasterTagList ()
    {
        try {

            Containers::Mapping<String, shared_ptr<TagInfo>> fullTagInfo_ptr;

            {
                DbgTrace (L"about to read metadata");
                Containers::Mapping<String, Metadata::Document> pt1;
                {
                    Debug::TimingTrace ttrc;
                    Metadata::Document::ReadFromJSONFile (&pt1, kDocumentMetaDataFile);
                }
                DbgTrace (L"found %d photos metadata", pt1.Keys ().length ());

                for (const auto& pi : pt1) {
                    String key = pi.fKey;
                    // DbgTrace (L"processing %s", key.c_str ());

                    // guarantee they are added to our tag list
                    for (const String& t : pi.fValue.tags) {
                        if (not fullTagInfo_ptr.ContainsKey (t)) {
                            fullTagInfo_ptr.Add (t, make_shared<TagInfo> ());
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
            }
            struct TagInfo_Serialize {
                Containers::Set<String>                     photosContaining;
                Containers::SortedCollection<TagInfoHelper> siblingTagsCount;
            };

            Containers::Mapping<String, TagInfo_Serialize> fullTagInfo;
            {
                DbgTrace (L"processing tag info");
                Debug::TimingTrace ttrc;

                // sorted multiset doesn't currently do what I want (sorts by key, not value)
                // so do the sorting by hand

                for (auto it = fullTagInfo_ptr.begin (); it != fullTagInfo_ptr.end (); ++it) {
                    TagInfo_Serialize ti;
                    ti.photosContaining = it->fValue->photosContaining;
                    for (auto it1 : it->fValue->siblingTagsCount) {
                        ti.siblingTagsCount.Add (TagInfoHelper{it1.fValue, it1.fCount});
                    }

                    fullTagInfo.Add (it->fKey, ti);
                }
            }

            {
                DbgTrace (L"writing processed tag info to %s", kTagInfoOutputFile.c_str ());
                Debug::TimingTrace ttrc;

                using DataExchange::ObjectVariantMapper;
                using DataExchange::StructFieldMetaInfo;
                ObjectVariantMapper tagInfoMapper;
                tagInfoMapper.AddCommonType<Containers::Set<String>> ();
                tagInfoMapper.AddClass<TagInfoHelper> ({
                    ObjectVariantMapper::StructFieldInfo{"key", StructFieldMetaInfo{&TagInfoHelper::key}},
                    ObjectVariantMapper::StructFieldInfo{"value", StructFieldMetaInfo{&TagInfoHelper::value}},
                });
                tagInfoMapper.AddCommonType<Containers::SortedCollection<TagInfoHelper>> ();
                tagInfoMapper.AddClass<TagInfo_Serialize> ({
                    ObjectVariantMapper::StructFieldInfo{"photosContaining", StructFieldMetaInfo{&TagInfo_Serialize::photosContaining}},
                    ObjectVariantMapper::StructFieldInfo{"siblingTagsCount", StructFieldMetaInfo{&TagInfo_Serialize::siblingTagsCount}},
                });
                tagInfoMapper.AddCommonType<Containers::Mapping<String, TagInfo_Serialize>> ();

                DataExchange::Variant::JSON::Writer{}.Write (tagInfoMapper.FromObject (fullTagInfo),
                                                             IO::FileSystem::FileOutputStream::New (kTagInfoOutputFile));
            }
        }
        catch (...) {
            DbgTrace (L"got exception=%s", Characters::ToString (current_exception ()).c_str ());
        }
    }
}

int main ([[maybe_unused]] int argc, [[maybe_unused]] const char* argv[])
{
    Debug::TraceContextBumper ctx{Stroika_Foundation_Debug_OptionalizeTraceArgs (
        L"main", L"argv=%s", Characters::ToString (vector<const char*>{argv, argv + argc}).c_str ())};
#if qDebug
    Geolocation::TestSuite ();
#endif

    LoadMasterTagList ();
    return EXIT_SUCCESS;
}
