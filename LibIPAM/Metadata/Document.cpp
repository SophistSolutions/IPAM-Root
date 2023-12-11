#include <fstream>
#include <iostream>

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

#include "Document.h"

using namespace Stroika::Foundation;
using Characters::String;

using namespace IPAM::LibIPAM::Metadata;

/*
 ********************************************************************************
 ************************ Metadata::Document::Comment ***************************
 ********************************************************************************
 */
String Document::Comment::ToString () const
{
    Characters::StringBuilder sb;
    sb += L"{";
    sb += L"comment: " + comment + L", ";
    sb += L"author: ";
    sb += (author.has_value ()) ? author.value ().As<wstring> ().c_str () : L"";
    sb += L"}";
    return sb.str ();
}

String Document::Comment::ToString (Containers::Sequence<Comment> comments)
{
    Characters::StringBuilder sb;
    sb += L"{";
    for (auto it : comments) {
        sb += it.ToString ();
    }
    sb += L"}";
    return sb.str ();
}

/*
     ********************************************************************************
     ******************************** Metadata::Document ****************************
     ********************************************************************************
     */
void Document::SupportVariantMapping (DataExchange::ObjectVariantMapper& mapper)
{
    using DataExchange::ObjectVariantMapper;
    using DataExchange::StructFieldMetaInfo;
    mapper.AddCommonType<Containers::Set<String>> ();

    mapper.AddClass<Document::Comment> ({
        ObjectVariantMapper::StructFieldInfo{L"comment", StructFieldMetaInfo{&Document::Comment::comment}},
        ObjectVariantMapper::StructFieldInfo{L"author", StructFieldMetaInfo{&Document::Comment::author}},
    });
    mapper.AddCommonType<Containers::Sequence<Document::Comment>> ();
    mapper.AddCommonType<optional<Containers::Sequence<Document::Comment>>> ();

    mapper.AddClass<Document> ({
        ObjectVariantMapper::StructFieldInfo{L"tags", StructFieldMetaInfo{&Document::tags}},
        ObjectVariantMapper::StructFieldInfo{L"date", StructFieldMetaInfo{&Document::date}},
        ObjectVariantMapper::StructFieldInfo{L"location", StructFieldMetaInfo{&Document::location}},
        ObjectVariantMapper::StructFieldInfo{L"comment", StructFieldMetaInfo{&Document::comment}},
        ObjectVariantMapper::StructFieldInfo{L"title", StructFieldMetaInfo{&Document::title}},
        ObjectVariantMapper::StructFieldInfo{L"rating", StructFieldMetaInfo{&Document::rating}},
        ObjectVariantMapper::StructFieldInfo{L"album", StructFieldMetaInfo{&Document::album}},
    });
}

void Document::WriteToFileAsJSON (Containers::Mapping<String, Document> mds, const std::filesystem::path& filePath)
{
    using DataExchange::ObjectVariantMapper;

    ObjectVariantMapper mapper;
    Document::SupportVariantMapping (mapper);
    mapper.AddCommonType<Containers::Mapping<String, Document>> ();

    DataExchange::Variant::JSON::Writer{}.Write (mapper.FromObject (mds), IO::FileSystem::FileOutputStream::New (filePath));
}

void Document::ReadFromJSONFile (Containers::Mapping<String, Document>* mds, const std::filesystem::path& filePath)
{
    using DataExchange::ObjectVariantMapper;

    ObjectVariantMapper mapper;
    Document::SupportVariantMapping (mapper);
    mapper.AddCommonType<Containers::Mapping<String, Document>> ();
    DataExchange::VariantValue xxx = DataExchange::Variant::JSON::Reader{}.Read (IO::FileSystem::FileInputStream::New (filePath));
    mapper.ToObject (xxx, mds);
}
