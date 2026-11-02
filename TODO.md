# TODO

**Note:** This file is for **short-term** TODOs only - a scratch list to hand context between work sessions and machines,
and to brief agents (human or AI) picking up a task. It is not a roadmap, issue tracker, or design doc: longer-range work
goes in https://github.com/SophistSolutions/IPAM-Root/issues, and the design in `Documentation/ReadMe.md`. Prune entries
as they are done.

## Open

- **Exiv2's Windows file-name patch - waits on Exiv2 [#9558](https://github.com/Exiv2/exiv2/pull/9558)** (or main's
  #3117, in a release). `ThirdPartyComponents/Exiv2` patches 0.28.9 so that `FileIo` opens a UTF-8 name by its wide
  name; #9558 proposes the same for 0.28.x. Once a release has a fix, move to it - the patch is applied to 0.28.9
  only, and `Tests/ImportMetadataTool` fails on a version without a fix.

- **Windows CI's `SHLVL=1` - waits on Stroika [#1258](https://github.com/SophistSolutions/Stroika/issues/1258)**.
  `build-N-test.yml` starts the Windows build container with `--env SHLVL=1`, or Stroika's OpenSSL download fails there
  (its Makefile sends `WebGet` to `$HOME`). Once a Stroika upgrade brings #1258's fix, drop it, and check Windows CI
  still passes.

- **A JSON Schema for `DocumentMetaData.json` - waits on Stroika
  [#1140](https://github.com/SophistSolutions/Stroika/issues/1140)** (JSON Schema validation, in its 3.0d26 milestone).
  Once a Stroika upgrade brings it, write the schema (`Documentation/Schemas/DocumentMetaData.schema.json`, say), and have
  `Tests/Metadata` check its saved-format text, and a file it has just written, against it.

- **IntelliSense takes `VERSION` for `<version>` - waits on Stroika
  [#1252](https://github.com/SophistSolutions/Stroika/issues/1252)**. IPAM includes its own headers from its root
  (`"LibIPAM/..."`), so the local, untracked `.vscode/c_cpp_properties.json` keeps `myDefaultIncludePath:
  ["${workspaceFolder}"]` - which also puts `VERSION` ahead of the system `<version>` (the compile is fine: there the
  system directories come first). Once a Stroika upgrade brings #1252's fix, set IPAM up as its upgrade note says, and
  check IntelliSense opens a `Tools/ImportMetadata` source with no errors.

- **A file's creation time - waits on Stroika [#1264](https://github.com/SophistSolutions/Stroika/issues/1264)**
  (`IO::FileSystem::Ptr::GetCreationTime`, in its 3.0d26 milestone). `Tools/ImportMetadata/Sources/Digikam.cpp`'s
  `GetCreationTime_` calls Stroika's from its 3.0d26 release, and carries its own code until then. Once a Stroika upgrade
  brings it, delete that code, its `#include`s and `qIPAM_StroikaHasGetCreationTime_`.

- **ImportMetadata's `-d` naming a path that is not ASCII, on Windows - waits on Stroika
  [#1265](https://github.com/SophistSolutions/Stroika/issues/1265)** (in its 3.0d26 milestone). Stroika's SQLite opens
  a database by its name in the code page, not UTF-8, so it cannot open one there. Once a Stroika upgrade brings the fix,
  `Tests/ImportMetadataTool`'s `ArgumentsOutsideTheCodePage_` puts its database under the `Ω` directory too.

- **Arguments that are not ASCII, on Linux - waits on Stroika
  [#1266](https://github.com/SophistSolutions/Stroika/issues/1266)** (in its 3.0d26 milestone). Stroika's `CommandLine`
  decodes a narrow argument in the C++ locale - `"C"`, which cannot decode UTF-8 - so ImportMetadata and TagStats
  probably terminate on one there. Once a Stroika upgrade brings the fix, `ArgumentsOutsideTheCodePage_` runs on Linux
  too (it is Windows-only).

## Watching

Not work items: checks an agent runs (how often), raising with LGP only what one finds.

- **WATCH: Exiv2 [#9558](https://github.com/Exiv2/exiv2/pull/9558), our Windows file-name PR - check once a day** (LGP
  2026-11-01): `gh pr view 9558 --repo Exiv2/exiv2 --json state,reviews,comments,statusCheckRollup`. Raise a review, a
  question, a failed check, or its merge. Last checked 2026-11-01: open, no reviews; only its Mergify, GitGuardian and
  Summary checks have run - its build workflows wait on a maintainer. Once a release has it, the "Exiv2's Windows
  file-name patch" entry above says what to do; then drop this.
