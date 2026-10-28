/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2026.  All rights reserved
 */

#include "Stroika/Frameworks/StroikaPreComp.h"

#include <cstdlib>
#include <fstream>
#include <iostream>

#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/Debug/Trace.h"
#include "Stroika/Foundation/Execution/CommandLine.h"
#include "Stroika/Foundation/IO/FileSystem/TemporaryFile.h"

#include "Stroika/Frameworks/Test/TestHarness.h"

#include "LibIPAM/Metadata/Document.h"

using std::filesystem::path;

using namespace Stroika::Foundation;
using namespace Stroika::Foundation::Characters;
using namespace Stroika::Foundation::Containers;
using namespace Stroika::Frameworks;

using namespace IPAM::LibIPAM::Metadata;

#if qHasFeature_GoogleTest
namespace {
    // Every field set, one document with few, and one with none - as ImportMetadata fills them from Exiv2 and digiKam
    Mapping<String, Document> MakeSample_ ()
    {
        Document full;
        full.tags     = Set<String>{"Fred"sv, "Statue of Liberty"sv, String{u"Zo\u00eb"}};
        full.date     = "1964-07-04T18:30:00Z"sv;
        full.location = "+40.6894-074.0447/"sv;
        full.comment  = Sequence<Document::Comment>{Document::Comment{"Our first trip to New York"sv, "Sterling"sv},
                                                    Document::Comment{"Fred took this one"sv}};
        full.title    = "Trip to New York"sv;
        full.rating   = 0.75;
        full.album    = "1960-1969/1964-07 New York"sv;

        Document few;
        few.tags   = Set<String>{"Fred"sv};
        few.rating = 0.01; // digiKam's one star

        return Mapping<String, Document>{{"P:/1960-1969/1964-07 New York/IMG_0001.jpg"sv, full},
                                         {"P:/1960-1969/1964-07 New York/IMG_0002.jpg"sv, few},
                                         {"P:/Scans/unknown.jpg"sv, Document{}}};
    }

    // MakeSample_ as WriteToFileAsJSON wrote it (but for the JSON escape \u00eb, as this file stays ASCII). Files on disk must
    // keep reading, so a change that breaks this test needs a plan for them - not just a new copy of this text.
    constexpr string_view kSavedFormat_ = R"({
    "P:/1960-1969/1964-07 New York/IMG_0001.jpg" : {
        "comment" : [
            {
                "comment" : "Our first trip to New York",
                "author" : "Sterling"
            },
            {
                "comment" : "Fred took this one"
            }
        ],
        "rating" : 0.75,
        "date" : "1964-07-04T18:30:00Z",
        "location" : "+40.6894-074.0447/",
        "album" : "1960-1969/1964-07 New York",
        "tags" : [
            "Fred",
            "Statue of Liberty",
            "Zo\u00eb"
        ],
        "title" : "Trip to New York"
    },
    "P:/1960-1969/1964-07 New York/IMG_0002.jpg" : {
        "tags" : [
            "Fred"
        ],
        "rating" : 0.01
    },
    "P:/Scans/unknown.jpg" : {
        "tags" : [
        ]
    }
})";

    // Document by document, so a failure names the one that differs (and EXPECT_EQ on whole Mappings has gtest print them as
    // raw bytes - they have no ToString () member - which aborts under ASan with MSVC)
    void ExpectSameDocuments_ (const Mapping<String, Document>& actual, const Mapping<String, Document>& expected)
    {
        EXPECT_EQ (actual.size (), expected.size ());
        for (const auto& kv : expected) {
            EXPECT_EQ (actual.Lookup (kv.fKey), kv.fValue) << kv.fKey;
        }
    }

    GTEST_TEST (Metadata, Document_RoundTrip_)
    {
        Mapping<String, Document>     sample = MakeSample_ ();
        IO::FileSystem::ScopedTmpFile tmpFile{"DocumentMetaData.json"};
        Document::WriteToFileAsJSON (sample, tmpFile);
        Mapping<String, Document> readBack;
        Document::ReadFromJSONFile (&readBack, tmpFile);
        ExpectSameDocuments_ (readBack, sample);
    }

    GTEST_TEST (Metadata, Document_ReadsSavedFormat_)
    {
        IO::FileSystem::ScopedTmpFile tmpFile{"DocumentMetaData.json"};
        std::ofstream{static_cast<path> (tmpFile), std::ios::binary} << kSavedFormat_;
        Mapping<String, Document> read;
        Document::ReadFromJSONFile (&read, tmpFile);
        ExpectSameDocuments_ (read, MakeSample_ ());
    }
}
#endif

int main (int argc, const char* argv[])
{
    Debug::TraceContextBumper ctx{"main", "argv={}"_f, Execution::CommandLine{argc, argv}};
    Test::Setup (argc, argv);
#if qHasFeature_GoogleTest
    return RUN_ALL_TESTS ();
#else
    cerr << "IPAM regression tests require building with google test feature" << endl;
#endif
}
