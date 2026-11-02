# IPAM Release Notes

## About

These release notes are a summary of major user-impactful changes - especially those to be aware of when upgrading.

## History


### 1.0d1 {2026-11-02} {[commits](../../commits/1.0d1)}

#### TLDR

- **First release.** IPAM's tools scrape photo metadata; the rest of its design (`Documentation/ReadMe.md` - documents, versions, sharing over IPFS) is not written yet
- **ImportMetadata** reads every photo under a directory (IPTC, XMP and EXIF, with Exiv2) and a digiKam database, merges them - digiKam's value wins where the user set one - and writes JSON. What it cannot read is left out of its photo's document, listed in `Problems.json` and counted on stderr - never dropped silently
- **TagStats** reads ImportMetadata's output and writes, for each tag, its photos and the tags seen with it
- Text and file names that are not ASCII work on Windows - in photos, in file names, and on the command line
- Builds on Windows (Visual Studio 2026) and Linux, on Stroika 3.0d25; CI builds and tests both on every push

#### Upgrade Notes

- None: this is the first release

#### Change Details

- ImportMetadata
  - Arguments: `-s DIR` (the photos), `-d FILE` (a digiKam database), `--include-digikam-only`, `-o DIR`; `--help`
  - Reads IPTC, XMP (its standard fields; ACDSee's, MicrosoftPhoto's; face-region names) and EXIF (date taken, with its zone if known; GPS; rating; description) - which key feeds which field is tabled above `ImageMetadataExtractor::Extract`
  - Text: XMP's read as UTF-8; IPTC's and EXIF's as UTF-8 if it is, else Windows-1252 - never in the machine's code page
  - Merging with digiKam: digiKam's date, location, rating and comment win over the file's; tags are both sets; digiKam's guess for a photo with no date (its file's time) is not taken as a date. Album roots in each form digiKam names one that says where it is (a path, a mount path, a network share), URL-decoded; one on a volume digiKam knows only by its UUID or label is reported
  - `Problems.json`: each value, photo or directory left out, and why. A directory Windows keeps for itself (hidden and system, as `$RECYCLE.BIN`) is skipped, and listed as a warning; stderr counts the rest by kind
  - A source directory or digiKam database it cannot read is an error, not an empty result
  - Windows: photos named outside the code page are read (Exiv2 patched - see Build System); arguments come whole (`wmain`)
  - Videos Exiv2 reads are listed, but with none of their own metadata ([issue 11](https://github.com/SophistSolutions/IPAM-Root/issues/11))
- TagStats
  - Arguments: `-i FILE` (ImportMetadata's `DocumentMetaData.json`), `-o FILE`; `--help`
  - Each tag's siblings are ordered by how often they appear with it
- LibIPAM
  - `Common::Geolocation`: ISO 6709 latitude, longitude and altitude, parsed and printed (a decimal fraction on the unit it follows)
  - `Metadata::Document`: one document's metadata - the data model is specified in `Document.h` - and its JSON
- Build System
  - One build root with Stroika (`make build-root`); `make default-configurations`; `make run-tests`; libIPAM linked through a generated `libIPAM.pc`, ahead of Stroika's libraries
  - Third-party components, built from source: expat 2.8.2; Exiv2 0.28.9, patched on Windows to open a file by its UTF-8 name - proposed to Exiv2 as [Exiv2 #9558](https://github.com/Exiv2/exiv2/pull/9558)
  - IPAM's own build images (`DockerBuildContainers/`): `sophistsolutionsinc/ipam-ubuntu-2604` and `ipam-windows-vs2k26`
  - CI (GitHub Actions): Linux and Windows - Debug, built and tested on every push; Release too, on `v1-Release`
- Tests (googletest): `Geolocation`; `Metadata` (Document's JSON round trip, and today's saved format); `ImportMetadataTool` (runs ImportMetadata on a small collection and digiKam database it makes, and checks what it writes)
- Documentation: `AGENTS.md` (for agents and people working in IPAM, on top of Stroika's), `TODO.md`
- Known limits, each in `TODO.md`
  - Windows: `-d` cannot name a database at a path that is not ASCII ([Stroika issue 1265](https://github.com/SophistSolutions/Stroika/issues/1265))
  - Linux: an argument that is not ASCII probably terminates ImportMetadata and TagStats ([Stroika issue 1266](https://github.com/SophistSolutions/Stroika/issues/1266))

#### Release-Validation

- Compilers Tested/Supported
  - g++ (Ubuntu 26.04's default)
  - MSVC 19.51 (Visual Studio 2026)
- OS/Platforms Tested/Supported
  - Windows 11 version 25H2 (build 26200) - Protagoras (laptop): Debug and Release
  - Windows, in IPAM's build image (docker, GitHub Actions): Debug and Release
  - Ubuntu 26.04, in IPAM's build image (docker, GitHub Actions): Debug and Release
- Built with Stroika 3.0d25
- Build Systems
  - [GitHub Actions](https://github.com/SophistSolutions/IPAM-Root/actions)
- Not yet validated
  - ImportMetadata and TagStats on a real photo collection and digiKam database (Sterling's, after this release)

----------------------------
