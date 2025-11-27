# SAS-Only Configuration Changes

This document summarizes the changes made to focus the EGM emulator exclusively on SAS (Slot Accounting System) protocol support, removing ASP and MS25 protocol support.

## Changes Made

### 1. ICardPlatform.h
**Removed:**
- `createMS25Port()` method
- `createASPPort()` method

**Kept:**
- `createSASPort()` method
- `setLED()` method
- `getPlatformInfo()` method

### 2. SimulatedPlatform (ICardPlatform.h and SimulatedPlatform.cpp)
**Removed:**
- `createMS25Port()` implementation
- `createASPPort()` implementation

**Kept:**
- `createSASPort()` implementation

### 3. Machine.h
**Removed:**
- `addMS25Port()` method
- `hasMS25()` method

**Kept:**
- `addSASPort()` method
- `getPrimarySASPort()` method
- `hasSAS()` method

### 4. Machine.cpp
**Removed:**
- `addMS25Port()` implementation
- `hasMS25()` implementation

**Kept:**
- `addSASPort()` implementation (stub for future SASCommPort)
- `getPrimarySASPort()` implementation (stub)
- `hasSAS()` implementation (stub)

### 5. README.md
**Changes:**
- Updated project overview to mention only SAS protocol
- Removed ASP and MS25 from protocol list
- Updated platform abstraction description to only mention SAS
- Removed MS25CommPort and ASPCommPort from TODO list
- Updated conversion process to only mention SAS
- Updated references section

### 6. CONVERSION_SUMMARY.md
**Changes:**
- Updated platform abstraction section to only mention SAS
- Removed MS25CommPort, ASPCommPort, MS25PollingDaemon, and ASPDaemon from "What's NOT Included"
- Removed ASP and MS25 specific feature counts
- Updated "What remains" section to only mention SAS

### 7. GETTING_STARTED.md
**No changes needed** - This file didn't contain ASP or MS25 references

## Rationale

The EGM emulator now focuses exclusively on the SAS (Slot Accounting System) protocol, which is the industry standard for slot machine communication with casino management systems. This simplification:

1. **Reduces complexity** - Single protocol focus makes the codebase easier to understand and maintain
2. **Improves focus** - Development efforts can concentrate on a complete SAS implementation
3. **Industry standard** - SAS is the dominant protocol in the gaming industry
4. **Cleaner architecture** - Removes unused abstraction layers for protocols that won't be implemented

## SAS Protocol

The Slot Accounting System (SAS) protocol is:
- **Industry standard** for slot machine communication
- **Feature-rich** with 100+ commands
- **Well-documented** with publicly available specifications
- **Widely supported** by gaming equipment manufacturers
- **Comprehensive** covering meters, events, AFT, TITO, progressives, and more

## What Remains to Implement

### Core SAS Components
1. **SASCommPort** - SAS communication port handler
2. **SASDaemon** - SAS polling daemon for continuous communication
3. **SAS Protocol Commands** - 100+ command implementations
4. **CRC-16** - Cyclic redundancy check for data integrity
5. **BCD Encoding/Decoding** - Binary-coded decimal support
6. **Exception Queue** - Event exception management

### SAS Features
1. **AFT (Account Funds Transfer)** - Electronic fund transfers
2. **TITO (Ticket In/Ticket Out)** - Voucher support
3. **Legacy Bonus** - Direct bonus credit awards
4. **Progressives** - Multi-level progressive jackpots
5. **Meters** - Comprehensive meter polling
6. **Events** - Real-time event reporting
7. **Validation** - Validation number generation

### Hardware Integration
1. **Serial Port** - POSIX termios (Linux) or Win32 API (Windows)
2. **Baud Rate** - 19200 bps with MARK parity
3. **Timing** - Precise timing for protocol compliance
4. **LED Control** - Status indicator integration

## File Structure After Changes

```
egm-emulator-cpp/
├── include/megamic/
│   ├── event/
│   │   └── EventService.h          [No changes]
│   ├── io/
│   │   └── CommChannel.h            [No changes]
│   ├── sas/
│   │   └── SASConstants.h           [No changes]
│   ├── simulator/
│   │   ├── Game.h                   [No changes]
│   │   ├── Machine.h                [✓ Removed MS25/ASP methods]
│   │   └── MachineEvents.h          [No changes]
│   └── ICardPlatform.h              [✓ Removed MS25/ASP methods]
│
├── src/
│   ├── event/
│   │   └── EventService.cpp         [No changes]
│   ├── io/
│   │   ├── CommChannel.cpp          [No changes]
│   │   └── SimulatedPlatform.cpp    [✓ Removed MS25/ASP implementations]
│   ├── sas/
│   │   └── SASConstants.cpp         [No changes]
│   └── simulator/
│       ├── Game.cpp                 [No changes]
│       ├── Machine.cpp              [✓ Removed MS25/ASP implementations]
│       └── main.cpp                 [No changes]
│
├── CMakeLists.txt                   [No changes]
├── build.sh                         [No changes]
├── README.md                        [✓ Updated documentation]
├── CONVERSION_SUMMARY.md            [✓ Updated documentation]
├── GETTING_STARTED.md               [No changes needed]
├── .gitignore                       [No changes]
└── SAS_ONLY_CHANGES.md              [✓ This file]
```

## API Changes

### Before (Multi-Protocol)
```cpp
// Platform could create multiple protocol ports
auto sasPort = platform->createSASPort();
auto ms25Port = platform->createMS25Port();
auto aspPort = platform->createASPPort();

// Machine could add multiple protocol ports
machine->addSASPort();
machine->addMS25Port();

// Machine could query for different protocols
bool hasSAS = machine->hasSAS();
bool hasMS25 = machine->hasMS25();
```

### After (SAS-Only)
```cpp
// Platform only creates SAS ports
auto sasPort = platform->createSASPort();

// Machine only adds SAS ports
machine->addSASPort();

// Machine only queries for SAS
bool hasSAS = machine->hasSAS();
```

## Building

No changes to build process. The same commands work:

```bash
# CMake
mkdir build && cd build
cmake ..
cmake --build .

# or simple build script
./build.sh
```

## Testing

The demo application (main.cpp) continues to work without changes as it didn't use MS25 or ASP features.

```bash
./build_manual/egm_simulator
```

## Future Development

Development should now focus on:

1. **Implement SASCommPort class**
   - Inherit from MachineCommPort
   - Handle SAS protocol state machine
   - Manage command/response queuing
   - Implement CRC checking

2. **Add SAS command handlers**
   - Long poll commands (0x00-0xFF)
   - General poll (0x80-0x9F)
   - Real-time event reporting
   - Meter queries
   - AFT/TITO support

3. **Implement SASDaemon**
   - Continuous polling loop
   - Exception queue management
   - Event generation
   - OneLink integration

4. **Serial port hardware support**
   - Linux termios implementation
   - Windows Win32 serial API
   - 19200 baud, MARK parity
   - Timeout handling

## References

- **SAS Protocol**: Industry standard for slot machine communication
- **IGT Documentation**: SAS Protocol specification
- **Original Java**: `C:\_code\gs-olk-product-megamic-main\src\megamic_jar\com\paltronics\megamic\sas\`

---

**Summary**: The EGM emulator now exclusively supports SAS protocol, simplifying the architecture and allowing focused development on a complete, production-ready SAS implementation.
