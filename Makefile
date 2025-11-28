# EGM Emulator Makefile
# Based on nCompass Makefile patterns

# this can be overridden via "make CFG=Debug"
export CFG ?= Release

# this should be inherited from "./build-with-docker.sh"
DOCKER_SYSTEM_TARGET ?= yocto

# set SHELL to bash to avoid inconsistencies in /bin/sh implementations
SHELL := bash

.PHONY: default
default:
	$(MAKE) clean
	$(MAKE) sentinel

# make sure commands build in the right order
.NOTPARALLEL:

.PHONY: clean
clean:
	# remove artifacts we create
	rm -rf \
		'${ROOTFS}' \
		./build \
		./Release/*.o

	# invoke sub-Makefile to remove project artifacts
	$(MAKE) -f EGMEmulator.mak clean

ROOTFS := /tmp/sentinelfs
SENTINEL_HOME := $(ROOTFS)/opt/ncompass
DIST := build/dist

# Docker for Windows interacts with "mksquashfs" in an odd way that often results in an invalid squashfs file
# so, to counteract that, we "mksquashfs" to this other directory, then move the final result to $DIST/
TEMP_SQUASHFS_DIR := /tmp/ncompass-sentinel-squashfs

# a few defaults that may not be set elsewhere
OBJCOPY ?= objcopy

.PHONY: sentinel
sentinel: prepare
	echo Building ${BUILD_NUMBER}
	cat /etc/hostname

	# Build the EGM Emulator
	$(MAKE) -f EGMEmulator.mak

	# sentinel binary
	mkdir -p ${SENTINEL_HOME}
	bash ./get-firmware-version.sh > ${SENTINEL_HOME}/version.txt
	mkdir -p ${SENTINEL_HOME}/bin
	cp Release/EGMEmulator ${SENTINEL_HOME}/bin/Sentinel
	${OBJCOPY} --only-keep-debug Release/EGMEmulator ${DIST}/Sentinel.debug
	chmod +x ${SENTINEL_HOME}/bin/Sentinel

	# Create version file
	bash ./get-version.sh > ${SENTINEL_HOME}/bin/version

	# debug.txt (logs directory)
	mkdir -p ${SENTINEL_HOME}/logs

ifeq ($(DOCKER_SYSTEM_TARGET), yocto)
	# init script for Zeus OS
	mkdir -p ${ROOTFS}/etc/init.d
	cp --no-preserve=mode Scripts/sentinel.sh ${ROOTFS}/etc/init.d
	chmod +x ${ROOTFS}/etc/init.d/sentinel.sh
	mkdir -p ${ROOTFS}/etc/rc5.d
	cd ${ROOTFS}/etc/rc5.d && ln -sf ../init.d/sentinel.sh S50sentinel
endif

	# scripts
	cp --no-preserve=mode Scripts/killchrome.sh ${SENTINEL_HOME}/bin/
	chmod +x ${SENTINEL_HOME}/bin/killchrome.sh
	cp --no-preserve=mode Scripts/launchchrome.sh ${SENTINEL_HOME}/bin/
	chmod +x ${SENTINEL_HOME}/bin/launchchrome.sh

	# GUI media files
	mkdir -p ${SENTINEL_HOME}/media
	cp -r media/* ${SENTINEL_HOME}/media/

	# Create image
	rm -f ${DIST}/sentinel.img
	rm -rf '$(TEMP_SQUASHFS_DIR)'
	mkdir -p '$(TEMP_SQUASHFS_DIR)'
	mkdir -p '$(DIST)'
ifeq ($(DOCKER_SYSTEM_TARGET), yocto)
	mksquashfs ${ROOTFS} '$(TEMP_SQUASHFS_DIR)/sentinel.img' -all-root
else
	mksquashfs ${SENTINEL_HOME} '$(TEMP_SQUASHFS_DIR)/sentinel.img' -all-root
endif

	cp -v '$(TEMP_SQUASHFS_DIR)/sentinel.img' '$(DIST)/'

	# cleanup our temporary directory
	rm -rf '$(TEMP_SQUASHFS_DIR)'

.PHONY: prepare
prepare:
	mkdir -p build
	mkdir -p '$(DIST)'

.PHONY: test
test:
	@echo "No tests configured for EGM Emulator"
