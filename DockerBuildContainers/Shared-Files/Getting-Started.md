# Quick-Start Building IPAM

```bash
mkdir Sandbox && cd Sandbox
git clone --recurse-submodules https://github.com/SophistSolutions/IPAM-Root.git
cd IPAM-Root
make CONFIGURATION=Debug all -j10
make CONFIGURATION=Debug run-tests
```

`make all -j10` (no `CONFIGURATION=`) builds all defined (default) configurations - slow.

Alternatively, run

```bash
make default-configurations
```

- review the created configurations in `ConfigurationFiles` - perhaps making more, or different ones.

then

```bash
make CONFIGURATION=YOUR-SELECTED-CONFIGURATION all -j10
```
