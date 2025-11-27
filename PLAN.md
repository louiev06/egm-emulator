# EGM Emulator C++ - Development Plan

This document outlines the complete roadmap for implementing a production-ready SAS protocol EGM emulator in C++.

## Project Goal

**Primary Objective**: Create a GUI-based EGM simulator for Zeus OS that can:
- ✅ Simulate game play (place bets, play games, win/lose)
- ✅ Simulate bill insertions ($1, $5, $10, $20, $50, $100)
- ✅ Simulate ticket out (TITO - print tickets)
- ✅ Respond to SAS polls (general polls for exceptions, long polls for meters/config)
- ✅ Display real-time state: Credits, meters, game info, door status
- ✅ Run on Zeus OS device OR remotely via Google Remote Desktop
- ✅ Simple, intuitive GUI for testing and demonstration

**Use Case**: Testing and validation tool for SAS communication, game logic, and Zeus OS integration.

## Current Status

✅ **Phase 1: Core Architecture** (COMPLETE - 2025-11-21)
- Machine simulator with state management
- Game configuration and management
- Event system (publish/subscribe - C++11 compatible)
- Platform abstraction layer
- Meter tracking (20+ meters)
- Progressive jackpot framework
- Credit management (cashable/restricted/non-restricted)
- Build system (CMake + shell script)
- Comprehensive documentation
- Zeus OS integration complete

✅ **Phase 2: SAS Protocol Foundation** (COMPLETE - 2025-11-24)
- CRC-16 calculation (with lookup table)
- BCD encoding/decoding utilities
- SAS command definitions (100+ commands)
- MachineCommPort base class
- SASCommPort implementation (receive thread, CRC validation)
- Machine integration (addSASPort, getPrimarySASPort)
- See [PHASE2_COMPLETE.md](PHASE2_COMPLETE.md) for details

✅ **Phase 5: SAS Command Handlers** (COMPLETE - 2025-11-26)
- ✅ Meter commands (9 commands: Send Coin In/Out, Games Played/Won/Lost, Drop, Jackpot, etc.)
- ✅ Enable/Disable commands (4 commands: Game control, bill acceptor)
- ✅ Date/Time commands (2 commands: Send/Set date and time)
- ✅ Exception handling (General poll, door events, game events, errors)
- ✅ TITO commands (5 commands: Validation, redemption, ticket info)
- ✅ AFT commands (5 commands: Register lock, transfer funds, interrogate status)
- ✅ Progressive jackpot commands (4 commands: Amount, win, levels, broadcast)
- **Total: 32 SAS long poll commands implemented**
- See implementation in `src/sas/commands/` and routing in [SASCommPort.cpp](src/sas/SASCommPort.cpp)
- See [PHASE5_COMPLETE.md](PHASE5_COMPLETE.md) for detailed documentation

✅ **Phase 6: SASDaemon Polling Loop** (COMPLETE - 2025-11-26)
- ✅ Core polling loop with continuous thread
- ✅ Discovery mode (query configuration, meters, progressives)
- ✅ Online mode (general polls + periodic long polls)
- ✅ Connection management and timeout handling
- ✅ Statistics tracking (polls sent, exceptions, timeouts)
- ✅ Configurable poll intervals (general: 40ms, long: 1000ms)
- ✅ Three operating modes: Discovery, Online, Offline
- See implementation in [SASDaemon.h](include/megamic/sas/SASDaemon.h) and [SASDaemon.cpp](src/sas/SASDaemon.cpp)

🔄 **Phase 7: GUI Implementation** (NEXT - User Goal)
- Implement simple GUI for EGM simulation and testing
- **GUI Requirements**:
  - ✅ Simulate button presses (bet, play, cashout)
  - ✅ Simulate bill insertions ($1, $5, $10, $20, $50, $100)
  - ✅ Display current credits in real-time
  - ✅ Show meter values (coin in, coin out, games played, etc.)
  - ✅ Trigger ticket out (TITO cashout)
  - ✅ Test SAS polls (send manual polls, view responses)
  - ✅ Display machine status (enabled/disabled, door status)
  - ✅ Progressive jackpot display
  - ✅ Run on Zeus OS device OR remotely via Google Remote Desktop
- **Technology Options**:
  - Web-based GUI (HTML/CSS/JavaScript with embedded web server)
  - Qt5 (native, cross-platform)
  - GTK+ (Linux-focused)
- See "GUI Implementation Plan" section below for details

## Remaining Work

---

## ~~Phase 2: SAS Protocol Foundation~~ ✅ COMPLETE

### 2.1 SAS Constants and Utilities

#### 2.1.1 CRC-16 Implementation
**Priority**: HIGH
**Effort**: 2-3 days
**Files**:
- `include/megamic/sas/CRC16.h`
- `src/sas/CRC16.cpp`

**Requirements**:
- Implement SAS-specific CRC-16 algorithm
- Polynomial: 0x8005 (x^16 + x^15 + x^2 + 1)
- Initial value: 0x0000
- Calculate CRC for message validation
- Verify CRC on received messages

**Deliverables**:
```cpp
class CRC16 {
public:
    static uint16_t calculate(const uint8_t* data, size_t length);
    static bool verify(const uint8_t* data, size_t length, uint16_t expectedCrc);
};
```

#### 2.1.2 BCD Encoding/Decoding
**Priority**: HIGH
**Effort**: 1-2 days
**Files**:
- `include/megamic/sas/BCD.h`
- `src/sas/BCD.cpp`

**Requirements**:
- Convert binary to BCD (Binary-Coded Decimal)
- Convert BCD to binary
- Handle multi-byte BCD values
- Support meter values (up to 10 digits)
- Validate BCD format

**Deliverables**:
```cpp
class BCD {
public:
    static std::vector<uint8_t> encode(uint64_t value, size_t bytes);
    static uint64_t decode(const uint8_t* bcdData, size_t bytes);
    static bool isValid(const uint8_t* bcdData, size_t bytes);
};
```

#### 2.1.3 SAS Command Definitions
**Priority**: HIGH
**Effort**: 3-4 days
**Files**:
- `include/megamic/sas/SASCommands.h`
- `src/sas/SASCommands.cpp`

**Requirements**:
- Define all SAS command codes (0x00-0xFF)
- Long poll commands
- General poll commands
- Multi-part commands
- Command parameter structures
- Response structures

**Deliverables**:
```cpp
namespace SASCommands {
    // Long poll commands
    constexpr uint8_t SEND_METERS = 0x10;
    constexpr uint8_t SEND_GAME_CONFIG = 0x1F;
    constexpr uint8_t AFT_TRANSFER_FUNDS = 0x72;
    constexpr uint8_t TITO_VALIDATION = 0x7B;
    // ... 100+ commands

    struct Command {
        uint8_t code;
        std::vector<uint8_t> data;
        uint16_t crc;
    };

    struct Response {
        std::vector<uint8_t> data;
        uint16_t crc;
    };
}
```

---

## Phase 3: Serial Communication

### 3.1 Serial Port Implementation

#### 3.1.1 Linux Serial Port (termios)
**Priority**: HIGH
**Effort**: 4-5 days
**Files**:
- `include/megamic/io/LinuxSerialPort.h`
- `src/io/LinuxSerialPort.cpp`

**Requirements**:
- POSIX termios API implementation
- 19200 baud rate
- MARK parity (space parity with inversion)
- 8 data bits, 1 stop bit
- No hardware flow control
- Timeout support (read/write)
- Non-blocking I/O option
- Error handling

**Deliverables**:
```cpp
class LinuxSerialPort : public CommChannel {
public:
    LinuxSerialPort(const std::string& device); // e.g., "/dev/ttyS0"
    bool open() override;
    void close() override;
    bool isOpen() const override;
    int read(uint8_t* buffer, int maxBytes, std::chrono::milliseconds timeout) override;
    int write(const uint8_t* buffer, int numBytes) override;
    void flush() override;
    std::string getName() const override;

    // SAS-specific configuration
    void configureSAS(); // 19200, MARK parity, 8N1
};
```

#### 3.1.2 Windows Serial Port (Win32 API)
**Priority**: MEDIUM
**Effort**: 4-5 days
**Files**:
- `include/megamic/io/WindowsSerialPort.h`
- `src/io/WindowsSerialPort.cpp`

**Requirements**:
- Win32 CreateFile/ReadFile/WriteFile API
- DCB (Device Control Block) configuration
- Same SAS parameters as Linux
- OVERLAPPED I/O for async operations
- Timeout support
- Error handling (GetLastError)

**Deliverables**:
```cpp
class WindowsSerialPort : public CommChannel {
    // Similar interface to LinuxSerialPort
    HANDLE hComm_;
    DCB dcbSerialParams_;
    COMMTIMEOUTS timeouts_;
};
```

#### 3.1.3 Serial Port Factory
**Priority**: MEDIUM
**Effort**: 1 day
**Files**:
- Update `ICardPlatform.h/cpp`

**Requirements**:
- Platform detection (#ifdef _WIN32, __linux__)
- Automatic port selection
- Configuration from environment/config file

**Deliverables**:
```cpp
class Pal264Platform : public ICardPlatform {
public:
    std::shared_ptr<io::CommChannel> createSASPort() override {
#ifdef __linux__
        return std::make_shared<io::LinuxSerialPort>("/dev/ttyS0");
#elif _WIN32
        return std::make_shared<io::WindowsSerialPort>("COM1");
#endif
    }
};
```

---

## Phase 4: SAS Communication Port

### 4.1 SASCommPort Implementation

#### 4.1.1 Base SASCommPort Class
**Priority**: HIGH
**Effort**: 5-7 days
**Files**:
- `include/megamic/sas/SASCommPort.h`
- `src/sas/SASCommPort.cpp`

**Requirements**:
- Inherit from MachineCommPort
- Manage serial port communication
- Send/receive SAS messages
- CRC validation
- Message framing (address byte + command + data + CRC)
- Response timeout handling (default: 20ms)
- Poll address support (0x01-0x7F)
- Thread-safe send/receive

**Deliverables**:
```cpp
class SASCommPort : public MachineCommPort {
public:
    SASCommPort(Machine* machine,
                std::shared_ptr<io::CommChannel> port,
                std::shared_ptr<event::EventService> eventService);

    // MachineCommPort interface implementation
    void gameSelected() override;
    void setMultigame(bool multigame) override;
    void progressiveHit(int levelId) override;
    void progressivePaid() override;
    void start() override;
    void stop() override;
    void gameStarted(int credits, double denom) override;
    void gameEnded() override;
    void doorOpen() override;
    void doorClose() override;
    void cashoutButton() override;
    void lightOn() override;
    void lightOff() override;
    void hopperLow() override;
    void setConnected(bool connected) override;
    bool isConnected() override;
    void handpayPending(int levelId, double amount) override;
    void resetOldestHandpay() override;
    void ramClear() override;
    void optionsChanged() override;
    std::string getPortType() const override { return "SAS"; }

    // SAS-specific methods
    bool sendCommand(const SASCommands::Command& cmd);
    bool receiveResponse(SASCommands::Response& response, std::chrono::milliseconds timeout);
    void setPollAddress(uint8_t address);
    uint8_t getPollAddress() const;

private:
    std::shared_ptr<io::CommChannel> port_;
    std::shared_ptr<event::EventService> eventService_;
    uint8_t pollAddress_;
    std::atomic<bool> connected_;
    std::mutex sendMutex_;
    std::mutex receiveMutex_;
};
```

#### 4.1.2 Exception Queue
**Priority**: HIGH
**Effort**: 3-4 days
**Files**:
- `include/megamic/sas/ExceptionQueue.h`
- `src/sas/ExceptionQueue.cpp`

**Requirements**:
- FIFO queue for SAS exceptions (events)
- Priority exceptions (general poll 0x80-0x9F)
- Exception codes (0x00-0x7F)
- Thread-safe queue operations
- Maximum queue size (typically 16-32 entries)
- Exception buffer management

**Deliverables**:
```cpp
class ExceptionQueue {
public:
    void addException(uint8_t exceptionCode, const std::vector<uint8_t>& data = {});
    bool hasException() const;
    uint8_t getNextException();
    std::vector<uint8_t> getExceptionData(uint8_t code);
    void clear();
    size_t size() const;

    // Priority exceptions
    bool hasPriorityException() const;
    uint8_t getNextPriorityException();

private:
    std::queue<uint8_t> exceptions_;
    std::queue<uint8_t> priorityExceptions_;
    std::map<uint8_t, std::vector<uint8_t>> exceptionData_;
    std::mutex mutex_;
    static constexpr size_t MAX_QUEUE_SIZE = 32;
};
```

---

## Phase 5: SAS Command Handlers

### 5.1 Core Command Implementation

#### 5.1.1 Meter Commands
**Priority**: HIGH
**Effort**: 5-6 days
**Files**:
- `include/megamic/sas/commands/MeterCommands.h`
- `src/sas/commands/MeterCommands.cpp`

**Commands to implement**:
- `0x10` - Send Meters 0-9 (coin in, coin out, drop, jackpot, games played)
- `0x11` - Send Meters 10-19
- `0x12` - Send Meters 20-29
- `0x13` - Send Meters 30-39
- `0x1F` - Send Game Configuration and Meters
- `0x2F` - Send Selected Meters for Game N
- `0x6F` - Send Multiple Meters

**Requirements**:
- Read meters from Machine object
- Format as BCD
- Calculate and append CRC
- Handle meter rollover (10 digits max)

**Deliverables**:
```cpp
class MeterCommands {
public:
    static SASCommands::Response handleSendMeters(Machine* machine, uint8_t meterGroup);
    static SASCommands::Response handleSendGameConfig(Machine* machine);
    static SASCommands::Response handleSendSelectedMeters(Machine* machine,
                                                          const std::vector<uint8_t>& meterCodes);
};
```

#### 5.1.2 Game Configuration Commands
**Priority**: HIGH
**Effort**: 4-5 days
**Files**:
- `include/megamic/sas/commands/GameConfigCommands.h`
- `src/sas/commands/GameConfigCommands.cpp`

**Commands to implement**:
- `0x1F` - Send Game Configuration
- `0x51` - Send Number of Games Implemented
- `0x52` - Send Game N Configuration
- `0x53` - Send Game N Meters
- `0x54` - Send Selected Game N Meters
- `0x56` - Send Enabled Game Numbers
- `0x76` - Send Enabled Features

**Requirements**:
- Report game count
- Report denomination codes
- Report max bet
- Report paytable ID
- Report game options

#### 5.1.3 Enable/Disable Commands
**Priority**: HIGH
**Effort**: 2-3 days
**Files**:
- `include/megamic/sas/commands/EnableCommands.h`
- `src/sas/commands/EnableCommands.cpp`

**Commands to implement**:
- `0x02` - Enable Gaming Machine
- `0x01` - Disable Gaming Machine
- `0x80` - General Poll (ACK when enabled)

**Requirements**:
- Set machine enabled state
- Update playability
- Generate exceptions

#### 5.1.4 Exception/Event Commands
**Priority**: HIGH
**Effort**: 6-7 days
**Files**:
- `include/megamic/sas/commands/ExceptionCommands.h`
- `src/sas/commands/ExceptionCommands.cpp`

**Commands to implement**:
- `0x80-0x9F` - General Poll (read priority exceptions)
- `0x7E` - Send Gaming Machine ID
- `0x7F` - Send Current Date and Time
- `0x31` - Send Last Exception
- `0x38` - Enter/Exit Maintenance Mode

**Exception codes to support**:
- `0x11` - Slot door was opened
- `0x12` - Slot door was closed
- `0x13` - Drop door was opened
- `0x14` - Drop door was closed
- `0x15` - Card cage was opened
- `0x16` - Card cage was closed
- `0x19` - AC power was applied
- `0x1A` - AC power was lost
- `0x1B` - Cashbox door was opened
- `0x1C` - Cashbox door was closed
- `0x51` - Handpay is pending
- `0x52` - Handpay was reset
- `0x54` - Bonus pay was awarded
- `0x60-0x67` - Game has started/ended
- `0x68` - Game recall entry has been made

---

### 5.2 Progressive Commands

#### 5.2.1 Progressive Configuration
**Priority**: HIGH
**Effort**: 4-5 days
**Files**:
- `include/megamic/sas/commands/ProgressiveCommands.h`
- `src/sas/commands/ProgressiveCommands.cpp`

**Commands to implement**:
- `0x85` - Send Progressive Level Information
- `0x86` - Send Progressive Win Amount
- `0x87` - Send Progressive Group ID
- `0x8A` - Send SAS Progressive Win Amount
- `0x84` - Progressive Broadcast

**Requirements**:
- Report configured progressive levels
- Broadcast progressive amounts
- Handle progressive hits
- Support up to 32 levels
- Group ID support

#### 5.2.2 Legacy Bonus Commands
**Priority**: MEDIUM
**Effort**: 3-4 days

**Commands to implement**:
- `0x8B` - Legacy Bonus Pay
- `0x8C` - Legacy Bonus Deduction

**Requirements**:
- Award bonus credits
- Deduct bonus credits
- Generate events
- ACK/NACK responses

---

### 5.3 AFT (Account Funds Transfer) Commands

#### 5.3.1 AFT Transfer Commands
**Priority**: HIGH
**Effort**: 7-10 days
**Files**:
- `include/megamic/sas/commands/AFTCommands.h`
- `src/sas/commands/AFTCommands.cpp`

**Commands to implement**:
- `0x72` - AFT Transfer Funds
- `0x73` - AFT Registration
- `0x74` - AFT Transfer Status
- `0x75` - AFT Lock
- `0x76` - AFT Unlock

**Requirements**:
- Transfer cashable credits
- Transfer restricted credits
- Transfer non-restricted credits
- Lock/unlock machine
- Registration with host
- Transaction IDs
- Receipt data
- Asset number support

**Deliverables**:
```cpp
class AFTCommands {
public:
    enum class TransferType {
        IN_CASHABLE = 0x00,
        OUT_CASHABLE = 0x01,
        IN_RESTRICTED = 0x10,
        OUT_RESTRICTED = 0x11,
        IN_NON_RESTRICTED = 0x20,
        WIN_FROM_GAME = 0x80
    };

    struct AFTTransferRequest {
        TransferType type;
        uint64_t amount;
        uint32_t transactionId;
        std::vector<uint8_t> receiptData;
    };

    static SASCommands::Response handleAFTTransfer(Machine* machine,
                                                    const AFTTransferRequest& request);
    static SASCommands::Response handleAFTLock(Machine* machine, bool lock);
    static SASCommands::Response handleAFTStatus(Machine* machine);
};
```

#### 5.3.2 AFT Transaction Management
**Priority**: HIGH
**Effort**: 3-4 days
**Files**:
- `include/megamic/sas/AFTTransaction.h`
- `src/sas/AFTTransaction.cpp`

**Requirements**:
- Transaction history (last 5-10 transactions)
- Transaction states (pending, complete, failed)
- Transaction IDs
- Amounts and types
- Timestamps

---

### 5.4 TITO (Ticket In/Ticket Out) Commands

#### 5.4.1 Ticket Validation
**Priority**: HIGH
**Effort**: 6-8 days
**Files**:
- `include/megamic/sas/commands/TITOCommands.h`
- `src/sas/commands/TITOCommands.cpp`

**Commands to implement**:
- `0x70` - Send Ticket Validation Number
- `0x71` - Redeem Ticket
- `0x7B` - Extended Ticket Validation
- `0x7C` - Cashout Ticket

**Requirements**:
- Validation number generation (18 digits)
- Ticket redemption
- Restricted/non-restricted ticket support
- Ticket expiration
- Ticket pool ID

**Deliverables**:
```cpp
class TITOCommands {
public:
    struct ValidationNumber {
        uint64_t value;  // 18-digit BCD
        uint8_t poolId;
        uint32_t timestamp;
    };

    struct TicketData {
        ValidationNumber validationNumber;
        uint64_t amount;
        bool isRestricted;
        uint32_t expirationDate;
    };

    static ValidationNumber generateValidationNumber();
    static SASCommands::Response handleTicketRedemption(Machine* machine,
                                                        const ValidationNumber& validation);
    static SASCommands::Response handleCashout(Machine* machine);
};
```

#### 5.4.2 Enhanced Validation
**Priority**: MEDIUM
**Effort**: 4-5 days

**Requirements**:
- System validation
- Secure enhanced validation
- Ticket expiration dates
- Multiple ticket pool support

---

### 5.5 Real-Time Event Polling

#### 5.5.1 Real-Time Event Configuration
**Priority**: MEDIUM
**Effort**: 5-6 days
**Files**:
- `include/megamic/sas/RealTimeEvents.h`
- `src/sas/RealTimeEvents.cpp`

**Commands to implement**:
- `0x7D` - Enable Real-Time Event Reporting
- `0x7E` - Send Real-Time Event Data

**Requirements**:
- Event masks (configure which events to report)
- Event data structures
- Game recall data
- Coin-in/coin-out events
- Game play events
- Door events

---

## Phase 6: Polling Daemon

### 6.1 SASDaemon Implementation

#### 6.1.1 Core Polling Loop
**Priority**: HIGH
**Effort**: 7-10 days
**Files**:
- `include/megamic/sas/SASDaemon.h`
- `src/sas/SASDaemon.cpp`

**Requirements**:
- Continuous polling thread
- General poll (0x80) - check for exceptions
- Long poll cycle - query meters/status
- Poll timing (typical: 20-40ms between polls)
- Service timeout (3 seconds for long polls)
- Exception priority handling
- Host communication integration

**Deliverables**:
```cpp
class SASDaemon {
public:
    SASDaemon(Machine* machine,
              std::shared_ptr<SASCommPort> port,
              std::shared_ptr<event::EventService> eventService);

    void start();
    void stop();
    bool isRunning() const;

    // Polling configuration
    void setPollInterval(std::chrono::milliseconds interval);
    void setServiceTimeout(std::chrono::milliseconds timeout);

private:
    void pollingThread();
    void handleGeneralPoll();
    void handleLongPoll();
    void processException(uint8_t exceptionCode);

    Machine* machine_;
    std::shared_ptr<SASCommPort> port_;
    std::shared_ptr<event::EventService> eventService_;
    std::unique_ptr<std::thread> pollingThread_;
    std::atomic<bool> running_;
    std::chrono::milliseconds pollInterval_;
    std::chrono::milliseconds serviceTimeout_;
};
```

#### 6.1.2 Discovery Mode
**Priority**: MEDIUM
**Effort**: 4-5 days

**Requirements**:
- Auto-detect game presence
- Query game configuration
- Enumerate enabled games
- Detect multi-game vs single-game
- Detect denomination support
- Detect enabled features

#### 6.1.3 Online Mode
**Priority**: HIGH
**Effort**: 3-4 days

**Requirements**:
- Continuous meter polling
- Exception monitoring
- Progressive updates
- Event generation
- Status reporting

---

## Phase 7: OneLink Integration

### 7.1 XML Schema Support

#### 7.1.1 XML Parser Integration
**Priority**: MEDIUM
**Effort**: 3-4 days
**Dependencies**: External XML library (TinyXML2 or RapidXML)

**Requirements**:
- Parse OneLink XML messages
- Serialize Machine state to XML
- Configuration messages
- Event messages
- Status messages

#### 7.1.2 OneLink Message Types
**Priority**: MEDIUM
**Effort**: 5-7 days

**Messages to implement**:
- Configuration request/response
- Status update
- Event publication
- Progressive level updates
- Game metrics

---

### 7.2 Network Communication

#### 7.2.1 TCP Client (Host Channel)
**Priority**: MEDIUM
**Effort**: 4-5 days
**Files**:
- `include/megamic/network/TCPClient.h`
- `src/network/TCPClient.cpp`

**Requirements**:
- TCP socket connection
- Reconnection logic
- Heartbeat/keepalive
- Message framing
- Thread-safe send/receive

#### 7.2.2 UDP Multicast (Progressive Updates)
**Priority**: LOW
**Effort**: 3-4 days
**Files**:
- `include/megamic/network/MulticastClient.h`
- `src/network/MulticastClient.cpp`

**Requirements**:
- Subscribe to multicast group
- Receive progressive level updates
- Parse and apply updates
- Handle network failures

#### 7.2.3 SSL/TLS Support
**Priority**: LOW
**Effort**: 3-4 days
**Dependencies**: OpenSSL or mbedTLS

**Requirements**:
- SSL/TLS encryption
- Certificate validation
- Secure connection establishment

---

## Phase 8: Advanced Features

### 8.1 Persistence

#### 8.1.1 State Serialization
**Priority**: MEDIUM
**Effort**: 4-5 days
**Files**:
- `include/megamic/persistence/StateSerializer.h`
- `src/persistence/StateSerializer.cpp`

**Requirements**:
- Save Machine state to disk
- Load Machine state on startup
- Handle corrupted data
- Version compatibility
- Atomic writes

**Storage locations**:
- `/var/lib/megamic/icard.dat` (Linux)
- `C:\ProgramData\MegaMIC\icard.dat` (Windows)

**Data to persist**:
- Device ID
- Asset number
- Game list
- Meter values
- Progressive configuration
- Last transaction IDs

#### 8.1.2 Configuration Files
**Priority**: MEDIUM
**Effort**: 2-3 days

**Requirements**:
- INI or JSON configuration
- Serial port settings
- Network settings
- Poll address
- Logging level

---

### 8.2 Logging

#### 8.2.1 Logging Framework Integration
**Priority**: MEDIUM
**Effort**: 2-3 days
**Dependencies**: spdlog (recommended)

**Requirements**:
- Multiple log levels (trace, debug, info, warn, error)
- Console and file logging
- Rotating log files
- Structured logging
- Performance (async logging)

**Deliverables**:
```cpp
// Usage
LOG_INFO("Machine started, poll address: {}", pollAddress);
LOG_DEBUG("Received command: 0x{:02X}", cmdCode);
LOG_ERROR("Serial port error: {}", error);
```

#### 8.2.2 Diagnostic Logging
**Priority**: LOW
**Effort**: 2-3 days

**Requirements**:
- SAS command trace
- Protocol analyzer output
- Ring buffer for recent events
- Dump to file on error

---

### 8.3 Web Console

#### 8.3.1 HTTP Server
**Priority**: LOW
**Effort**: 5-7 days
**Dependencies**: Boost.Beast or cpp-httplib

**Requirements**:
- Lightweight HTTP server
- REST API endpoints
- JSON responses
- CORS support

**Endpoints**:
- `GET /api/status` - Machine status
- `GET /api/meters` - All meters
- `GET /api/games` - Game list
- `GET /api/progressives` - Progressive levels
- `GET /api/events` - Recent events
- `POST /api/reboot` - Reboot machine

#### 8.3.2 Web UI
**Priority**: LOW
**Effort**: 7-10 days

**Requirements**:
- Static HTML/CSS/JS
- Real-time updates (WebSocket or SSE)
- Meter display
- Event log
- Configuration viewer
- Protocol trace viewer

---

### 8.4 Watchdog and Monitoring

#### 8.4.1 System Watchdog
**Priority**: MEDIUM
**Effort**: 2-3 days

**Requirements**:
- Monitor polling daemon health
- Detect hung threads
- Automatic restart on failure
- Heartbeat updates

#### 8.4.2 Network Monitoring
**Priority**: LOW
**Effort**: 2-3 days

**Requirements**:
- Detect network connectivity loss
- Report to OneLink
- Attempt reconnection
- Offline mode

---

## Phase 9: Testing and Validation

### 9.1 Unit Tests

#### 9.1.1 Test Framework Setup
**Priority**: HIGH
**Effort**: 1-2 days
**Dependencies**: Google Test

**Requirements**:
- CMake integration
- Test fixtures
- Mock objects

**Files**:
- `tests/CMakeLists.txt`
- `tests/test_machine.cpp`
- `tests/test_sas_commands.cpp`
- `tests/test_crc.cpp`
- `tests/test_bcd.cpp`

#### 9.1.2 Core Component Tests
**Priority**: HIGH
**Effort**: 5-7 days

**Tests to write**:
- Machine state transitions
- Meter tracking and rollover
- Progressive management
- Credit management
- Event system
- CRC calculations
- BCD encoding/decoding

#### 9.1.3 SAS Protocol Tests
**Priority**: HIGH
**Effort**: 7-10 days

**Tests to write**:
- Command parsing
- Response generation
- Exception queue
- AFT transactions
- TITO validation
- Message framing

---

### 9.2 Integration Tests

#### 9.2.1 SAS Protocol Compliance
**Priority**: HIGH
**Effort**: 10-14 days

**Requirements**:
- Test against SAS specification
- Test with real hardware (if available)
- Test with SAS simulator/emulator
- Verify timing requirements
- Verify CRC calculations
- Verify all mandatory commands

#### 9.2.2 Stress Testing
**Priority**: MEDIUM
**Effort**: 3-5 days

**Requirements**:
- High-frequency polling
- Large meter values
- Many exceptions queued
- Long-running stability
- Memory leak detection (valgrind)

---

### 9.3 System Tests

#### 9.3.1 End-to-End Testing
**Priority**: MEDIUM
**Effort**: 5-7 days

**Requirements**:
- Full OneLink integration
- Real game play scenarios
- Progressive hits
- AFT transfers
- TITO cashouts
- Error recovery

#### 9.3.2 Certification Testing
**Priority**: LOW (if needed)
**Effort**: Variable

**Requirements**:
- GLI compliance testing
- Gaming Labs certification
- Jurisdiction-specific testing

---

## Phase 10: Documentation and Deployment

### 10.1 Technical Documentation

#### 10.1.1 API Documentation
**Priority**: MEDIUM
**Effort**: 3-4 days
**Tools**: Doxygen

**Requirements**:
- Generate from source comments
- Class diagrams
- Call graphs
- Example code

#### 10.1.2 Architecture Documentation
**Priority**: MEDIUM
**Effort**: 2-3 days

**Documents**:
- System architecture overview
- Threading model
- State machine diagrams
- Protocol flow diagrams
- Sequence diagrams

#### 10.1.3 User Manual
**Priority**: LOW
**Effort**: 3-5 days

**Content**:
- Installation guide
- Configuration guide
- Troubleshooting guide
- FAQ

---

### 10.2 Deployment

#### 10.2.1 Build Packaging
**Priority**: MEDIUM
**Effort**: 2-3 days

**Requirements**:
- Linux package (.deb, .rpm)
- Windows installer (.msi)
- ARM cross-compilation
- Static linking options

#### 10.2.2 Installation Scripts
**Priority**: MEDIUM
**Effort**: 2-3 days

**Requirements**:
- Systemd service (Linux)
- Windows service
- Auto-start configuration
- Log rotation setup

#### 10.2.3 Docker Container
**Priority**: LOW
**Effort**: 1-2 days

**Requirements**:
- Dockerfile
- Docker Compose
- Volume mounts for config
- Serial device passthrough

---

## Summary Timeline

### Critical Path (Minimum Viable Product)

| Phase | Description | Effort | Dependencies |
|-------|-------------|--------|--------------|
| 2.1 | SAS Constants & Utilities | 6-9 days | None |
| 3.1 | Serial Port (Linux) | 4-5 days | Phase 2.1 |
| 4.1 | SASCommPort | 5-7 days | Phase 3.1 |
| 5.1 | Core Commands | 17-21 days | Phase 4.1 |
| 5.2 | Progressive Commands | 4-5 days | Phase 5.1 |
| 6.1 | SASDaemon | 7-10 days | Phase 5 |
| 9.1 | Unit Tests | 12-17 days | Ongoing |
| 9.2 | Integration Tests | 10-14 days | Phase 6 |

**Estimated MVP Time**: 3-4 months (full-time development)

### Full Feature Set

| Phase | Description | Effort | Priority |
|-------|-------------|--------|----------|
| All above | MVP | 3-4 months | HIGH |
| 5.3 | AFT Commands | 10-14 days | HIGH |
| 5.4 | TITO Commands | 10-13 days | HIGH |
| 7 | OneLink Integration | 15-20 days | MEDIUM |
| 8.1 | Persistence | 6-8 days | MEDIUM |
| 8.2 | Logging | 4-6 days | MEDIUM |
| 8.3 | Web Console | 12-17 days | LOW |
| 10 | Documentation & Deployment | 10-15 days | MEDIUM |

**Estimated Full Feature Time**: 6-8 months (full-time development)

---

## Dependencies and Prerequisites

### External Libraries

#### Required
- **C++17 compiler** (GCC 7+, Clang 5+, MSVC 2017+)
- **CMake** 3.15+
- **pthread** (POSIX threads)

#### Recommended
- **spdlog** - Logging framework
- **Google Test** - Unit testing
- **TinyXML2** or **RapidXML** - XML parsing
- **cpp-httplib** or **Boost.Beast** - HTTP server (optional)

#### Platform-Specific
- **Linux**: `libpthread`, `libc`, termios headers
- **Windows**: Win32 API, WinSock2
- **ARM**: Cross-compilation toolchain

### Hardware Requirements

#### Development
- Standard PC (Linux or Windows)
- USB-to-serial adapter (for testing)
- SAS protocol analyzer (optional)

#### Production
- PAL-264 hardware or compatible
- ARM processor (embedded Linux)
- Serial ports (RS-232)
- Network connectivity

---

## Risk Assessment

### High Risk Items

1. **Serial Port Timing**
   - **Risk**: SAS protocol has strict timing requirements
   - **Mitigation**: Use hardware timestamps, measure actual timings, optimize critical paths

2. **CRC Implementation**
   - **Risk**: Incorrect CRC calculation breaks protocol
   - **Mitigation**: Extensive testing, use reference implementations, validate against known values

3. **Thread Safety**
   - **Risk**: Race conditions in concurrent code
   - **Mitigation**: Use mutexes, atomic operations, thread sanitizer, code review

4. **OneLink Compatibility**
   - **Risk**: Version incompatibilities with server
   - **Mitigation**: Follow schema specifications exactly, test with real servers

### Medium Risk Items

1. **Memory Leaks**
   - **Risk**: Long-running process memory growth
   - **Mitigation**: Use smart pointers, run valgrind, stress testing

2. **Platform Portability**
   - **Risk**: Code doesn't work on target platform
   - **Mitigation**: Cross-compile early, test on target hardware

3. **Performance**
   - **Risk**: Can't meet polling frequency requirements
   - **Mitigation**: Profile code, optimize hot paths, use async I/O

---

## Success Criteria

### MVP Success
- ✅ Serial communication working (send/receive)
- ✅ CRC validation passing
- ✅ Core SAS commands implemented (meters, enable/disable)
- ✅ General poll working
- ✅ Exception queue functional
- ✅ Progressive updates working
- ✅ Integration test passing with SAS tester

### Production Ready
- ✅ All critical SAS commands implemented
- ✅ AFT/TITO support complete
- ✅ OneLink integration functional
- ✅ Persistence working
- ✅ Logging comprehensive
- ✅ Unit test coverage >80%
- ✅ Integration tests passing
- ✅ 24-hour stress test passing
- ✅ Documentation complete
- ✅ Deployment packages available

---

## Getting Started

### Recommended Development Order

1. **Week 1-2**: CRC-16, BCD, SAS Constants
2. **Week 3-4**: Linux Serial Port
3. **Week 5-6**: SASCommPort base implementation
4. **Week 7-10**: Core command handlers (meters, config, enable/disable)
5. **Week 11-12**: Exception queue and event polling
6. **Week 13-14**: SASDaemon polling loop
7. **Week 15-16**: Unit and integration testing
8. **Week 17+**: Advanced features (AFT, TITO, etc.)

### Quick Start Commands

```bash
# Clone and build
cd C:\_code\egm-emulator\egm-emulator-cpp
mkdir build && cd build
cmake ..
cmake --build .

# Run tests (when implemented)
ctest

# Run simulator
./egm_simulator
```

---

## Resources

### SAS Protocol
- IGT SAS Protocol Specification
- Gaming Standards Association documentation
- SAS Version 6.02+ specifications

### Development Tools
- **Compiler**: GCC 7+, Clang 5+, MSVC 2017+
- **Debugger**: GDB, LLDB, Visual Studio Debugger
- **Profiler**: perf, valgrind, Visual Studio Profiler
- **Static Analysis**: clang-tidy, cppcheck
- **Protocol Analyzer**: Logic analyzer with serial decode

### Testing Resources
- SAS protocol test suite
- Hardware SAS emulator (if available)
- OneLink test server

---

## GUI Implementation Plan

### Overview
The GUI will provide a testing and demonstration interface for the EGM simulator, allowing users to:
- Simulate player actions (betting, playing, cashing out)
- Insert bills to add credits
- Monitor machine state in real-time
- Test SAS communication
- View meters and statistics

### Requirements

#### Functional Requirements
1. **Player Simulation**
   - Bet button (increment bet amount)
   - Play/Spin button (deduct bet, play game, award win)
   - Cashout button (print ticket, deduct credits)
   - Max bet button (bet maximum allowed)

2. **Bill Acceptor Simulation**
   - Six buttons for bill denominations: $1, $5, $10, $20, $50, $100
   - Visual feedback when bill is accepted
   - Update credits immediately
   - Increment appropriate meters

3. **Display Elements**
   - **Credits Display**: Large, prominent display of current credits
   - **Meter Panel**: Show key meters (coin in, coin out, games played, won, lost, jackpot, drop)
   - **Progressive Display**: Show 4 progressive levels with current amounts
   - **Status Indicators**: Machine enabled, door status, bill acceptor status
   - **Last Ticket**: Show last printed ticket validation number and amount

4. **SAS Testing Panel**
   - Send manual polls (general poll, meter queries)
   - View last poll response
   - Connection status indicator
   - Poll statistics (total polls, timeouts, errors)

5. **Machine Control**
   - Enable/Disable machine button
   - Enable/Disable bill acceptor button
   - Door open/close simulation
   - RAM clear button

#### Non-Functional Requirements
- **Performance**: UI updates at least 10 times per second
- **Responsiveness**: Button presses respond within 100ms
- **Compatibility**: Run on Zeus OS (embedded Linux ARM) and desktop via remote desktop
- **Simplicity**: Clean, intuitive interface requiring minimal training

### Technology Options

#### Option 1: Web-Based GUI (RECOMMENDED)
**Technology**: HTML/CSS/JavaScript + Embedded HTTP Server (cpp-httplib or Mongoose)

**Pros**:
- ✅ Cross-platform (works anywhere with a web browser)
- ✅ Perfect for remote desktop access (just open browser)
- ✅ Easy to style and iterate on design
- ✅ No additional GUI library dependencies
- ✅ Real-time updates via WebSocket or Server-Sent Events
- ✅ Can be accessed from multiple devices simultaneously

**Cons**:
- ❌ Requires embedding HTTP server in C++ application
- ❌ Need to implement WebSocket or SSE for real-time updates

**Implementation**:
```cpp
// Use cpp-httplib for HTTP server
#include "httplib.h"

class WebGUI {
    httplib::Server server;
    Machine* machine;

    void setupRoutes() {
        server.Get("/", [](const Request&, Response& res) {
            res.set_content(getIndexHTML(), "text/html");
        });

        server.Get("/api/status", [this](const Request&, Response& res) {
            json status = {
                {"credits", machine->getCredits()},
                {"meters", getMeterData()},
                {"progressives", getProgressiveData()}
            };
            res.set_content(status.dump(), "application/json");
        });

        server.Post("/api/action/bet", [this](const Request&, Response& res) {
            machine->incrementBet();
            res.set_content("{\"status\":\"ok\"}", "application/json");
        });
    }
};
```

#### Option 2: Qt5 GUI
**Technology**: Qt5 framework with C++

**Pros**:
- ✅ Native GUI with excellent performance
- ✅ Cross-platform (Linux, Windows, embedded)
- ✅ Rich widget library
- ✅ Signal/slot mechanism for events
- ✅ Proven on embedded devices

**Cons**:
- ❌ Large dependency (~100MB+ for Qt5 runtime)
- ❌ May not be available on Zeus OS without additional installation
- ❌ More complex setup than web-based
- ❌ Remote desktop requires full X11/Wayland forwarding

#### Option 3: GTK+ GUI
**Technology**: GTK+ 3 with C bindings

**Pros**:
- ✅ Lightweight compared to Qt
- ✅ Often pre-installed on Linux systems
- ✅ Good performance

**Cons**:
- ❌ Linux-only (not truly cross-platform)
- ❌ More verbose C API
- ❌ Remote desktop requires X11 forwarding

### Recommended Approach: Web-Based GUI

**Architecture**:
```
┌─────────────────────────────────────────────┐
│         Browser (Chrome/Firefox)            │
│  ┌─────────────────────────────────────┐   │
│  │    HTML/CSS/JavaScript Frontend      │   │
│  │  - Display credits, meters, status   │   │
│  │  - Button handlers (bet, play, etc.) │   │
│  │  - Real-time updates via SSE         │   │
│  └─────────────────────────────────────┘   │
└──────────────────┬──────────────────────────┘
                   │ HTTP/SSE
                   │
┌──────────────────▼──────────────────────────┐
│       C++ EGM Simulator Process             │
│  ┌─────────────────────────────────────┐   │
│  │   HTTP Server (cpp-httplib)         │   │
│  │  - Serve HTML/CSS/JS files          │   │
│  │  - REST API endpoints               │   │
│  │  - Server-Sent Events stream        │   │
│  └─────────────────────────────────────┘   │
│  ┌─────────────────────────────────────┐   │
│  │   Machine + SASDaemon + SASCommPort │   │
│  └─────────────────────────────────────┘   │
└─────────────────────────────────────────────┘
```

**API Endpoints**:
- `GET /` - Serve main HTML page
- `GET /api/status` - Get machine status (JSON)
- `GET /api/meters` - Get all meters (JSON)
- `GET /api/progressives` - Get progressive levels (JSON)
- `GET /api/events` - Server-Sent Events stream for real-time updates
- `POST /api/action/bet` - Increment bet
- `POST /api/action/play` - Play game
- `POST /api/action/cashout` - Cashout
- `POST /api/bill/{denomination}` - Insert bill ($1, $5, $10, $20, $50, $100)
- `POST /api/control/enable` - Enable/disable machine
- `POST /api/poll/{command}` - Send manual SAS poll

**Server-Sent Events Stream** (for real-time updates):
```
event: credits
data: {"credits": 12500}

event: meter
data: {"meter": "coin_in", "value": 500000}

event: progressive
data: {"level": 1, "amount": 1250}
```

### Implementation Steps

1. **Add HTTP Server Library**
   - Download cpp-httplib (header-only library)
   - Add to project includes

2. **Create WebGUI Class**
   - Embed HTML/CSS/JS as strings in C++
   - Set up HTTP server routes
   - Implement REST API endpoints
   - Add SSE stream for real-time updates

3. **Create Frontend**
   - HTML page with button layout
   - CSS for styling (clean, modern look)
   - JavaScript for button handlers and real-time updates
   - Display panels for credits, meters, progressives

4. **Integrate with Simulator**
   - Connect WebGUI to Machine instance
   - Wire up button actions to machine methods
   - Stream updates on every state change

5. **Testing**
   - Test on local development machine
   - Test on Zeus OS device
   - Test via remote desktop connection
   - Verify performance and responsiveness

### Next Phase After GUI
After GUI is complete, remaining optional enhancements:
- **OneLink Integration** (Phase 7 original plan)
- **Persistence** (save/load state)
- **Advanced Logging**
- **Configuration Files**

---

**Document Version**: 1.1
**Last Updated**: 2025-11-26
**Status**: Active Development Plan
