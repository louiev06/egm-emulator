# EGM Emulator - Build System Summary

## Quick Start

```cmd
cd C:\_code\egm-emulator\egm-emulator-cpp
wsl_build.bat --docker-target yocto --build-option sentinel
```

## What Gets Built

**Input**: EGMEmulator C++ source code
**Output**: `sentinel.img` (tarball)
**Unpacks to**: `/opt/ncompass/bin/Sentinel` (executable)

## Files Created for Build System

### Core Build Files
1. **EGMEmulator.mak** - GNU Makefile for compilation
   - Compiles all source files (main.cpp, SAS protocol, simulator, etc.)
   - Links with `-ls7lite` (Zeus OS library) and `-lpthread`
   - Creates `Release/EGMEmulator` executable
   - `make sentinel` target packages into sentinel.img

2. **build-with-docker.sh** - Docker build wrapper
   - Mounts current directory into Docker container
   - Uses nCompass Yocto firmware Docker image
   - Invokes `.docker-entrypoint.sh` and `make`

3. **wsl_build.bat** - Windows build script
   - Syncs code from Windows to WSL using robocopy
   - Starts WSL Docker daemon if needed
   - Calls `build-with-docker.sh` in WSL
   - Copies `sentinel.img` back to Windows

4. **.docker-entrypoint.sh** - Docker container entry point
   - Sets up environment variables (CFLAGS, CXXFLAGS)
   - Adds DOCKER_SYSTEM_TARGET defines
   - Executes make command

5. **get-firmware-version.sh** - Version management
   - Reads version from `egm-firmware-version` file
   - Sets `EGM_FIRMWARE_VERSION` environment variable

6. **egm-firmware-version** - Version file
   - Contains: `2.0.0`

### Modified Files
1. **src/simulator/main.cpp** - Updated to be SAS slave device
   - Creates Machine, SASCommPort (not SASDaemon)
   - Runs as SAS slave listening on `/dev/ttymxc4`
   - Displays statistics every 10 seconds

### Directory Structure (Build Output)
```
sentinel.img (tarball containing):
└── opt/
    └── ncompass/
        └── bin/
            └── Sentinel (executable - renamed from EGMEmulator)
```

## Build Process Flow

```
Windows (wsl_build.bat)
  └─> Robocopy code to WSL
      └─> WSL (build-with-docker.sh)
          └─> Docker Container (Yocto toolchain)
              └─> .docker-entrypoint.sh
                  └─> make -f EGMEmulator.mak sentinel
                      ├─> Compile all .cpp files → .o files
                      ├─> Link .o files → Release/EGMEmulator
                      ├─> Copy EGMEmulator to sentinel_image/opt/ncompass/bin/Sentinel
                      └─> tar czf sentinel.img sentinel_image/
          └─> Copy sentinel.img back to Windows
```

## Deployment

1. Copy `sentinel.img` to SD card root
2. Insert SD card into Zeus device
3. Boot device
4. Zeus OS startup script (`/etc/init.d/sentinel.sh`) unpacks sentinel.img
5. Executes `/opt/ncompass/bin/Sentinel`

## Key Differences from nCompass Sentinel

| Aspect | nCompass Sentinel | EGM Emulator |
|--------|------------------|--------------|
| **Source** | 300+ files, complex dependencies | ~20 core files, simpler |
| **Purpose** | Player tracking, bonus system | SAS slave, game emulation |
| **SAS Role** | Can be master or slave | Slave only |
| **Dependencies** | CardReaderLayer, SSL, curl, ActiveMQ | Only s7lite, pthread |
| **Build Time** | 5-10 minutes | < 1 minute |
| **Output Name** | Sentinel | Sentinel (renamed from EGMEmulator) |
| **Angular UI** | Yes (embedded web UI) | No (future phase) |

## Configuration Notes

- **No startup script needed**: Uses existing `/etc/init.d/sentinel.sh` from Zeus OS
- **No OS modifications**: Drops in as direct replacement for Sentinel
- **Same paths**: Uses `/opt/ncompass` to match Zeus OS expectations
- **Same executable name**: "Sentinel" to match startup script

## Next Steps (GUI - Phase 7)

After successful build and deployment:
1. Verify EGM Emulator runs and responds to SAS polls
2. Test with Player Tracking device as master
3. Implement web-based GUI (Phase 7)
   - Add cpp-httplib for embedded HTTP server
   - Create HTML/CSS/JS interface for game simulation
   - Add API endpoints for bill insertion, play, cashout
   - Display real-time meters and credits

## Build Artifacts

After successful build:
- **Release/EGMEmulator** - Compiled executable (ARM32 for Zeus OS)
- **sentinel.img** - Deployment package (gzipped tar)
- **Release/*.o** - Object files (intermediate)

## Cleanup

Remove build artifacts:
```bash
make -f EGMEmulator.mak clean
```

This removes:
- Release/EGMEmulator
- Release/*.o
- sentinel.img
- sentinel_image/ directory
