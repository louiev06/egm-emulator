#!/usr/bin/env bash
set -Eeuo pipefail

# add DOCKER_SYSTEM_TARGET_xxx as a define in CFLAGS / CXXFLAGS so it can be used inside EGMEmulator code:
#
#   #if defined(DOCKER_SYSTEM_TARGET_yocto)
#
#     ...
#
#   #else
#     #error unknown DOCKER_SYSTEM_TARGET
#   #endif
#
export COMMON_CFLAGS="${COMMON_CFLAGS:-} -DDOCKER_SYSTEM_TARGET_${DOCKER_SYSTEM_TARGET//-/_}"
export COMMON_CXXFLAGS="${COMMON_CXXFLAGS:-} -DDOCKER_SYSTEM_TARGET_${DOCKER_SYSTEM_TARGET//-/_}"
export COMMON_LDFLAGS="${COMMON_LDFLAGS:-}"

export CFLAGS="${CFLAGS:-} $COMMON_CFLAGS"
export CXXFLAGS="${CXXFLAGS:-} $COMMON_CXXFLAGS"
export LDFLAGS="${LDFLAGS:-} $COMMON_LDFLAGS"

case "$DOCKER_SYSTEM_TARGET" in
	*-i386 | yocto)
		# if we're in a 32bit userspace, use "linux32" to make "uname -m" report that we're in a 32bit kernel too
		# (especially for 32bit Coverity, which otherwise chokes on the 64bit kernel even though the userspace is fully 32bit)
		set -- linux32 "$@"

		# Source the Yocto SDK environment setup
		if [ -f /opt/fsl-imx-xwayland/5.4-zeus/environment-setup-cortexa9t2hf-neon-poky-linux-gnueabi ]; then
			set +u
			source /opt/fsl-imx-xwayland/5.4-zeus/environment-setup-cortexa9t2hf-neon-poky-linux-gnueabi
			set -u
		fi
		;;
esac

exec "$@"
