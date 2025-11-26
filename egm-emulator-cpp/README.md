# EGM Emulator - C++ Port

This is a C++ port of the Paltronics MegaMIC EGM (Electronic Gaming Machine) emulator, originally written in Java.

## Project Overview

The EGM emulator simulates a gaming machine cabinet that communicates with casino management systems using the industry-standard SAS (Slot Accounting System) protocol for slot machine communication.

## Version

- **Version**: 15.4.0
- **Original**: Java 8 implementation
- **C++ Port**: C++11

## Architecture

### Core Components

#### Machine ([Machine.h](include/megamic/simulator/Machine.h), [Machine.cpp](src/simulator/Machine.cpp))
The main class simulating a game cabinet with:
- Multiple game configurations
- Meter tracking (coin-in, coin-out, games played, etc.)
- Progressive jackpot management
- Credit management (cashable, restricted, non-restricted)
- State management (door, lights, hopper, handpay, AFT lock)
- Event publishing system

#### Game ([Game.h](include/megamic/simulator/Game.h), [Game.cpp](src/simulator/Game.cpp))
Represents an individual game within a multi-game cabinet:
- Game number and denomination
- Max bet configuration
- Coin-in meter tracking
- Paytable information

#### EventService ([EventService.h](include/megamic/event/EventService.h), [EventService.cpp](src/event/EventService.cpp))
Thread-safe publish-subscribe event bus for:
- Game state changes
- Progressive hits
- Bonus awards
- AFT/EFT transfers
- Door/light/hopper events

#### Platform Abstraction ([ICardPlatform.h](include/megamic/ICardPlatform.h))
Hardware abstraction layer for:
- Serial port creation (SAS)
- LED control
- Platform-specific features

**Implementations:**
- **SimulatedPlatform** ([SimulatedPlatform.cpp](src/io/SimulatedPlatform.cpp)) - For development/testing
- **ZeusPlatform** ([ZeusPlatform.h](include/megamic/ZeusPlatform.h), [ZeusPlatform.cpp](src/io/ZeusPlatform.cpp)) - For Zeus OS / Axiomtek S7 Lite hardware

### Supporting Components

#### SAS Constants ([SASConstants.h](include/megamic/sas/SASConstants.h))
- Meter code definitions
- Denomination mappings
- Protocol constants

#### Communication Channels ([CommChannel.h](include/megamic/io/CommChannel.h))
Abstract serial port interface with implementations:
- **PipedCommChannel** - For simulation and testing
- **ZeusSerialPort** ([ZeusSerialPort.h](include/megamic/io/ZeusSerialPort.h), [ZeusSerialPort.cpp](src/io/ZeusSerialPort.cpp)) - Zeus OS hardware UART (19200 baud, 9-bit SAS)
- Timeout-aware I/O operations

#### Machine Events ([MachineEvents.h](include/megamic/simulator/MachineEvents.h))
Event type definitions:
- `GamePlayedEvent`
- `ProgressiveHitEvent`
- `BonusAwardedEvent`
- `AftTransferEvent`
- `GameChangedEvent`
- And more...

## Key Features

### Meter Management
Tracks 20+ different meters including:
- Coin in/out
- Games played/won/lost
- Jackpots
- Bill acceptor counts ($1, $5, $10, $20, $50, $100)
- Ticket in/out
- Current credits (cashable, restricted, non-restricted)

### Progressive Jackpots
- Multiple progressive levels per machine
- Progressive link monitoring (5-second timeout)
- Hit detection and handpay triggering
- Progressive value broadcasting

### Credit Types
- **Cashable**: Regular player credits
- **Restricted**: Promotional credits with restrictions
- **Non-Restricted**: Non-cashable promotional credits

### State Management
- Door open/close detection
- Service light control
- Hopper low detection
- AFT (Account Funds Transfer) locking
- Handpay pending states
- Game delay timers

### Event System
Type-safe C++ event system using:
- `std::function` for callbacks
- Custom `EventHolder` for type-erased event storage (C++11 compatible)
- `std::mutex` for thread safety
- Template-based subscription API

## Building

### Prerequisites
- CMake 3.15 or higher
- C++11 compatible compiler (GCC 4.8+, Clang 3.3+, MSVC 2013+)
- Threads library (pthreads on Unix, native on Windows)
- **Zeus OS only**: S7Lite library (libs7lite.so)

### Build Instructions

#### Standard Build (Development/Testing)
```bash
cd egm-emulator-cpp
mkdir build
cd build
cmake ..
cmake --build .
```

#### Zeus OS Build (Hardware Platform)
On Zeus OS, the build automatically detects `/dev/ttymxc4` and enables Zeus integration:
```bash
cd egm-emulator-cpp
mkdir build
cd build
cmake ..  # Auto-detects Zeus OS
cmake --build .
```

The CMake configuration will output:
```
-- Zeus OS platform detected - enabling Zeus integration
```

### Build Options

- `BUILD_TESTS` - Build unit tests (default: ON)
- `BUILD_SIMULATOR` - Build simulator executable (default: ON)

```bash
cmake -DBUILD_TESTS=OFF -DBUILD_SIMULATOR=ON ..
```

### Running the Simulator

#### Standard Platform
```bash
./egm_simulator
```

#### Zeus OS Platform
```bash
./egm_simulator
# Or if watchdog is enabled:
sudo ./egm_simulator
```

## Project Structure

```
egm-emulator-cpp/
├── include/
│   └── megamic/
│       ├── event/
│       │   └── EventService.h
│       ├── io/
│       │   ├── CommChannel.h
│       │   └── ZeusSerialPort.h          [Zeus OS]
│       ├── sas/
│       │   └── SASConstants.h
│       ├── simulator/
│       │   ├── Game.h
│       │   ├── Machine.h
│       │   └── MachineEvents.h
│       ├── ICardPlatform.h
│       └── ZeusPlatform.h                [Zeus OS]
├── src/
│   ├── event/
│   │   └── EventService.cpp
│   ├── io/
│   │   ├── CommChannel.cpp
│   │   ├── SimulatedPlatform.cpp         [Non-Zeus]
│   │   ├── ZeusSerialPort.cpp            [Zeus OS]
│   │   └── ZeusPlatform.cpp              [Zeus OS]
│   ├── sas/
│   │   └── SASConstants.cpp
│   └── simulator/
│       ├── Game.cpp
│       ├── Machine.cpp
│       └── main.cpp
├── tests/
├── CMakeLists.txt
├── README.md
├── CONVERSION_SUMMARY.md
├── SAS_ONLY_CHANGES.md
├── GETTING_STARTED.md
├── PLAN.md
└── ZEUS_INTEGRATION.md                   [Zeus OS Guide]
```

## Differences from Java Implementation

### Language-Specific Changes

1. **BigDecimal → double**
   - Java's `BigDecimal` replaced with `double` for currency
   - Acceptable for gaming applications with proper rounding

2. **Threading**
   - `java.util.Timer` → `std::thread` with `std::chrono`
   - `ExecutorService` → Direct thread management
   - `BlockingQueue` → `std::queue` with mutexes

3. **Collections**
   - `ArrayList` → `std::vector`
   - `HashMap` → `std::map`/`std::unordered_map`
   - `LinkedList` → `std::queue`

4. **Event System**
   - Java reflection → C++ templates with custom `EventHolder` (type-erased storage)
   - Type-safe subscription via `std::function`

5. **Memory Management**
   - `new` → `std::make_shared` for RAII
   - Automatic cleanup via destructors
   - Smart pointers throughout

### Design Improvements

1. **Thread Safety**
   - All meter operations protected by mutex
   - Lock guards for RAII-style locking
   - Atomic variables for boolean flags

2. **C++11 Features**
   - Range-based for loops
   - Auto type deduction
   - Lambda expressions for callbacks
   - Move semantics
   - Smart pointers (shared_ptr, unique_ptr)

3. **Const Correctness**
   - Const methods where applicable
   - Const references for parameters

## Zeus OS Integration

### Overview
The EGM emulator includes full support for the **Zeus OS / Axiomtek S7 Lite** gaming platform, providing hardware-accelerated SAS communication and platform features.

### Hardware Features
- **SAS Serial Port** - Pre-configured 19200 baud, 9-bit mode (hardware UART)
- **SRAM Storage** - Non-volatile memory for game state persistence
- **Watchdog Timer** - System reliability monitoring (0-255 second timeout)
- **Battery Monitoring** - Battery backup status and voltage
- **RTC** - Real-time clock
- **LED Control** - Status indicators
- **LCD Backlight** - Display brightness control (0-1023)

### Zeus API Wrapper Classes

#### ZeusSerialPort ([ZeusSerialPort.h](include/megamic/io/ZeusSerialPort.h))
Wraps the Zeus S7Lite UART API for SAS communication:
```cpp
auto port = std::make_shared<io::ZeusSerialPort>("SAS");
port->open();

// Send SAS message
uint8_t txBuffer[] = {0x01, 0x80};  // Poll address 1
port->write(txBuffer, 2);

// Receive response
uint8_t rxBuffer[256];
int bytesRead = port->read(rxBuffer, sizeof(rxBuffer),
                          std::chrono::milliseconds(20));
```

**Key Features:**
- Automatic 8-bit ↔ 16-bit word conversion (Zeus API uses USHORT)
- Thread-safe API access with mutexes
- Configurable timeouts
- Buffer management (clear TX/RX buffers)

#### ZeusPlatform ([ZeusPlatform.h](include/megamic/ZeusPlatform.h))
Complete platform implementation for Zeus OS:
```cpp
auto platform = std::make_shared<ZeusPlatform>(
    true,  // Enable watchdog
    30     // 30-second timeout
);

platform->initialize();

// Watchdog management
platform->kickWatchdog();  // Call periodically

// SRAM persistence
GameState state;
platform->writeSRAM(0, (uint8_t*)&state, sizeof(GameState)/2);
platform->readSRAM(0, (uint8_t*)&state, sizeof(GameState)/2);

// Battery monitoring
bool batteryOK = platform->getBatteryStatus();
uint16_t voltage = platform->getBatteryVoltage();
```

### Building for Zeus OS
The build system automatically detects Zeus OS by checking for `/dev/ttymxc4`:
```bash
cmake ..  # Auto-detection
# Output: "Zeus OS platform detected - enabling Zeus integration"
```

Manual override (if needed):
```bash
cmake -DZEUS_OS=ON ..
```

### Cross-Compilation
For ARM cross-compilation:
```bash
cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain-arm-zeus.cmake ..
make
```

### Zeus API Documentation
Complete Zeus OS integration guide: [ZEUS_INTEGRATION.md](ZEUS_INTEGRATION.md)

Topics covered:
- S7Lite API function reference
- Data type conversions (9-bit SAS protocol)
- Architecture diagrams
- Usage examples
- Troubleshooting guide
- Performance considerations

## TODO - Future Enhancements

### Protocol Implementation
- [ ] Implement `SASCommPort` class
- [ ] Add CRC calculation utilities (CRC-16 for SAS)
- [ ] BCD encoding/decoding helpers

### Platform Support
- [x] Zeus OS / Axiomtek S7 Lite integration (complete)
- [x] Zeus serial port wrapper (ZeusSerialPort)
- [ ] Linux serial port implementation (termios) - for non-Zeus platforms
- [ ] Windows serial port implementation (Win32 API)
- [ ] PAL-264 hardware platform support
- [ ] Zeus LED controller integration (via GPIO)

### Additional Features
- [ ] XML schema support for OneLink
- [ ] SSL/TLS for network communication
- [ ] HTTP server for web console
- [ ] Persistent storage (ICard serialization)
- [ ] Logging framework integration (spdlog)
- [ ] Configuration file support
- [ ] Unit tests (Google Test)

### Protocol Features
- [ ] AFT (Account Funds Transfer) implementation
- [ ] TITO (Ticket In/Ticket Out) support
- [ ] Real-time meter polling
- [ ] Exception handling and reporting
- [ ] Bonus award processing
- [ ] Progressive broadcasts

## Conversion Notes

This C++ port was created by converting [Machine.java](C:\_code\gs-olk-product-megamic-main\src\megamic_sim_jar\com\paltronics\megamic\simulator\Machine.java) from the original Java implementation.

### Java Source
- **Location**: `C:\_code\gs-olk-product-megamic-main`
- **Package**: `com.paltronics.megamic.simulator`
- **Build System**: Apache Ant
- **Dependencies**: OneLink Commons, Gson, XPP3

### Conversion Process
1. ✅ Project structure and CMake build system
2. ✅ Core classes (Machine, Game)
3. ✅ Event system
4. ✅ Platform abstraction
5. ⏳ SAS communication port
6. ⏳ SAS protocol implementation
7. ⏳ Network integration
8. ⏳ Testing and validation

## License

Copyright © 2009 Paltronics, Inc. All Rights Reserved.

This is a proprietary system. The original Java implementation is PALTRONICS PROPRIETARY/CONFIDENTIAL.

This C++ port maintains the same architecture and functionality as the original implementation.

## References

- **Original Project**: Paltronics PAL-264 "MegaMIC" (Machine Interface Controller)
- **Protocol**: SAS (Slot Accounting System)
- **Casino Management System**: OneLink
- **Version**: 15.4.0 (5.5 series)
