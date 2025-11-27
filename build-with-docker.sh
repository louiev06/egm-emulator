#!/usr/bin/env bash
set -Eeuo pipefail

# Run EGM Emulator build using Docker.
#
# You can optionally pass in a make target, ie
#
#       ./build-with-docker.sh egmemulator
#
# Or build for a different system target like Yocto, ie
#
#       DOCKER_SYSTEM_TARGET=yocto ./build-with-docker.sh

: "${DOCKER_SYSTEM_TARGET:?not set -- expected value like yocto: DOCKER_SYSTEM_TARGET=yocto $0 $*}"

# Set EGM_FIRMWARE_VERSION
. ./get-firmware-version.sh

windows=
case "$(uname -o)" in
	Msys) windows=1 ;;
esac

_abs() {
	if [ -n "$windows" ] && command -v cygpath > /dev/null; then
		cygpath --absolute --windows "$@"
	else
		readlink -f "$@"
	fi
}

dir="$(_abs "$PWD")"
# Use unique container name to avoid conflicts
CONTAINER_NAME="egmemulator-build-with-docker-$$-$(date +%s)"
args=(
        --name "$CONTAINER_NAME"
        --hostname "docker-egmemulator-$DOCKER_SYSTEM_TARGET"
        --init

        # see also --tty behavior below
        --interactive --rm

        # extra / prefixes on following paths are for the sake of Windows (to stop MinGW from "helpfully" converting them to C:\... style paths)

        --mount "type=bind,src=$dir,dst=//home/falcondev/tfs/egmemulator"

        --workdir //home/falcondev/tfs/egmemulator

	--env DOCKER_SYSTEM_TARGET
	--env BUILD_NUMBER="${BUILD_NUMBER:-99}"
	--env MAKEFLAGS="-j4 ${MAKEFLAGS:-}"
)

echo $dir

# on Windows, UID/GID mapping in Docker containers doesn't matter (Docker handles that with their filesystem sharing)
if [ -n "$windows" ]; then
	args+=( --user root )
else
	user="$(id -u):$(id -g)"
	groups="$(id -G | xargs -rn1 echo --group-add)"
	args+=(
		--mount type=bind,src=//etc/passwd,dst=//etc/passwd,readonly
		--mount type=bind,src=//etc/group,dst=//etc/group,readonly
		--user "$user"
		$groups
	)
fi

dockerRun='docker run'
if [ -t 0 ] && [ -t 1 ]; then
	args+=( --tty )

	if [ -n "$windows" ] && command -v winpty > /dev/null; then
		dockerRun="winpty $dockerRun"

		# Docker Toolbox creates a "docker" function to disable Git for Windows MSYS path conversions, but we handle those all ourselves instead (and work around MSYS auto-path conversion) since we need to for Docker for Windows (where MSYS is not the expected usage)
		unset docker &> /dev/null || :
	fi
fi

DOCKER_IMAGE="us-west1-docker.pkg.dev/cxs-docker-repository/cxs-docker-dev/ncompass/firmware:$EGM_FIRMWARE_VERSION"
if [ "$DOCKER_SYSTEM_TARGET" = 'yocto' ]; then
	DOCKER_IMAGE_TARGET="$DOCKER_SYSTEM_TARGET-amd64"
else
	DOCKER_IMAGE_TARGET="$DOCKER_SYSTEM_TARGET"
fi

# For yocto, use the standard yocto SDK image
if [ "$DOCKER_SYSTEM_TARGET" = 'yocto' ]; then
	DOCKER_IMAGE="$DOCKER_IMAGE-${DOCKER_IMAGE_TARGET##*-}-sdk-${DOCKER_IMAGE_TARGET%%-*}"
fi

# allow running "bash build-with-docker.sh bash" to get an interactive shell in the container environment
# (otherwise, we assume arguments should be passed directly to "make")
if [ "${1:-}" != 'bash' ]; then
	set -- make -f EGMEmulator.mak "$@"
fi
exec $dockerRun "${args[@]}" "$DOCKER_IMAGE" bash .docker-entrypoint.sh -- "$@"
