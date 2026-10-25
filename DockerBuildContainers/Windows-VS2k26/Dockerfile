# Start with basic set of tools from Stroika build environment
ARG BASE_IMAGE=sophistsolutionsinc/stroika-buildvm-windows-msys-vs2k26:v3
FROM ${BASE_IMAGE}

SHELL ["cmd", "/S", "/C"]

# Tools IPAM needs beyond Stroika's (eg for IPFS) get installed here.

## hardwired location from sophistsolutionsinc/stroika-buildvm-windows-msys-vs2k26 dockerfile - find better way to compute!
RUN rm -f c:/msys64/home/Getting-Started-With-Stroika.md
ADD Shared-Files/Getting-Started.md c:/msys64/home/
RUN sed -i -e 's/Getting-Started-With-Stroika/Getting-Started/g' c:/msys64/home/.bash_profile

### NOTE TO RUN
###     docker run -v c:\Sandbox:c:\Sandbox -it sophistsolutionsinc/ipam-windows-vs2k26
###     docker run -it sophistsolutionsinc/ipam-windows-vs2k26
