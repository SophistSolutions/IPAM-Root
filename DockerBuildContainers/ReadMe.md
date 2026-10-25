# Docker Build Containers

There is no need to use this, but it can provide a quickstart to building IPAM, and it is what CI builds in.

Just create one or more of these docker images, and run them as in:

- `make build-images` (or one: `make IMAGES=sophistsolutionsinc/ipam-ubuntu-2604 build-images`)
- `docker run -it sophistsolutionsinc/ipam-ubuntu-2604`
  - then inside the docker container find Getting-Started.md, and follow its instructions to build (basically clone
    IPAM-Root and type make)

The image is Ubuntu 26.04, on the matching Stroika build image (`stroika-buildvm-ubuntu2604-small`), plus the distro's
default g++. Tools IPAM needs beyond Stroika's - for IPFS, say - go in its Dockerfile.

CI builds in it (`.github/workflows/build-N-test-Matrix.json`), and the build-Dev-Docker-Containers workflow (run by
hand) builds and publishes it, tagged `latest` and `v1`, from `.github/workflows/build-Dev-Docker-Containers-Matrix.json`.
By hand: `make build-images tag-images`, then `docker login` (as an account that can push to sophistsolutionsinc) and
`make push-images`.
