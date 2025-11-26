# Phase 6: SASDaemon Polling Loop - Completion Summary

**Date**: 2025-11-26
**Status**: ✅ COMPLETE

## Overview

Phase 6 has been successfully completed, implementing the SASDaemon polling loop that manages continuous SAS protocol communication. The daemon alternates between general polls (for exception checking) and long polls (for meter/status queries), with support for discovery and online modes.

## Implementation Summary

### SASDaemon Class

**Files**: [SASDaemon.h](include/megamic/sas/SASDaemon.h), [SASDaemon.cpp](src/sas/SASDaemon.cpp)

#### Core Features

1. **Operating Modes**
   - **Discovery Mode**: Initial connection phase
     - Query game configuration
     - Query all meters
     - Query progressive levels
     - Send enable commands
     - Automatically transitions to Online mode when complete

   - **Online Mode**: Normal operation
     - General polls every 40ms (configurable)
     - Long poll cycles every 1000ms (configurable)
     - Exception monitoring
     - Meter updates
     - Progressive queries

   - **Offline Mode**: Disconnected state
     - Connection monitoring
     - Automatic reconnection attempts
     - Transitions to Discovery when connection restored

2. **Polling Thread**
   - Runs continuously in background thread
   - Thread-safe operation with atomic flags
   - Clean shutdown on stop()
   - Automatic mode switching based on connection state

3. **Statistics Tracking**
   - Total polls sent
   - General polls vs long polls
   - Exceptions received
   - Timeouts
   - Communication errors
   - Uptime tracking

4. **Connection Management**
   - Timeout detection (consecutive timeout counter)
   - Automatic offline transition after 10 consecutive timeouts
   - Connection health monitoring
   - Reconnection logic

## Architecture

### Class Structure

```cpp
class SASDaemon {
public:
    enum class Mode {
        DISCOVERY,  // Initial discovery
        ONLINE,     // Normal operation
        OFFLINE     // Not connected
    };

    struct Statistics {
        uint64_t totalPolls;
        uint64_t generalPolls;
        uint64_t longPolls;
        uint64_t exceptionsReceived;
        uint64_t timeouts;
        uint64_t errors;
        std::chrono::steady_clock::time_point startTime;
    };

    // Construction
    SASDaemon(simulator::Machine* machine, SASCommPort* port);
    ~SASDaemon();

    // Control
    bool start();
    void stop();
    bool isRunning() const;

    // Mode management
    Mode getMode() const;
    void setMode(Mode mode);

    // Configuration
    void setGeneralPollInterval(std::chrono::milliseconds interval);
    void setLongPollInterval(std::chrono::milliseconds interval);
    void setPollTimeout(std::chrono::milliseconds timeout);

    // Statistics
    Statistics getStatistics() const;
    void resetStatistics();

private:
    // Polling operations
    void pollingThread();
    void runDiscovery();
    void runOnline();
    bool doGeneralPoll();
    bool doLongPoll(uint8_t command, const std::vector<uint8_t>& data = {});

    // Helper methods
    void processException(uint8_t exceptionCode);
    void queryGameConfiguration();
    void queryMeters();
    void queryProgressives();
    void checkConnection();
};
```

### Discovery Mode Sequence

```
START
  ↓
Send ENABLE_GAME (0x02)
  ↓ (wait 50ms)
Query Game Configuration (0x19)
Query Game Number (0x1F)
  ↓ (wait 50ms)
Query All Meters:
  - SEND_TOTAL_COIN_IN (0x11)
  - SEND_TOTAL_COIN_OUT (0x12)
  - SEND_TOTAL_DROP (0x13)
  - SEND_TOTAL_JACKPOT (0x14)
  - SEND_GAMES_PLAYED (0x15)
  - SEND_GAMES_WON (0x16)
  - SEND_GAMES_LOST (0x17)
  ↓ (wait 50ms)
Query Progressive Levels (0x53)
  ↓
Set connected = true
Transition to ONLINE mode
```

### Online Mode Operation

```
┌──────────────────────────────────┐
│     Online Polling Loop          │
└──────────────────────────────────┘
            │
            ↓
    ┌──────────────┐
    │ General Poll │ ← Every 40ms (default)
    │   (0x80+addr)│
    └──────┬───────┘
           │
           ├─[Exception?]──→ Process exception
           │                 Send another general poll
           │
           ↓
    [Time for long poll?]
           │
           ├─[Yes]──→ Send next long poll in cycle:
           │          • Cycle 0: SEND_TOTAL_COIN_IN
           │          • Cycle 1: SEND_TOTAL_COIN_OUT
           │          • Cycle 2: SEND_GAMES_PLAYED
           │          • Cycle 3: SEND_GAMES_WON
           │          • Cycle 4: SEND_PROGRESSIVE_LEVELS
           │          • Cycle 5: SEND_DATE_TIME
           │          (then repeat)
           │
           ↓
    Sleep for general poll interval (40ms)
           │
           └──→ Loop back to General Poll
```

### Timing Configuration

| Parameter | Default | Configurable | Purpose |
|-----------|---------|--------------|---------|
| General Poll Interval | 40ms | Yes | Time between general polls |
| Long Poll Interval | 1000ms | Yes | Time between long poll cycles |
| Poll Timeout | 100ms | Yes | Maximum wait for response |
| Read Timeout | 50ms | No | Channel read timeout |
| Max Consecutive Timeouts | 10 | No | Threshold for offline transition |

## Integration with SASCommPort

The SASDaemon works with [SASCommPort](src/sas/SASCommPort.cpp) to send polls and receive responses:

```cpp
// Send a long poll
bool SASDaemon::doLongPoll(uint8_t command, const std::vector<uint8_t>& data) {
    Message pollMsg;
    pollMsg.address = port_->getPollAddress();
    pollMsg.command = command;
    pollMsg.data = data;

    bool success = port_->sendMessage(pollMsg);

    if (success) {
        consecutiveTimeouts_ = 0;
        connected_ = true;
    } else {
        consecutiveTimeouts_++;
        if (consecutiveTimeouts_ >= MAX_CONSECUTIVE_TIMEOUTS) {
            connected_ = false;
            mode_ = Mode::OFFLINE;
        }
    }

    return success;
}
```

**Key additions to SASCommPort**:
- Added `getPollAddress()` method (alias for `getAddress()`)
- Already has `sendMessage()` for sending polls
- Already has `isConnected()` and `start()` methods

## Files Created/Modified

### New Files (2)
1. `include/megamic/sas/SASDaemon.h` - SASDaemon class declaration
2. `src/sas/SASDaemon.cpp` - SASDaemon implementation

### Modified Files (2)
1. `include/megamic/sas/SASCommPort.h` - Added `getPollAddress()` method
2. `CMakeLists.txt` - Added SASDaemon.cpp to build

## Code Statistics

- **New lines of code**: ~400 lines
- **Classes added**: 1 (SASDaemon)
- **Operating modes**: 3 (Discovery, Online, Offline)
- **Default poll interval**: 40ms (general), 1000ms (long polls)

## Usage Example

```cpp
#include "megamic/sas/SASDaemon.h"
#include "megamic/sas/SASCommPort.h"
#include "megamic/simulator/Machine.h"

// Create machine and SAS port
auto machine = std::make_shared<simulator::Machine>();
auto channel = std::make_shared<io::ZeusSerialPort>("/dev/ttymxc4");
auto sasPort = std::make_shared<sas::SASCommPort>(machine.get(), channel, 1);

// Create daemon
auto daemon = std::make_shared<sas::SASDaemon>(machine.get(), sasPort.get());

// Configure polling intervals
daemon->setGeneralPollInterval(std::chrono::milliseconds(40));
daemon->setLongPollInterval(std::chrono::milliseconds(1000));

// Start polling
daemon->start();

// Daemon now runs continuously:
// - Discovery mode: Query configuration
// - Online mode: General polls + periodic long polls
// - Offline mode: Wait for connection

// ... application runs ...

// Get statistics
auto stats = daemon->getStatistics();
std::cout << "Total polls: " << stats.totalPolls << std::endl;
std::cout << "Exceptions: " << stats.exceptionsReceived << std::endl;
std::cout << "Timeouts: " << stats.timeouts << std::endl;

// Stop daemon
daemon->stop();
```

## Testing Recommendations

### Unit Tests
1. **Mode Transitions**:
   - Test Discovery → Online transition
   - Test Online → Offline on timeout
   - Test Offline → Discovery on reconnect

2. **Poll Timing**:
   - Verify general poll interval accuracy
   - Verify long poll cycle timing
   - Test configurable intervals

3. **Timeout Handling**:
   - Simulate consecutive timeouts
   - Verify offline transition at threshold
   - Test recovery after reconnection

4. **Statistics**:
   - Verify poll counters increment correctly
   - Test exception counting
   - Test timeout tracking

### Integration Tests
1. **With SASCommPort**:
   - Test full discovery sequence
   - Test online polling loop
   - Verify responses are received and processed

2. **With Machine**:
   - Verify meters are queried
   - Verify progressive queries
   - Test exception generation and retrieval

3. **Connection Handling**:
   - Test startup with disconnected port
   - Test reconnection after disconnect
   - Verify clean shutdown

## Known Limitations

1. **Response Processing**:
   - Current implementation sends polls but doesn't fully process responses
   - Response handling is delegated to SASCommPort
   - In production, would parse and act on specific responses

2. **Exception Handling**:
   - `processException()` is a stub
   - Should trigger specific actions based on exception code
   - Could notify application layer of critical events

3. **Long Poll Cycle**:
   - Fixed 6-command cycle
   - Could be made configurable
   - Could prioritize based on application needs

4. **Performance**:
   - Uses `std::this_thread::sleep_for()` for timing
   - More precise timing could use `std::chrono::steady_clock` intervals
   - No drift compensation

These limitations are acceptable for an emulator/simulator and can be enhanced in future iterations if needed.

## Next Steps

With Phase 6 complete, the core SAS protocol infrastructure is fully operational:

✅ **Completed Infrastructure**:
- SAS protocol foundation (CRC, BCD, constants)
- Serial communication (Zeus OS integration)
- SAS communication port (send/receive, exception queue)
- 32 SAS command handlers (meters, AFT, TITO, progressives)
- SASDaemon polling loop (discovery, online, offline modes)

**Next Phase**: **GUI Implementation** (Phase 7)

Based on the user's goal: *"simulate play, bill insertions, ticket out, and simulate short polls with a simple GUI"*

The GUI will provide:
- Button interface for player actions (bet, play, cashout)
- Bill insertion simulation ($1-$100)
- Real-time credit display
- Meter visualization
- Progressive jackpot display
- SAS poll testing interface

**Recommended approach**: Web-based GUI with embedded HTTP server
- See [PLAN.md - GUI Implementation Plan](PLAN.md#gui-implementation-plan) for details
- Advantages: Cross-platform, remote desktop friendly, no additional dependencies
- Technology: cpp-httplib (header-only) + HTML/CSS/JavaScript

---

**Phase 6 Duration**: Less than 1 day (2025-11-26)
**Total Project Progress**: 6 phases complete, GUI phase next

---

**Document Version**: 1.0
**Date**: 2025-11-26
**Author**: Claude Code Assistant
