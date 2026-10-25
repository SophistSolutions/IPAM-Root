export TOP_ROOT=$(abspath ../)/
StroikaRoot=$(TOP_ROOT)ThirdPartyComponents/Stroika/StroikaRoot/

.DEFAULT_GOAL := help

DETECTED_HOST_OS=$(shell $(StroikaRoot)Build/Scripts/DetectedHostOS)


help:
	@echo "make build-images to create the docker images on your computer"
	@echo "to run the image, use:"
	@echo "    docker run -it sophistsolutionsinc/ipam-ubuntu-2604"
	@echo "n.b. use --security-opt seccomp=unconfined to run debugger (ptrace/address randomization disable)"
	@echo "and first thing when you run docker image, cat /Getting-Started.md"
	@echo "make tag-images, then make push-images (after docker login) to publish them, as the build-Dev-Docker-Containers workflow does"


DOCKER_BUILD_EXTRAS_?=\

DEFAULT_IMAGES_?=

## NOTE: Since these can be slow/big, the IMAGES arg can be given on the commandline to override this
ifeq (${DETECTED_HOST_OS},Linux)
DEFAULT_IMAGES_+= sophistsolutionsinc/ipam-ubuntu-2604
endif

IMAGES?=${DEFAULT_IMAGES_}

build-images:
	${MAKE} --no-print-directory ${IMAGES}


sophistsolutionsinc/ipam-ubuntu-2604:
	docker build -f Ubuntu-2604/Dockerfile -t $@ ${DOCKER_BUILD_EXTRAS_} .


tag-images:
	for image in ${IMAGES}; do \
		docker tag $${image} $${image}:v1; \
	done


push-images:
	#if you get errors doing this (permission denied) make sure you ran docker login (as an account that can push to sophistsolutionsinc)
	for image in ${IMAGES}; do \
		docker push $${image}:latest; \
		docker push $${image}:v1; \
	done
