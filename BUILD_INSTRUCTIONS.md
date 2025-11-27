# EGM Emulator - Build Instructions

## Overview

The EGM Emulator is a SAS-compliant Electronic Gaming Machine emulator that runs on Zeus OS hardware. It builds using the same Docker/Yocto toolchain as nCompass Sentinel and produces a `sentinel.img` file that can replace the standard Sentinel application.

## Prerequisites

1. **WSL (Windows Subsystem for Linux)** with Ubuntu 20.04
2. **Docker** installed and running in WSL
3. **Access to the nCompass Docker images** (same as used for Sentinel builds)
4. **WSL configuration file** at `C:\.config\wsl.ini`

### WSL Configuration File

Create `C:\.config\wsl.ini` with the following content:

```ini
SOURCE_BASE=C:\_code\egm-emulator
DEST_BASE=\\wsl.localhost\Ubuntu-20.04\home\lou\egm-code
WSL_BASE=/home/lou/egm-code
UBUNTU_VERSION=Ubuntu-20.04
```

Adjust paths as needed for your environment.

## Building

### Windows Build (Recommended)

From the `egm-emulator-cpp` directory, run:

```cmd
wsl_build.bat --docker-target yocto --build-option sentinel
```

This will:
1. Sync your code to WSL
2. Start the Docker container with Yocto toolchain
3. Build the EGMEmulator executable
4. Package it into `sentinel.img`
5. Copy `sentinel.img` back to your Windows directory

### WSL/Linux Build

From within WSL or Linux:

```bash
DOCKER_SYSTEM_TARGET=yocto ./build-with-docker.sh sentinel
```

### Build Output

The build produces:
- **Release/EGMEmulator** - The compiled executable
- **sentinel.img** - Tarball containing the executable (placed at `/opt/ncompass/bin/Sentinel`)

## Installation on Zeus OS Device

1. Copy `sentinel.img` to the root of your Zeus OS SD card
2. Reboot the device
3. The Zeus OS startup script will automatically unpack and run the EGMEmulator

**Note**: This replaces the standard Sentinel application. The executable is named "Sentinel" to match the existing Zeus OS startup script expectations.

## Build System Architecture

The build system follows the same pattern as nCompass Sentinel:

- **EGMEmulator.mak** - GNU Makefile for compiling the project
- **build-with-docker.sh** - Docker wrapper script (Linux/WSL)
- **wsl_build.bat** - Windows batch script for building via WSL
- **.docker-entrypoint.sh** - Docker container entrypoint
- **get-firmware-version.sh** - Version management
- **Scripts/egmemulator.sh** - Zeus OS startup script (optional, not used for sentinel.img)

## Build Targets

### Make Targets

- `make all` - Build the EGMEmulator executable only
- `make sentinel` - Build executable and package into sentinel.img
- `make clean` - Remove build artifacts
- `make cleanall` - Deep clean (same as clean for this project)
- `make rebuild` - Clean and rebuild everything

### Build Options

The build can be customized with environment variables:

- **DOCKER_SYSTEM_TARGET** - Target platform (default: `yocto`)
- **BUILD_NUMBER** - Build number for versioning (default: `99`)
- **ZEUS_OS** - Enable Zeus OS specific code (automatically set for yocto builds)

## Project Structure

```
egm-emulator-cpp/
├── include/           # Header files
│   └── megamic/
│       ├── event/     # Event system
│       ├── io/        # I/O and platform abstraction
│       ├── sas/       # SAS protocol implementation
│       └── simulator/ # Gaming machine simulation
├── src/               # Source files
│   ├── event/
│   ├── io/
│   ├── sas/
│   │   └── commands/  # SAS command handlers
│   └── simulator/
├── Scripts/           # Deployment scripts
├── Release/           # Build output directory
├── EGMEmulator.mak    # Main makefile
├── build-with-docker.sh
├── wsl_build.bat
├── .docker-entrypoint.sh
└── get-firmware-version.sh
```

## Device Configuration

### EGM Emulator (Slave Device - Device B)

The built application runs as a **SAS Slave** device that:
- Responds to SAS polls from a master device
- Emulates an Electronic Gaming Machine
- Reports meters, game state, and progressive jackpots
- Handles TITO, AFT, and other SAS commands

**Serial Port**: `/dev/ttymxc4`
**SAS Address**: 1 (default)

### Hardware Connections

Connect two Zeus devices with a crossover serial cable:
- Pin 1 (White/RX) ↔ Pin 2 (Red/TX)
- Pin 2 (Red/TX) ↔ Pin 1 (White/RX)
- Pin 3 (Black/GND) ↔ Pin 3 (Black/GND)

## Troubleshooting

### Build Fails

1. **Docker not running**:
   ```cmd
   wsl --distribution Ubuntu-20.04 --exec bash -c "sudo service docker start"
   ```

2. **Permission errors**: Make sure scripts are executable:
   ```bash
   chmod +x build-with-docker.sh .docker-entrypoint.sh get-firmware-version.sh
   ```

3. **Container conflicts**: Clean up old containers:
   ```bash
   docker ps -a | grep egmemulator-build-with-docker | awk '{print $1}' | xargs docker rm -f
   ```

### Runtime Issues on Zeus OS

1. **Check logs**:
   ```bash
   tail -f /var/log/egmemulator.log
   # or if using sentinel.img:
   tail -f /var/log/sentinel.log
   ```

2. **Verify serial port**:
   ```bash
   ls -l /dev/ttymxc4
   ```

3. **Check if running**:
   ```bash
   ps aux | grep EGMEmulator
   # or if using sentinel.img:
   ps aux | grep Sentinel
   ```

## Development

For development without Zeus OS hardware, the code includes simulated platform support. Build without `-DZEUS_OS` flag to use simulated I/O.

## Version

- **EGM Emulator**: Version 1.0.0
- **Firmware Version**: 2.0.0 (see `egm-firmware-version`)
- **C++ Standard**: C++11 (for Zeus OS compatibility)

## License

Proprietary - Internal use only
