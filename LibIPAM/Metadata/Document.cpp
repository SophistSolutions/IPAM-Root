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
using namespace Stroika::Foundation::Characters;
using namespace Stroika::Foundation::DataExchange;

using namespace IPAM::LibIPAM::Metadata;

/*
 ********************************************************************************
 ************************ Metadata::Document::Comment ***************************
 ********************************************************************************
 */
String Document::Comment::ToString () const
{
    StringBuilder sb;
    sb << "{"sv;
    sb << "comment: "sv + comment + L", "sv;
    sb << "author: "sv << author;
    sb << "}"sv;
    return sb;
}

String Document::Comment::ToString (Containers::Sequence<Comment> comments)
{
    StringBuilder sb;
    sb << "{"sv;
    for (auto it : comments) {
        sb << it.ToString () << ", "sv;
    }
    sb << "}"sv;
    return sb;
}

/*
 ********************************************************************************
 ******************************** Metadata::Document ****************************
 ********************************************************************************
 */
void Document::SupportVariantMapping (DataExchange::ObjectVariantMapper& mapper)
{
    mapper.AddCommonType<Containers::Set<String>> ();

    mapper.AddClass<Document::Comment> ({
        ObjectVariantMapper::StructFieldInfo{"comment"sv, StructFieldMetaInfo{&Document::Comment::comment}},
        ObjectVariantMapper::StructFieldInfo{"author"sv, StructFieldMetaInfo{&Document::Comment::author}},
    });
    mapper.AddCommonType<Containers::Sequence<Document::Comment>> ();
    mapper.AddCommonType<optional<Containers::Sequence<Document::Comment>>> ();

    mapper.AddClass<Document> ({
        ObjectVariantMapper::StructFieldInfo{"tags"sv, StructFieldMetaInfo{&Document::tags}},
        ObjectVariantMapper::StructFieldInfo{"date"sv, StructFieldMetaInfo{&Document::date}},
        ObjectVariantMapper::StructFieldInfo{"location"sv, StructFieldMetaInfo{&Document::location}},
        ObjectVariantMapper::StructFieldInfo{"comment"sv, StructFieldMetaInfo{&Document::comment}},
        ObjectVariantMapper::StructFieldInfo{"title"sv, StructFieldMetaInfo{&Document::title}},
        ObjectVariantMapper::StructFieldInfo{"rating"sv, StructFieldMetaInfo{&Document::rating}},
        ObjectVariantMapper::StructFieldInfo{"album"sv, StructFieldMetaInfo{&Document::album}},
    });
}

void Document::WriteToFileAsJSON (Containers::Mapping<String, Document> mds, const std::filesystem::path& filePath)
{
    ObjectVariantMapper mapper;
    Document::SupportVariantMapping (mapper);
    mapper.AddCommonType<Containers::Mapping<String, Document>> ();
    Variant::JSON::Writer{}.Write (mapper.FromObject (mds), IO::FileSystem::FileOutputStream::New (filePath));
}

void Document::ReadFromJSONFile (Containers::Mapping<String, Document>* mds, const std::filesystem::path& filePath)
{
    ObjectVariantMapper mapper;
    Document::SupportVariantMapping (mapper);
    mapper.AddCommonType<Containers::Mapping<String, Document>> ();
    VariantValue xxx = Variant::JSON::Reader{}.Read (IO::FileSystem::FileInputStream::New (filePath));
    mapper.ToObject (xxx, mds);
}
