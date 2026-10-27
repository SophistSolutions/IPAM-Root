# AGENTS.md

This file provides guidance to agents (and people) working in this repository.

@ThirdPartyComponents/Stroika/StroikaRoot/AGENTS.md

IPAM is an application built on Stroika (scaffolded with Stroika's `Skel`, in 2022). Stroika's AGENTS.md, imported
above, applies here - agents that do not follow `@` imports: read `ThirdPartyComponents/Stroika/StroikaRoot/AGENTS.md`
first. Its build system, C++ conventions, Commits, Issues and "Working with LGP" sections are IPAM's too. These parts of
it do not apply: its regression-test layout (`Tests/NN`, renumbering), its CI matrix and supported-platforms tables (IPAM's
CI is one job - see Layout), its release milestones and `Release-Notes.md`, `UPGRADE NOTE:`s in commit messages (nothing is built on
IPAM), and its "Downstream projects" list (IPAM is on it). Where the two files disagree, this one wins.

## What this is

IPAM, the "Interplanetary Album Manager": tools to organize photo albums (and other documents) and share them over IPFS
without handing control to a third party. It is early-stage. `Documentation/ReadMe.md` is the design (documents,
versions, sections, tags, an API-Server that talks to IPFS), and most of it is not written yet: there is no IPFS code.
What works today is scraping photo metadata. Version in `VERSION`; one branch, `v1-Dev`; no release notes.

## Layout

- `LibIPAM/` - the static library `Builds/<CONFIG>/libIPAM.lib` (`.a` on UNIX), namespace `IPAM::LibIPAM`:
  - `Common/Geolocation` - ISO 6709 latitude, longitude and altitude, parsed and printed.
  - `Metadata/Document` - one document's metadata (tags, date, location, comments, title, rating, album), its
    `ObjectVariantMapper` mapping, and JSON file read/write. The long comment in `Document.h` specifies the data model.
- `Tools/PS1/` - the scraper: reads every image under a directory with Exiv2, and a digiKam SQLite database, merges
  them, and writes JSON (`FileScrape.json`, `DigikamScrape.json`, `ExtenstionTally.json`, `DocumentMetaData.json`) to
  `-o <dir>`. Its inputs are hard-wired in `Main.cpp` (`P:/...`, `c:/Digikam/digikam4.db`).
- `Tools/PS2/` - reads PS1's `DocumentMetaData.json` and writes tag co-occurrence counts
  (`DocumentMetaDataTagInfo.json`); both paths are hard-wired to `c:\ssw\mdResults\`.
- `API-Server/` - a stub `main` that does nothing yet.
- `Tests/Geolocation/` - the only test: googletest, over `LibIPAM/Common/Geolocation`.
- `ThirdPartyComponents/` - Stroika (git submodule, at `StroikaRoot`), plus `expat` and `Exiv2`, which only PS1 links.
- `Workspaces/` - the VS Code workspace and a Visual Studio 2022 solution (Visual Studio is for debugging; make builds).
- `DockerBuildContainers/` - IPAM's build images (`sophistsolutionsinc/ipam-ubuntu-2604` and `ipam-windows-vs2k26`, on
  Stroika's `stroika-buildvm-ubuntu2604-small` and `stroika-buildvm-windows-msys-vs2k26`): tools IPAM needs beyond
  Stroika's go in their Dockerfiles. The `build-Dev-Docker-Containers` workflow (run by hand) publishes them, from its
  `-Matrix.json`.
- `.github/workflows/build-N-test.yml` - CI on every push, one job per entry of `build-N-test-Matrix.json` that the branch
  selects (`run_on_branch`, as in Stroika's): today Linux Debug and Windows Debug, each in IPAM's image, building
  everything and running the test. No macOS. Results: https://github.com/SophistSolutions/IPAM-Root/actions (public, so
  readable with no token, as Stroika's are).

## Build

Stroika's build section applies. The IPAM-specific parts:
- One build root: the first `make` (`make build-root`) points StroikaRoot's `Builds`, `ConfigurationFiles` and
  `IntermediateFiles` at IPAM's own, so Stroika and IPAM build into the same `Builds/<CONFIG>/`.
- `make default-configurations` makes IPAM's list, from the top-level `Makefile`: on Windows Debug, Release,
  Release-Logging and x86/x86_64 variants (Debug and Release are not built by default - name them); on Linux and macOS
  Debug and Release, with the default compiler. All get `--ATLMFC no`. IPAM takes Stroika's configure defaults
  (compiler, C++ standard, ...) - a flag here needs an IPAM reason (LGP, 2026-10-23).
- `make CONFIGURATION=Debug all -j8` builds, in order: the Stroika libraries (with `QUICK_BUILD=1`, so libraries already
  there are not rebuilt), expat, Exiv2, `LibIPAM`, `Tools`, `Tests`, `API-Server`. `make all` alone builds every
  default configuration - slow.
- Output: `Builds/<CONFIG>/libIPAM.lib` and `Builds/<CONFIG>/IPAM/<project path>/<name>.exe` (eg
  `Builds/Debug/IPAM/Tools/PS1/PS1.exe`); objects under `IntermediateFiles/<CONFIG>/IPAM/`.
- **A header edit rebuilds nothing, in IPAM as in Stroika.** After editing an IPAM `.h`/`.inl`,
  `rm -rf IntermediateFiles/<CONFIG>/IPAM` before trusting a build. After moving the Stroika submodule, see "Upgrading
  Stroika".
- A new `.cpp` is listed by hand: in its directory's Makefile `Objs` (a new `LibIPAM` directory also in `SubDirs` in
  `LibIPAM/Makefile`), and in its `.vcxproj` and `.vcxproj.filters`.
- The executables link libIPAM through `Builds/<CONFIG>/lib/pkgconfig/libIPAM.pc`, which `LibIPAM/Makefile` writes:
  named as `DEFAULT_LINK_LINE`'s 2nd argument, it goes ahead of Stroika's libraries, as GNU ld needs. A library passed in
  the 4th argument (`LDFLAGS`) comes after them - fine with MSVC, undefined references on Linux. So libraries go in the
  2nd argument by their `.pc` names - PS1's is `libIPAM exiv2` (`exiv2.pc` brings expat and zlib) - and `LDFLAGS` keeps
  only system libraries a `.pc` misses (PS1's `psapi.lib`).
- expat and Exiv2: each Makefile pins `VERSION`, and names the page to check for newer releases. The zip is cached in
  `ThirdPartyComponents/Origs-Cache/`, then extracted and patched (`Patches/`) into
  `IntermediateFiles/<CONFIG>/ThirdPartyComponents/<name>/`, built with cmake, and installed into
  `Builds/<CONFIG>/{include,lib}`. The logs are in that directory: `CONFIGURE-OUT.txt`, `BUILD-OUT.txt`, `INSTALL.OUT`,
  `PATCHING.OUT`. Once its library exists, nothing rebuilds it - not a new `VERSION`, not a new patch: run
  `make CONFIGURATION=X clobber` in `ThirdPartyComponents/<name>` first (and in `Exiv2` after a new expat). Exiv2 builds
  against expat and Stroika's zlib, and all three must agree on the MSVC runtime and library names - the comments in
  `Exiv2/Makefile` say how.
- Formatting: `make format-code` (Stroika's clang-format rules; `.clang-format` is a link to Stroika's).

## Run and check

- A check is the build (with its warnings, as in Stroika), then the test, `Builds/<CONFIG>/IPAM/Tests/Geolocation/
  Geolocation` (`.exe` on Windows): count its `[  PASSED  ]` line as Stroika's AGENTS.md says. `make CONFIGURATION=X
  run-tests` runs it - with no `CONFIGURATION=`, each default configuration's in turn. Unlike Stroika's, it builds nothing.
- PS1 and PS2 need the photo collection and digiKam database at their hard-wired paths, so usually they can only be
  built. Say what you ran, and what you could not.
- DbgTrace goes to a trace file in Debug and Release-Logging builds.
- `git grep` searches IPAM's own files: it skips the Stroika submodule and everything untracked - `Builds/`,
  `IntermediateFiles/` (where Exiv2's and expat's sources are extracted). A `grep -r` searches all of those too.

## Upgrading Stroika

The submodule follows Stroika's releases (`v3-Release`), or `v3-Dev` between them for a fix IPAM needs. To upgrade:
1. `git status` - note what is local, the submodule pointer especially. `git -C ThirdPartyComponents/Stroika/StroikaRoot
   describe --tags` names the version you start from.
2. Move the submodule, from IPAM's root: `make latest-submodules` (a checkout and pull of `v3-Release`;
   `STROIKA_COMMIT=v3-Dev`, or a tag, for another).
3. Read Stroika's `Release-Notes.md`, "Upgrade Notes (X to Y)", for every release passed; `git grep` IPAM for each note.
4. Compare IPAM's Skel-derived files with Stroika's `Build/Shared/Skel-Templates/` (`Common/` and `Basic/`): the top
   `Makefile`, `ThirdPartyComponents/Makefile` and `ThirdPartyComponents/Stroika/Makefile`, `.vscode/`, `Workspaces/`,
   and each project's `Makefile` and `Sources/Makefile` (from `Basic/MyApp/`). IPAM's copies carry its own changes
   (several projects, the expat/Exiv2 link lines, its configure flags): merge, do not copy.
5. Check `ThirdPartyComponents/expat` and `Exiv2` against Stroika's own cmake-built components
   (`Build/Lib/Make/Makefile-CMake-Common.mk`, `ThirdPartyComponents/libxml2/Makefile`) and its zlib library names -
   Stroika changes have broken them before (a stale zlib library name, 2c7ee80).
6. Rebuild from clean - `make distclean`, then `make CONFIGURATION=Debug all -j8` (it makes the default configurations
   again): an incremental build links the old Stroika libraries (Stroika's `Documentation/Upgrading.md` says why).
7. Fix the errors, and the deprecation warnings too: deprecated Stroika names go away at v3.0a1. A renamed macro cannot
   warn - search for each old name the notes list.
8. Build Debug and Release with MSVC, and with g++ on a Linux box; run the test; run PS1 and PS2 where their data is.
9. Report every change IPAM needed that the upgrade notes do not mention, and every note that was wrong or unclear, so
   Stroika's Release-Notes can be fixed.
10. The commit's headline names the Stroika version ("Stroika 3.0d25").
