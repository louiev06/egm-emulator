# EGM Emulator - Development Plan

## Current Status (Last Updated: 2025-11-27 13:03)

### Completed
- ✅ C++ EGM Emulator successfully built for Zeus OS
- ✅ Cross-compilation using Yocto SDK (ARM Cortex-A9)
- ✅ Zeus OS hardware integration via s7lite library
- ✅ SAS protocol implementation (Slot Accounting System)
- ✅ Fixed all mutex deadlock issues by converting to recursive_mutex
- ✅ Watchdog timer implementation and feeding (30s timeout, 10s kick interval)
- ✅ Serial port communication via /dev/ttymxc4
- ✅ Build system with version tracking (Build #6)
- ✅ SquashFS image packaging for Zeus OS deployment
- ✅ VSCode tasks.json integration for build workflow

### Architecture
- **Language**: C++11 (Zeus OS requirement)
- **Target Platform**: Zeus OS / Axiomtek (ARM Cortex-A9)
- **Build System**: WSL + Docker + Yocto SDK
- **Firmware Version**: 2.1-zeus-13
- **Communication**: SAS protocol over RS-232 (9-bit mode, 19200 baud)
- **Threading**: Recursive mutexes for deadlock-free multi-threaded access
- **Packaging**: SquashFS filesystem image

### Key Components
1. **Machine Simulator** - Core EGM functionality
   - Game management (3 games configured)
   - Credit/meter tracking
   - Progressive jackpot levels (4 levels)
   - Thread-safe with recursive_mutex

2. **SAS Communication** - Slave device implementation
   - Listens on address 1
   - Responds to general polls and long polls
   - Command processing and response generation
   - Statistics tracking

3. **Zeus Platform Integration**
   - s7lite library for hardware access
   - Watchdog timer management
   - Serial port via Zeus UART API
   - Platform-specific implementations

4. **Build Workflow**
   - increment-build.sh: Auto-increment build number
   - wsl_build.bat: WSL + Docker build orchestration
   - .docker-entrypoint.sh: SDK environment setup
   - EGMEmulator.mak: Main makefile

### Recent Fixes
- **Deadlock Resolution**: Converted all std::mutex to std::recursive_mutex
  - Machine.h, ZeusSerialPort.h, EventService.h
  - MachineCommPort.h, SASCommPort.h, SASDaemon.h
- **Meter Constants**: Renamed MTR_* to METER_* for consistency
- **ODR Compliance**: Added constexpr static member definitions
- **C/C++ Linkage**: Wrapped s7lite.h with extern "C"

### Testing Status
- ✅ Builds successfully with Yocto SDK
- ✅ Runs on Zeus OS hardware without crashes
- ✅ Watchdog properly fed, no automatic reboots
- ✅ Serial port initializes correctly
- ✅ SAS port listening for polls
- ⏳ Awaiting SAS master device for protocol testing

### Next Steps
1. Connect SAS master device for protocol validation
2. Test SAS command processing and responses
3. Verify meter updates and game state changes
4. Test progressive jackpot functionality
5. Validate multi-game switching
6. Performance testing under load

### File Structure
```
egm-emulator-cpp/
├── include/megamic/          # Header files
│   ├── simulator/            # Machine, Game classes
│   ├── sas/                  # SAS protocol
│   ├── io/                   # Zeus platform, serial port
│   ├── event/                # Event system
│   └── version.h             # Build version tracking
├── src/                      # Implementation files
├── Scripts/                  # Deployment scripts
├── EGMEmulator.mak          # Main makefile
├── wsl_build.bat            # Build orchestration
├── .docker-entrypoint.sh    # Docker setup
├── increment-build.sh       # Version management
└── push-changes.sh          # Git workflow automation
```

### Build Commands
- **Build**: `./wsl_build.bat --docker-target yocto --build-option sentinel`
- **Clean**: `./wsl_build.bat --docker-target yocto --build-option clean`
- **VSCode**: Ctrl+Shift+B (uses tasks.json)

### Deployment
1. Build creates `sentinel.img` (SquashFS format)
2. Copy to Zeus OS device
3. Mount and extract to `/opt/ncompass/bin/Sentinel`
4. Run: `/opt/ncompass/bin/Sentinel`

### Known Issues
- None currently

### Documentation
- BUILD_INSTRUCTIONS.md - Complete build setup guide
- USAGE.md - Runtime usage and deployment
- GETTING_STARTED.md - Quick start guide
