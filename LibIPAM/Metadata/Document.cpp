#include "Stroika/Frameworks/StroikaPreComp.h"

#include <fstream>
#include <iostream>

#include "Stroika/Foundation/Characters/String.h"
#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/Containers/Mapping.h"
#include "Stroika/Foundation/Containers/MultiSet.h"
#include "Stroika/Foundation/Containers/Set.h"
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
String Document::ToString () const
{
    StringBuilder sb;
    sb << "{"sv;
    sb << "tags: "sv << tags << ", "sv;
    sb << "date: "sv << date << ", "sv;
    sb << "location: "sv << location << ", "sv;
    sb << "comment: "sv << comment << ", "sv;
    sb << "title: "sv << title << ", "sv;
    sb << "rating: "sv << rating << ", "sv;
    sb << "album: "sv << album;
    sb << "}"sv;
    return sb;
}

String Document::FormatDate (const Time::DateTime& d)
{
    // not d.AsUTC () when the zone is unknown: that takes the local time for UTC
    return (d.GetTimezone () ? d.AsUTC () : d).Format (Time::DateTime::kISO8601Format);
}

void Document::SupportVariantMapping (DataExchange::ObjectVariantMapper& mapper)
{
    mapper.AddCommonType<Containers::Set<String>> ();

    mapper.AddClass<Document::Comment> ({
        {"comment"sv, &Document::Comment::comment},
        {"author"sv, &Document::Comment::author},
    });
    mapper.AddCommonType<Containers::Sequence<Document::Comment>> ();
    mapper.AddCommonType<optional<Containers::Sequence<Document::Comment>>> ();

    mapper.AddClass<Document> ({
        {"tags"sv, &Document::tags},
        {"date"sv, &Document::date},
        {"location"sv, &Document::location},
        {"comment"sv, &Document::comment},
        {"title"sv, &Document::title},
        {"rating"sv, &Document::rating},
        {"album"sv, &Document::album},
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
