# TODO

**Note:** This file is for **short-term** TODOs only - a scratch list to hand context between work sessions and machines,
and to brief agents (human or AI) picking up a task. It is not a roadmap, issue tracker, or design doc: longer-range work
goes in https://github.com/SophistSolutions/IPAM-Root/issues, and the design in `Documentation/ReadMe.md`. Prune entries
as they are done.

## Open

- Add libIPAM.pc file support and use it in the executables (maybe .pc files for the support libraries too, or wrap them
  in libIPAM).

- **IntelliSense takes `VERSION` for `<version>` - waits on Stroika
  [#1252](https://github.com/SophistSolutions/Stroika/issues/1252)**. IPAM includes its own headers from its root
  (`"LibIPAM/..."`), so the local, untracked `.vscode/c_cpp_properties.json` keeps `myDefaultIncludePath:
  ["${workspaceFolder}"]` - which also puts `VERSION` ahead of the system `<version>` (the compile is fine: there the
  system directories come first). Once a Stroika upgrade brings #1252's fix, set IPAM up as its upgrade note says, and
  check IntelliSense opens a `Tools/PS1` source with no errors.

- **PS1 writes the unmerged scrape** (found 2026-10-23): `Main.cpp` builds `masterList` - the file scrape with the
  digiKam data merged in (Tools/PS1/Sources/Main.cpp:200) - but writes `fileScrape` to `DocumentMetaData.json` (:219),
  so the merge is thrown away, and PS2 reads the unmerged data.

- **Geolocation test assertions that cannot fail** (found 2026-10-23): `EXPECT_TRUE ((abs (b.seconds ()) - 3.6348) <
  0.00001)` (Tests/Geolocation/Sources/Main.cpp:37, :74, and the `- 0` / `- .23` lines near them) is true for any value
  below 3.6348 - meant `abs (b.seconds () - 3.6348)`. And three ISO 6709 examples are commented out, failing on zero
  padding (`+00-025/` prints `+00-25/`; :112-128).

- Small build leftovers (found 2026-10-23):
  - `make run-tests` runs `Geolocation.exe` (Tests/Makefile:12) - no such file on UNIX - and needs `CONFIGURATION=`.
  - expat's and Exiv2's `make all` with no `CONFIGURATION=` calls `$(StroikaRoot)GetConfigurations`, which does not exist
    (ThirdPartyComponents/expat/Makefile:54, ThirdPartyComponents/Exiv2/Makefile:82) - `Build/Scripts/GetConfigurations`.
  - `.vscode/launch.json` launches `Builds/<CONFIG>/IPAM/IPAM` (:11, :34, :41), Skel's placeholder: there is no such
    program.
  - `Tools/PS2/Sources/TagInfo.{h,cpp}` are built by nothing, and use a `DocumentMetadata` type that no longer exists:
    delete them, or fold what is useful into `Main.cpp`.
