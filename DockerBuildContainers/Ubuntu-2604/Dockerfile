# Start with basic set of tools from Stroika build environment
ARG BASE_IMAGE=sophistsolutionsinc/stroika-buildvm-ubuntu2604-small:v3
FROM ${BASE_IMAGE}

# anytime running apt-get intall, good idea to make sure packages downloaded up to date
RUN apt-get update && apt-get upgrade -y

# the distro's default g++ - what the top Makefile's configurations use. Tools IPAM needs beyond Stroika's (eg for
# IPFS) get installed here too.
RUN apt-get install -y g++

RUN rm -f /Getting-Started-With-Stroika.md
ADD Shared-Files/Getting-Started.md /
RUN sed -i -e 's/Getting-Started-With-Stroika/Getting-Started/g' $HOME/.bash_profile

### NOTE TO RUN
###     docker run -v /Sandbox:/Sandbox -it sophistsolutionsinc/ipam-ubuntu-2604
###     docker run -it sophistsolutionsinc/ipam-ubuntu-2604
