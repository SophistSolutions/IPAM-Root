/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2022.  All rights reserved
 */
#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/Containers/Mapping.h"
#include "Stroika/Foundation/Containers/SortedMultiSet.h"
#include "Stroika/Foundation/DataExchange/ObjectVariantMapper.h"
#include "Stroika/Foundation/DataExchange/Variant/JSON/Writer.h"
#include "Stroika/Foundation/Debug/TimingTrace.h"
#include "Stroika/Foundation/Debug/Trace.h"
#include "Stroika/Foundation/Execution/CommandLine.h"
#include "Stroika/Foundation/IO/FileSystem/FileOutputStream.h"

#include "LibIPAM/Common/Geolocation.h"
#include "LibIPAM/Metadata/Document.h"

using namespace std::filesystem;

using namespace Stroika::Foundation;
using namespace Stroika::Foundation::Characters;
using namespace Stroika::Foundation::Containers;

using namespace IPAM::LibIPAM;
using namespace IPAM::LibIPAM::Common;

const path kDocumentMetaDataFile = L"c:\\ssw\\mdResults\\DocumentMetaData.json";
const path kTagInfoOutputFile    = L"c:\\ssw\\mdResults\\DocumentMetaDataTagInfo.json";

namespace {
    struct TagInfo {
        Set<String>      photosContaining;
        MultiSet<String> siblingTagsCount;
    };

    // sorted multiset doesn't currently do what I want (sorts by key, not value)
    // so do the sorting by hand
    struct TagInfoHelper {
        String       key;
        unsigned int value;

        auto operator<=> (const TagInfoHelper& rhs) const = default;
    };

    void LoadMasterTagList_ ()
    {
        try {
            Mapping<String, shared_ptr<TagInfo>> fullTagInfo_ptr;

            {
                DbgTrace ("about to read metadata"_f);
                Mapping<String, Metadata::Document> pt1;
                {
                    Debug::TimingTrace ttrc;
                    Metadata::Document::ReadFromJSONFile (&pt1, kDocumentMetaDataFile);
                }
                DbgTrace ("found {} photos metadata"_f, pt1.Keys ().length ());

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
                        Set<String> rest (pi.fValue.tags.begin () + skipCount, pi.fValue.tags.end ());
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
                Set<String>                     photosContaining;
                SortedCollection<TagInfoHelper> siblingTagsCount;
            };

            Mapping<String, TagInfo_Serialize> fullTagInfo;
            {
                DbgTrace ("processing tag info"_f);
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
                DbgTrace ("writing processed tag info to {}"_f, kTagInfoOutputFile);
                Debug::TimingTrace ttrc;

                using DataExchange::ObjectVariantMapper;
                ObjectVariantMapper tagInfoMapper;
                tagInfoMapper.AddCommonType<Set<String>> ();
                tagInfoMapper.AddClass<TagInfoHelper> ({
                    {"key"sv, &TagInfoHelper::key},
                    {"value"sv, &TagInfoHelper::value},
                });
                tagInfoMapper.AddCommonType<SortedCollection<TagInfoHelper>> ();
                tagInfoMapper.AddClass<TagInfo_Serialize> ({
                    {"photosContaining"sv, &TagInfo_Serialize::photosContaining},
                    {"siblingTagsCount"sv, &TagInfo_Serialize::siblingTagsCount},
                });
                tagInfoMapper.AddCommonType<Mapping<String, TagInfo_Serialize>> ();

                DataExchange::Variant::JSON::Writer{}.Write (tagInfoMapper.FromObject (fullTagInfo),
                                                             IO::FileSystem::FileOutputStream::New (kTagInfoOutputFile));
            }
        }
        catch (...) {
            DbgTrace ("got exception={}"_f, current_exception ());
        }
    }
}

int main ([[maybe_unused]] int argc, [[maybe_unused]] const char* argv[])
{
    Debug::TraceContextBumper ctx{"main", "argv={}"_f, Execution::CommandLine{argc, argv}};
    LoadMasterTagList_ ();
    return EXIT_SUCCESS;
}
