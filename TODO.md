# TODO

**Note:** This file is for **short-term** TODOs only - a scratch list to hand context between work sessions and machines,
and to brief agents (human or AI) picking up a task. It is not a roadmap, issue tracker, or design doc: longer-range work
goes in https://github.com/SophistSolutions/IPAM-Root/issues, and the design in `Documentation/ReadMe.md`. Prune entries
as they are done.

## Open

- **Windows CI's `SHLVL=1` - waits on Stroika [#1258](https://github.com/SophistSolutions/Stroika/issues/1258)**.
  `build-N-test.yml` starts the Windows build container with `--env SHLVL=1`, or Stroika's OpenSSL download fails there
  (its Makefile sends `WebGet` to `$HOME`). Once a Stroika upgrade brings #1258's fix, drop it, and check Windows CI
  still passes.

- **IntelliSense takes `VERSION` for `<version>` - waits on Stroika
  [#1252](https://github.com/SophistSolutions/Stroika/issues/1252)**. IPAM includes its own headers from its root
  (`"LibIPAM/..."`), so the local, untracked `.vscode/c_cpp_properties.json` keeps `myDefaultIncludePath:
  ["${workspaceFolder}"]` - which also puts `VERSION` ahead of the system `<version>` (the compile is fine: there the
  system directories come first). Once a Stroika upgrade brings #1252's fix, set IPAM up as its upgrade note says, and
  check IntelliSense opens a `Tools/PS1` source with no errors.
