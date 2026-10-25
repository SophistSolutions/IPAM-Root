# TODO

**Note:** This file is for **short-term** TODOs only - a scratch list to hand context between work sessions and machines,
and to brief agents (human or AI) picking up a task. It is not a roadmap, issue tracker, or design doc: longer-range work
goes in https://github.com/SophistSolutions/IPAM-Root/issues, and the design in `Documentation/ReadMe.md`. Prune entries
as they are done.

## Open

- Link Exiv2 and expat through the `.pc` files their builds already install (`exiv2.pc`, `expat.pc`), in place of the
  hand-written `-lexiv2 -lexpat -lz` / `exiv2.lib libexpat.lib psapi.lib` lines in PS1's and the Geolocation test's
  Makefiles - or wrap them in libIPAM.pc.

- **IntelliSense takes `VERSION` for `<version>` - waits on Stroika
  [#1252](https://github.com/SophistSolutions/Stroika/issues/1252)**. IPAM includes its own headers from its root
  (`"LibIPAM/..."`), so the local, untracked `.vscode/c_cpp_properties.json` keeps `myDefaultIncludePath:
  ["${workspaceFolder}"]` - which also puts `VERSION` ahead of the system `<version>` (the compile is fine: there the
  system directories come first). Once a Stroika upgrade brings #1252's fix, set IPAM up as its upgrade note says, and
  check IntelliSense opens a `Tools/PS1` source with no errors.

- Small build leftovers (found 2026-10-23):
  - `make run-tests` runs `Geolocation.exe` (Tests/Makefile:12) - no such file on UNIX - and needs `CONFIGURATION=`.
  - expat's and Exiv2's `make all` with no `CONFIGURATION=` calls `$(StroikaRoot)GetConfigurations`, which does not exist
    (ThirdPartyComponents/expat/Makefile:54, ThirdPartyComponents/Exiv2/Makefile:82) - `Build/Scripts/GetConfigurations`.
  - `.vscode/launch.json` launches `Builds/<CONFIG>/IPAM/IPAM` (:11, :34, :41), Skel's placeholder: there is no such
    program.
  - `Tools/PS2/Sources/TagInfo.{h,cpp}` are built by nothing, and use a `DocumentMetadata` type that no longer exists:
    delete them, or fold what is useful into `Main.cpp`.
