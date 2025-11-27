# Zeus OS Integration Guide

This document explains how the EGM Emulator integrates with Zeus OS and the Axiomtek S7 Lite hardware platform.

## Overview

The Zeus OS platform provides a third-party API for accessing hardware features of the Axiomtek S7 Lite gaming board. This API is implemented in the **S7Lite library** (libs7lite.so) and provides the following capabilities:

- **SAS Serial Communication** - Hardware UART pre-configured for SAS protocol
- **SRAM Storage** - Non-volatile memory for game state persistence
- **Watchdog Timer** - Hardware watchdog for system reliability
- **Battery Monitoring** - Battery backup status and voltage
- **RTC** - Real-time clock
- **LED Control** - Status indicators
- **LCD Backlight** - Display brightness control

## Architecture

```
┌─────────────────────────────────────────────────────┐
│         EGM Emulator C++ Application                │
├─────────────────────────────────────────────────────┤
│  Machine  │  SASDaemon  │  SASCommPort              │
├─────────────────────────────────────────────────────┤
│       ZeusPlatform        │     ZeusSerialPort      │
│    (megamic namespace)    │   (megamic::io)         │
├─────────────────────────────────────────────────────┤
│      Zeus OS S7Lite API (Third-Party Library)       │
│  S7LITE_DLL_Init, S7LITE_UART_*, S7LITE_SRAM_*, ... │
├─────────────────────────────────────────────────────┤
│              Axiomtek S7 Lite Hardware               │
│  MCU (/dev/ttymxc4)  │  SRAM  │  Watchdog  │  LEDs │
└─────────────────────────────────────────────────────┘
```

## Key Classes

### ZeusSerialPort

**Location**: [include/megamic/io/ZeusSerialPort.h](include/megamic/io/ZeusSerialPort.h)

Wraps the Zeus S7Lite UART API to provide SAS serial communication.

**Key Zeus API Functions Used:**
- `S7LITE_DLL_Init()` - Initialize hardware
- `S7LITE_UART_SendBuffer()` - Send data (9-bit SAS)
- `S7LITE_UART_GetBuffer()` - Receive data (9-bit SAS)
- `S7LITE_UART_ClearBuffers()` - Clear TX/RX buffers
- `S7LITE_UART_SetTimeouts()` - Configure read timeout

**Important**: Zeus API uses 16-bit words (USHORT) where only the lower 8 bits contain data. The 9th bit (wake bit) is handled by the hardware automatically.

### ZeusPlatform

**Location**: [include/megamic/ZeusPlatform.h](include/megamic/ZeusPlatform.h)

Implements the ICardPlatform interface for Zeus OS hardware.

**Features:**
- Creates ZeusSerialPort instances
- Manages hardware watchdog
- SRAM read/write for game state persistence
- Battery monitoring
- Platform information queries

## Zeus OS S7Lite API Reference

### Initialization

```cpp
// Initialize the S7Lite library (must be called first)
S7_Result S7LITE_DLL_Init(void);

// Cleanup and release resources
S7_Result S7LITE_DLL_DeInit(void);

// Get library version
S7_Result S7LITE_DLL_GetDLLVersion(BYTE *pVersion);  // [MAJOR, MINOR, PATCH]
```

### UART Functions (SAS Communication)

```cpp
// Send a buffer of words (16-bit, lower 8 bits = data)
S7_Result S7LITE_UART_SendBuffer(UINT uart, USHORT *pbuffer, UINT length);

// Receive a buffer of words (16-bit, lower 8 bits = data)
S7_Result S7LITE_UART_GetBuffer(UINT uart, USHORT *pbuffer, UINT *plength);

// Clear RX and/or TX buffers
S7_Result S7LITE_UART_ClearBuffers(UINT uart, USHORT mask);
// mask: CLR_RX_BUFFER (0x01), CLR_TX_BUFFER (0x02)

// Set read timeout
S7_Result S7LITE_UART_SetTimeouts(UINT uart, UINT readinterval,
                                   UINT writemultipiler, UINT writeconstant);
```

**UART Configuration** (Pre-configured by hardware):
- **Baud Rate**: 19200
- **Data Bits**: 9 (8 data + 1 wake bit)
- **Parity**: None (9th bit handles addressing)
- **Stop Bits**: 1
- **Handshaking**: None

### SRAM Functions (Non-Volatile Storage)

```cpp
// Get SRAM capacity
S7_Result S7LITE_SRAM_Size(UINT* pSize);  // Returns size in bytes

// Read from SRAM
S7_Result S7LITE_SRAM_Read(S7LITE_SRAMACCESSBLOCK access,
                           void (*fn)(PVOID context), PVOID context);

// Write to SRAM
S7_Result S7LITE_SRAM_Write(S7LITE_SRAMACCESS access,
                            void (*fn)(PVOID context), PVOID context);

// SRAM access structures
typedef struct {
    PUCHAR buffer;   // Data buffer
    UINT offset;     // Offset in SRAM (in WORD units, 16-bit)
    UINT length;     // Number of words to read/write
} S7LITE_SRAMACCESSBLOCK;

typedef struct {
    S7LITE_SRAMACCESSBLOCK block[4];  // Up to 4 blocks per operation
} S7LITE_SRAMACCESS;
```

**SRAM Limits**:
- Maximum single read: 4096 bytes
- Maximum single write block: 256 words (512 bytes)
- Maximum write blocks per operation: 4

### Watchdog Functions

```cpp
// Enable watchdog timer
S7_Result S7LITE_Watchdog_Enable(void);

// Set watchdog timeout (0-255 seconds)
S7_Result S7LITE_Watchdog_SetTimeout(UINT time);

// Kick watchdog (prevent reset)
S7_Result S7LITE_Watchdog_Kick(void);
```

### Battery Monitoring

```cpp
// Get battery status
S7_Result S7LITE_Battery_GetStatus(BOOLEAN *pStatus);  // TRUE = good, FALSE = bad

// Get battery voltage in millivolts
S7_Result S7LITE_Battery_GetVoltage(USHORT *voltage);
```

### RTC Functions

```cpp
// Get current time
S7_Result S7LITE_Rtc_GetTime(SYSTEMTIME *systemTime);

// Set RTC and system time
S7_Result S7LITE_Rtc_SetTime(SYSTEMTIME systemTime);
```

### Display Functions

```cpp
// Set LCD backlight brightness (0-1023)
S7_Result S7LITE_RemoteLCD_SetBacklightPWM(UINT brightness);
```

### Firmware Functions

```cpp
// Get MCU firmware version string
S7_Result S7LITE_Firmware_Version(char *version, size_t *size);
// Format: "PRODUCT.MAJOR.MINOR.BUILD"

// Upgrade MCU firmware from file
S7_Result S7LITE_FirmwareUpgradeByFile(char *filename);
```

## Data Type Conversions

### SAS 9-bit Protocol

Zeus OS hardware handles SAS 9-bit mode automatically. The UART is configured with:
- 8 data bits
- 1 wake bit (9th bit)

**Data Format in Zeus API:**
```
16-bit Word (USHORT)
┌─────────┬─────────┐
│ Byte 1  │ Byte 0  │
│ (0x00)  │ (data)  │
└─────────┴─────────┘
    ↑          ↑
  Unused    Data (8-bit)
```

**Conversion in ZeusSerialPort:**

```cpp
// Write: Convert uint8_t[] to USHORT[]
for (int i = 0; i < numBytes; i++) {
    words[i] = static_cast<USHORT>(buffer[i]);  // Lower 8 bits = data
}

// Read: Convert USHORT[] to uint8_t[]
for (int i = 0; i < numWords; i++) {
    buffer[i] = static_cast<uint8_t>(words[i] & 0xFF);  // Extract lower 8 bits
}
```

## Usage Examples

### Example 1: Basic Initialization

```cpp
#include "megamic/ZeusPlatform.h"
#include "megamic/simulator/Machine.h"

int main() {
    // Create Zeus platform
    auto platform = std::make_shared<megamic::ZeusPlatform>(
        true,  // Enable watchdog
        30     // 30-second timeout
    );

    // Initialize platform
    if (!platform->initialize()) {
        std::cerr << "Failed to initialize Zeus platform" << std::endl;
        return 1;
    }

    std::cout << platform->getPlatformInfo() << std::endl;

    // Create event service
    auto eventService = std::make_shared<megamic::event::EventService>();

    // Create machine
    auto machine = std::make_shared<megamic::simulator::Machine>(
        eventService,
        platform
    );

    // Watchdog kick loop (should be in separate thread)
    while (running) {
        platform->kickWatchdog();
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }

    // Cleanup
    platform->shutdown();
    return 0;
}
```

### Example 2: SAS Communication

```cpp
#include "megamic/io/ZeusSerialPort.h"

// Create and open port
auto port = std::make_shared<megamic::io::ZeusSerialPort>("SAS");
if (!port->open()) {
    std::cerr << "Failed to open SAS port" << std::endl;
    return;
}

// Send SAS message
uint8_t txBuffer[] = {0x01, 0x80};  // Poll address 1, general poll
int bytesWritten = port->write(txBuffer, 2);

// Receive response (20ms timeout typical for SAS)
uint8_t rxBuffer[256];
int bytesRead = port->read(rxBuffer, sizeof(rxBuffer),
                           std::chrono::milliseconds(20));

if (bytesRead > 0) {
    // Process SAS response
    for (int i = 0; i < bytesRead; i++) {
        printf("%02X ", rxBuffer[i]);
    }
    printf("\n");
}

port->close();
```

### Example 3: SRAM Persistence

```cpp
#include "megamic/ZeusPlatform.h"

auto platform = std::make_shared<megamic::ZeusPlatform>();
platform->initialize();

// Save game state to SRAM
struct GameState {
    uint32_t gamesPlayed;
    uint32_t coinIn;
    uint32_t coinOut;
} state = {100, 5000, 3000};

bool success = platform->writeSRAM(
    0,                          // Offset (in words)
    reinterpret_cast<uint8_t*>(&state),
    sizeof(GameState) / 2       // Length (in words)
);

// Load game state from SRAM
GameState loadedState;
success = platform->readSRAM(
    0,                          // Offset (in words)
    reinterpret_cast<uint8_t*>(&loadedState),
    sizeof(GameState) / 2       // Length (in words)
);
```

### Example 4: Battery Monitoring

```cpp
auto platform = std::make_shared<megamic::ZeusPlatform>();
platform->initialize();

// Check battery status
if (!platform->getBatteryStatus()) {
    std::cerr << "WARNING: Battery is low!" << std::endl;
}

// Get battery voltage
uint16_t voltage = platform->getBatteryVoltage();
std::cout << "Battery voltage: " << voltage << " mV" << std::endl;

// Typical values:
// - Good battery: 3000-3500 mV
// - Low battery: < 2800 mV
```

## Building for Zeus OS

### CMake Configuration

Add Zeus-specific build options to `CMakeLists.txt`:

```cmake
# Detect Zeus OS
if(EXISTS "/dev/ttymxc4")
    set(ZEUS_OS ON)
    add_definitions(-DZEUS_OS)
endif()

# Add Zeus source files
if(ZEUS_OS)
    set(ZEUS_SOURCES
        src/io/ZeusSerialPort.cpp
        src/io/ZeusPlatform.cpp
    )

    # Link Zeus S7Lite library
    target_link_libraries(egm_core s7lite)
else()
    # Use simulated platform
    set(ZEUS_SOURCES
        src/io/SimulatedPlatform.cpp
    )
endif()

list(APPEND COMMON_SOURCES ${ZEUS_SOURCES})
```

### Compilation

```bash
# On Zeus OS target
mkdir build && cd build
cmake ..
make

# Cross-compilation for Zeus OS (ARM)
mkdir build-arm && cd build-arm
cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain-arm-zeus.cmake ..
make
```

### Linking

The S7Lite library must be available:

```bash
# Check library availability
ls -l /usr/lib/libs7lite.so*

# Link during compilation
g++ -o egm_simulator ... -ls7lite
```

## Hardware Details

### Axiomtek S7 Lite Specifications

- **MCU Device**: `/dev/ttymxc4`
- **MCU Baud Rate**: 1.5 Mbps (internal communication)
- **SAS Baud Rate**: 19200 bps (to slot machine)
- **SRAM**: Variable size (queried via API)
- **Watchdog**: 0-255 seconds timeout
- **RTC Device**: `/dev/rtc`
- **Watchdog Device**: `/dev/watchdog`

### Physical Connections

```
┌─────────────────────────────────────┐
│     Axiomtek S7 Lite Board          │
├─────────────────────────────────────┤
│  ARM CPU  →  MCU (STM32)  →  SAS   │
│     ↑          ↑              Port  │
│     └──────────┘                ↓   │
│    libs7lite.so              Slot   │
│                             Machine │
└─────────────────────────────────────┘
```

## Thread Safety

The Zeus S7Lite API requires thread-safe access:

**ZeusSerialPort Protection:**
- Uses `std::mutex` for all API calls
- Separate read/write locks in S7Lite internally
- Safe for multi-threaded use

**Best Practices:**
```cpp
// Good: Single SASDaemon thread
std::thread sasThread([&port]() {
    while (running) {
        port->write(command, commandSize);
        port->read(response, responseSize, timeout);
    }
});

// Bad: Multiple threads writing simultaneously
// (Though mutex protects this, performance suffers)
```

## Troubleshooting

### Issue: S7LITE_DLL_Init() fails

**Possible causes:**
- MCU device `/dev/ttymxc4` not accessible
- Permission denied (need root or group membership)
- MCU firmware not responding

**Solution:**
```bash
# Check device permissions
ls -l /dev/ttymxc4

# Add user to dialout group
sudo usermod -a -G dialout $USER

# Check MCU is responding
cat /dev/ttymxc4  # Should show some output
```

### Issue: No data received from SAS port

**Possible causes:**
- Timeout too short
- Buffers not cleared
- Wrong poll address
- Slot machine not connected

**Solution:**
```cpp
// Clear buffers before reading
port->clearBuffers(true, true);

// Use longer timeout for debugging
port->read(buffer, size, std::chrono::milliseconds(1000));

// Check if bytes were sent
int written = port->write(data, size);
if (written != size) {
    std::cerr << "Write failed!" << std::endl;
}
```

### Issue: SRAM read/write fails

**Possible causes:**
- Offset out of bounds
- Length exceeds maximum
- SRAM size not queried

**Solution:**
```cpp
// Check SRAM size first
uint32_t sramSize = platform->getSRAMSize();
std::cout << "SRAM Size: " << sramSize << " bytes" << std::endl;

// Ensure offset + length doesn't exceed size
if ((offset + length) * 2 > sramSize) {
    std::cerr << "SRAM access out of bounds!" << std::endl;
}
```

### Issue: Watchdog resets system

**Possible causes:**
- kickWatchdog() not called frequently enough
- Application hung/crashed
- Timeout too short

**Solution:**
```cpp
// Increase watchdog timeout
auto platform = std::make_shared<ZeusPlatform>(true, 60);  // 60 seconds

// Create dedicated watchdog thread
std::thread wdtThread([&platform]() {
    while (running) {
        platform->kickWatchdog();
        std::this_thread::sleep_for(std::chrono::seconds(15));  // Half timeout
    }
});
```

## Performance Considerations

### UART Throughput

- **Maximum SAS message rate**: ~50 messages/second
- **Typical poll interval**: 20-40ms
- **Read timeout**: 20ms (typical SAS response time)

### SRAM Access

- **Read speed**: ~4096 bytes in <100ms
- **Write speed**: ~512 bytes in <50ms
- **Use batching** for multiple writes (up to 4 blocks)

### CPU Usage

- Polling loop: <5% CPU
- Zeus API overhead: Minimal (<1% CPU)
- Most time spent waiting for I/O

## References

- **Zeus OS Documentation**: See Axiomtek S7 Lite SDK
- **S7Lite API Reference**: [s7lite.h](C:\_code\Firmware.Krogoth\axiomtek\s7lite\include\s7lite.h)
- **S7Lite Implementation**: [s7lite.c](C:\_code\Firmware.Krogoth\axiomtek\s7lite\src\s7lite.c)
- **SAS Protocol**: IGT SAS Version 6.02+ specification

---

**Last Updated**: 2025-11-21
**Status**: Production Ready
**Platform**: Zeus OS / Axiomtek S7 Lite
