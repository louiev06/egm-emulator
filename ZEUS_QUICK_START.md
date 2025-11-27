# Zeus OS Quick Start Guide

Quick reference for building and using the EGM emulator on Zeus OS / Axiomtek S7 Lite hardware.

## Prerequisites

- Zeus OS (Linux-based gaming platform)
- Axiomtek S7 Lite hardware board
- S7Lite library (`libs7lite.so`) installed
- C++17 compiler (GCC 7+ or ARM cross-compiler)
- CMake 3.15+

## Building on Zeus OS

### 1. Clone/Copy the Project
```bash
cd /home/your-user
cp -r /path/to/egm-emulator-cpp ./
cd egm-emulator-cpp
```

### 2. Build with CMake
```bash
mkdir build && cd build
cmake ..
```

**Expected output:**
```
-- Zeus OS platform detected - enabling Zeus integration
-- Configuring done
-- Generating done
```

```bash
cmake --build .
```

### 3. Run the Simulator
```bash
# May require root for watchdog access
sudo ./egm_simulator
```

## Using Zeus Platform in Code

### Basic Setup

```cpp
#include "megamic/ZeusPlatform.h"
#include "megamic/simulator/Machine.h"
#include "megamic/event/EventService.h"

int main() {
    // Create Zeus platform with watchdog
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

    // Start machine
    machine->start();

    // Main loop with watchdog kicking
    while (running) {
        platform->kickWatchdog();  // Must call every < 30 seconds
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }

    // Cleanup
    machine->stop();
    platform->shutdown();

    return 0;
}
```

### SAS Communication

```cpp
#include "megamic/io/ZeusSerialPort.h"

// Create and open SAS port
auto port = std::make_shared<megamic::io::ZeusSerialPort>("SAS");
if (!port->open()) {
    std::cerr << "Failed to open SAS port" << std::endl;
    return;
}

// Send SAS general poll to address 1
uint8_t txBuffer[] = {0x01, 0x80};  // Address 1, general poll
int written = port->write(txBuffer, 2);

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

### SRAM Persistence

```cpp
// Define your game state structure
struct GameState {
    uint32_t gamesPlayed;
    uint32_t coinIn;
    uint32_t coinOut;
    // ... more fields
};

// Save to SRAM
GameState state = {100, 5000, 3000};
bool success = platform->writeSRAM(
    0,                                      // Offset (in words)
    reinterpret_cast<uint8_t*>(&state),
    sizeof(GameState) / 2                   // Length (in words)
);

// Load from SRAM
GameState loadedState;
success = platform->readSRAM(
    0,                                      // Offset (in words)
    reinterpret_cast<uint8_t*>(&loadedState),
    sizeof(GameState) / 2                   // Length (in words)
);
```

### Battery Monitoring

```cpp
// Check battery status
if (!platform->getBatteryStatus()) {
    std::cerr << "WARNING: Battery is low!" << std::endl;
}

// Get battery voltage
uint16_t voltage = platform->getBatteryVoltage();
std::cout << "Battery: " << voltage << " mV" << std::endl;

// Typical values:
// - Good battery: 3000-3500 mV
// - Low battery: < 2800 mV
```

### Watchdog Thread

```cpp
#include <thread>
#include <atomic>

std::atomic<bool> running{true};

// Create dedicated watchdog thread
std::thread watchdogThread([&platform, &running]() {
    while (running) {
        platform->kickWatchdog();
        std::this_thread::sleep_for(std::chrono::seconds(15));
    }
});

// ... main application logic ...

// Cleanup
running = false;
watchdogThread.join();
```

## Hardware Details

### Serial Port Configuration
- **Device**: `/dev/ttymxc4` (MCU communication)
- **SAS Baud Rate**: 19200 bps
- **Data Format**: 9-bit (8 data + 1 wake bit)
- **Hardware**: Pre-configured by S7Lite library

### SRAM
- **Size**: Query with `platform->getSRAMSize()`
- **Addressing**: Word-based (16-bit units)
- **Maximum read**: 4096 bytes
- **Maximum write**: 512 bytes per block (4 blocks max)

### Watchdog
- **Timeout range**: 0-255 seconds
- **Recommendation**: Set to 30-60 seconds, kick every 10-15 seconds
- **Reset behavior**: System reboots if not kicked in time

## Common Issues

### Issue: "Failed to initialize Zeus platform"
**Cause**: MCU device not accessible or permission denied

**Solution:**
```bash
# Check device
ls -l /dev/ttymxc4

# Add user to dialout group
sudo usermod -a -G dialout $USER

# Reboot or re-login
```

### Issue: No SAS data received
**Cause**: Timeout too short, wrong poll address, or slot machine not connected

**Solution:**
```cpp
// Clear buffers first
port->clearBuffers(true, true);

// Use longer timeout for debugging
port->read(buffer, size, std::chrono::milliseconds(1000));

// Verify write succeeded
int written = port->write(data, size);
if (written != size) {
    std::cerr << "Write failed!" << std::endl;
}
```

### Issue: Watchdog resets system
**Cause**: `kickWatchdog()` not called frequently enough

**Solution:**
```cpp
// Increase timeout
auto platform = std::make_shared<ZeusPlatform>(true, 60);  // 60 seconds

// Kick more frequently (half of timeout)
std::this_thread::sleep_for(std::chrono::seconds(30));
platform->kickWatchdog();
```

### Issue: SRAM read/write fails
**Cause**: Offset out of bounds or length exceeds maximum

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

## Performance Tips

1. **SAS Communication**
   - Typical poll rate: 20-40ms between polls
   - Don't exceed ~50 polls/second
   - Use 20ms timeout for SAS responses

2. **SRAM Access**
   - Batch multiple writes using 4-block structure
   - Cache frequently accessed data in RAM
   - SRAM write is slower than read

3. **Watchdog**
   - Don't kick too frequently (creates overhead)
   - Set timeout to 2-3x your longest operation
   - Use dedicated thread for reliable kicking

4. **Threading**
   - Keep watchdog thread separate from SAS polling
   - Use event-driven architecture to avoid busy-waiting
   - Minimize lock contention on shared resources

## Additional Resources

- **Complete Zeus Integration Guide**: [ZEUS_INTEGRATION.md](ZEUS_INTEGRATION.md)
- **S7Lite API Reference**: `C:\_code\Firmware.Krogoth\axiomtek\s7lite\include\s7lite.h`
- **Implementation Examples**: [ZEUS_INTEGRATION.md](ZEUS_INTEGRATION.md) - Examples section
- **Main README**: [README.md](README.md)
- **Overall Plan**: [PLAN.md](PLAN.md)

---

**Platform**: Zeus OS / Axiomtek S7 Lite
**Status**: Production Ready
**Last Updated**: 2025-11-21
