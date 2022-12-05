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
#include "Stroika/Foundation/Debug/Trace.h"
#include "Stroika/Foundation/Streams/iostream/OutputStreamFromStdOStream.h"


#include "DocumentMetadata.h"


using namespace Stroika::Foundation;
using Characters::String;



namespace Metadata {
    String DocumentMetadata::Comment::ToString () const
    {
        Characters::StringBuilder sb;
        sb += L"{";
        sb += L"comment: " + comment + L", ";
        sb += L"author: ";
        sb += (author.has_value ()) ? author.value ().c_str () : L"";
        sb += L"}";
        return sb.str ();
    }

    String DocumentMetadata::Comment::ToString (Containers::Sequence<Comment> comments)
    {
        Characters::StringBuilder sb;
        sb += L"{";
        for (auto it : comments) {
            sb += it.ToString ();
        }
        sb += L"}";
        return sb.str ();
    }


    DocumentMetadata::DocumentMetadata ()
    {
    }

    void DocumentMetadata::SupportVariantMapping (DataExchange::ObjectVariantMapper& mapper)
    {
        using DataExchange::ObjectVariantMapper;
        using DataExchange::StructFieldMetaInfo;
        mapper.AddCommonType<Containers::Set<String>> ();

        mapper.AddClass<DocumentMetadata::Comment> ({
            ObjectVariantMapper::StructFieldInfo{L"comment", StructFieldMetaInfo{&DocumentMetadata::Comment::comment}},
            ObjectVariantMapper::StructFieldInfo{L"author", StructFieldMetaInfo{&DocumentMetadata::Comment::author}},
        });
        mapper.AddCommonType<Containers::Sequence<DocumentMetadata::Comment>> ();
        mapper.AddCommonType <optional<Containers::Sequence<DocumentMetadata::Comment>>> ();

        mapper.AddClass<DocumentMetadata> ({
            ObjectVariantMapper::StructFieldInfo{L"tags", StructFieldMetaInfo{&DocumentMetadata::tags}},
            ObjectVariantMapper::StructFieldInfo{L"date", StructFieldMetaInfo{&DocumentMetadata::date}},
            ObjectVariantMapper::StructFieldInfo{L"location", StructFieldMetaInfo{&DocumentMetadata::location}},
            ObjectVariantMapper::StructFieldInfo{L"comment", StructFieldMetaInfo{&DocumentMetadata::comment}},
            ObjectVariantMapper::StructFieldInfo{L"title", StructFieldMetaInfo{&DocumentMetadata::title}},
            ObjectVariantMapper::StructFieldInfo{L"rating", StructFieldMetaInfo{&DocumentMetadata::rating}},
            ObjectVariantMapper::StructFieldInfo{L"album", StructFieldMetaInfo{&DocumentMetadata::album}},
        });
    }

    void DocumentMetadata::WriteToFileAsJSON (Containers::Mapping<String, DocumentMetadata> mds, String filePath)
    {
        using DataExchange::ObjectVariantMapper;

        ObjectVariantMapper mapper;
        DocumentMetadata::SupportVariantMapping (mapper);
        mapper.AddCommonType<Containers::Mapping<String, DocumentMetadata>> ();
       
        auto tagFile = std::wofstream (filePath.c_str ());
        DataExchange::Variant::JSON::Writer{}.Write (mapper.FromObject (mds), tagFile);
    }

    void DocumentMetadata::ReadFromJSONFile (Containers::Mapping<String, DocumentMetadata>* mds, String filePath)
    {
        using DataExchange::ObjectVariantMapper;

        ObjectVariantMapper mapper;
        DocumentMetadata::SupportVariantMapping (mapper);
        mapper.AddCommonType<Containers::Mapping<String, DocumentMetadata>> ();
        std::wifstream             fileScrapeInput = std::wifstream (filePath.c_str ());
        DataExchange::VariantValue xxx             = DataExchange::Variant::JSON::Reader{}.Read (fileScrapeInput);
        mapper.ToObject (xxx, mds);
    }
}


