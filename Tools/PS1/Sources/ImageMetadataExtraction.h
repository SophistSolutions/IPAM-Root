#pragma once

#include <filesystem>

#include "Stroika/Foundation/Characters/String.h"
#include "Stroika/Foundation/Containers/Mapping.h"
#include "Stroika/Foundation/Containers/MultiSet.h"

#include "LibIPAM/Metadata/Document.h"

namespace Metadata {

    using namespace IPAM::LibIPAM::Metadata;

    using Characters::String;
    class ImageMetadataExtractor {
    public:
        ImageMetadataExtractor ();

        // extract relevant metadata from image file
        Metadata::Document Extract (const std::filesystem::path& pictFile);

        // extract relevant metadata from all image files in the directory (and subdirectories)
        Containers::Mapping<String, Metadata::Document> ExtractAll (const std::filesystem::path& topDir);

        // find all the file extensions in a directory (and subdirectories) and optionally
        // attempt to extract image metadata from first file of each extension found
        Containers::MultiSet<String> TallyExtensions (const std::filesystem::path& topDir, String outputDirectoryForSampleFiles);
    };

}
