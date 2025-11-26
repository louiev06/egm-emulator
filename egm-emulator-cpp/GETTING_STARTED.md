# Getting Started with EGM Emulator C++

## Quick Start

This C++ project is a port of the Paltronics MegaMIC EGM (Electronic Gaming Machine) emulator, converted from the Java implementation at `C:\_code\gs-olk-product-megamic-main`.

## What You Have

A complete, working C++ implementation of the core EGM simulator with:

✅ **Machine simulation** - Full game cabinet with multi-game support
✅ **Meter tracking** - 20+ different meters (coin in/out, games played, etc.)
✅ **Progressive jackpots** - Multiple progressive levels with link monitoring
✅ **Credit management** - Cashable, restricted, and non-restricted credits
✅ **Event system** - Type-safe publish/subscribe event bus
✅ **State management** - Door, lights, hopper, handpay, AFT locking
✅ **Demo application** - Working example in [src/simulator/main.cpp](src/simulator/main.cpp)

## File Overview

### Headers (include/megamic/)
```
event/
  EventService.h          - Publish/subscribe event bus

io/
  CommChannel.h           - Serial port abstraction

sas/
  SASConstants.h          - SAS protocol constants & denominations

simulator/
  Game.h                  - Individual game configuration
  Machine.h               - Main machine simulator (400+ lines)
  MachineEvents.h         - All event type definitions

ICardPlatform.h           - Hardware abstraction interface
```

### Implementation (src/)
```
event/
  EventService.cpp        - Event system implementation

io/
  CommChannel.cpp         - Piped channel for simulation
  SimulatedPlatform.cpp   - Platform stub for testing

sas/
  SASConstants.cpp        - Denomination mapping tables

simulator/
  Game.cpp                - Game class implementation
  Machine.cpp             - Machine class (1000+ lines)
  main.cpp                - Demo application
```

### Build Files
```
CMakeLists.txt            - CMake build configuration
build.sh                  - Simple shell build script
.gitignore                - Git ignore patterns
```

### Documentation
```
README.md                 - Full project documentation
CONVERSION_SUMMARY.md     - Detailed conversion notes
GETTING_STARTED.md        - This file
```

## Building

### Option 1: CMake (Recommended)

```bash
cd C:\_code\egm-emulator\egm-emulator-cpp
mkdir build && cd build
cmake ..
cmake --build .
```

Run the demo:
```bash
./egm_simulator
```

### Option 2: Shell Script

```bash
cd C:\_code\egm-emulator\egm-emulator-cpp
chmod +x build.sh
./build.sh
```

Run the demo:
```bash
./build_manual/egm_simulator
```

### Option 3: Manual Compilation

```bash
g++ -std=c++17 -pthread -I./include \
    src/event/EventService.cpp \
    src/io/CommChannel.cpp \
    src/io/SimulatedPlatform.cpp \
    src/sas/SASConstants.cpp \
    src/simulator/Game.cpp \
    src/simulator/Machine.cpp \
    src/simulator/main.cpp \
    -o egm_simulator

./egm_simulator
```

## Understanding the Code

### 1. Machine Class

The `Machine` class ([Machine.h](include/megamic/simulator/Machine.h)) is the heart of the simulator:

```cpp
#include "megamic/simulator/Machine.h"
#include "megamic/event/EventService.h"
#include "megamic/ICardPlatform.h"

// Create dependencies
auto eventService = std::make_shared<EventService>();
auto platform = std::make_shared<SimulatedPlatform>();

// Create machine
auto machine = std::make_shared<Machine>(eventService, platform);

// Configure accounting denomination (1 cent)
machine->setAccountingDenomCode(1);

// Add games
auto game1 = machine->addGame(1, 0.01, 5, "Game Name", "Paytable");
machine->setCurrentGame(game1);

// Add credits
machine->addCredits(100.0);  // $100

// Play
machine->start();
machine->bet(3);  // Bet 3 credits
```

### 2. Event System

Subscribe to events to monitor machine activity:

```cpp
// Subscribe to game events
eventService->subscribe<GamePlayedEvent>([](const GamePlayedEvent& e) {
    std::cout << "Game played: " << e.game->getGameName()
              << ", wager: $" << e.wager << std::endl;
});

// Subscribe to progressive hits
eventService->subscribe<ProgressiveHitEvent>([](const ProgressiveHitEvent& e) {
    std::cout << "Progressive hit! Level " << e.levelId
              << " for $" << e.win << std::endl;
});
```

### 3. Progressive Jackpots

```cpp
// Add progressive levels
machine->addProgressive(1);  // Level 1
machine->addProgressive(2);  // Level 2

// Set values (would normally come from server)
machine->setProgressiveValue(1, 1250.50);
machine->setProgressiveValue(2, 5000.75);

// Trigger a hit
machine->progressiveHit(1);  // Player hit level 1!
```

### 4. Meters

Access any of 20+ meters:

```cpp
using namespace megamic::sas;

// Game metrics
int64_t gamesPlayed = machine->getGamesPlayed();
int64_t gamesWon = machine->getMeter(SASConstants::MTR_GAMES_WON);
int64_t gamesLost = machine->getMeter(SASConstants::MTR_GAMES_LOST);

// Financial metrics
double coinIn = machine->getCoinInMeter();
int64_t coinOut = machine->getCoinOutMeter();
int64_t jackpot = machine->getJackpotMeter();

// Current state
int64_t currentCredits = machine->getCredits();
double cashableAmount = machine->getCashableAmount();
```

### 5. State Management

```cpp
// Door events
machine->setDoorOpen(true);
if (!machine->isPlayable()) {
    std::cout << "Cannot play - door is open!" << std::endl;
}
machine->setDoorOpen(false);

// Service light
machine->setLightOn(true);

// Hopper
machine->setHopper(true);  // Low

// AFT locking
machine->setAftLocked(true);
// Machine is now locked and unplayable
```

## Key Concepts

### Denominations

Games have a denomination (bet per credit):

```cpp
// Penny game
auto pennyGame = machine->addGame(1, 0.01, 5, "Penny Slots", "PS-001");

// Quarter game
auto quarterGame = machine->addGame(2, 0.25, 3, "Quarter Slots", "QS-001");

// Dollar game
auto dollarGame = machine->addGame(3, 1.00, 10, "Dollar Slots", "DS-001");
```

### Accounting Denomination

The machine tracks credits in a specific accounting denomination:

```cpp
// Set to penny (all credits tracked in pennies)
machine->setAccountingDenomCode(1);  // Code 1 = $0.01

// Add $100
machine->addCredits(100.0);

// Credits stored as: 10000 (pennies)
int64_t credits = machine->getCredits();  // 10000

// Convert back to dollars
double dollars = machine->getCashableAmount();  // $100.00
```

### Credit Types

Three types of credits:

```cpp
// Cashable - can be cashed out
machine->addCredits(50.0);

// Restricted - promotional, limited use
machine->addRestrictedCredits(25.0);

// Non-restricted - promotional, cannot cash out
machine->addNonRestrictedCredits(10.0);
```

### Handpays

Large wins trigger handpays:

```cpp
// Set handpay limit
machine->setHandpayLimit(400.00);

// Award bonus
machine->awardBonus(45000, false);  // $450 in accounting credits

// Check status
if (machine->isHandpayPending()) {
    std::cout << "Attendant payout required!" << std::endl;

    // Attendant resets after paying
    machine->handpayReset();
}
```

## Example Application Flow

```cpp
int main() {
    // 1. Create services
    auto eventService = std::make_shared<EventService>();
    auto platform = std::make_shared<SimulatedPlatform>();
    auto machine = std::make_shared<Machine>(eventService, platform);

    // 2. Configure machine
    machine->setAccountingDenomCode(1);  // Penny accounting
    machine->setHandpayLimit(1200.00);   // $1200 handpay limit

    // 3. Add games
    auto game1 = machine->addGame(1, 0.01, 100, "Buffalo", "BUF-001");
    auto game2 = machine->addGame(2, 0.25, 20, "Wheel of Fortune", "WOF-001");
    machine->setCurrentGame(game1);

    // 4. Setup progressives
    machine->addProgressive(1);
    machine->setProgressiveValue(1, 5250.75);

    // 5. Subscribe to events
    eventService->subscribe<GamePlayedEvent>([](const auto& e) {
        std::cout << "Played: " << e.game->getGameName() << std::endl;
    });

    // 6. Add credits and start
    machine->addCredits(100.0);
    machine->start();

    // 7. Play games
    for (int i = 0; i < 10; i++) {
        machine->bet(5);

        // Simulate random win
        if (rand() % 3 == 0) {
            machine->GameWon();
            machine->addJackpot(2.50);
        } else {
            machine->GameLost();
        }
    }

    // 8. Check results
    std::cout << "Games played: " << machine->getGamesPlayed() << std::endl;
    std::cout << "Coin in: $" << machine->getCoinInMeter() << std::endl;
    std::cout << "Credits: " << machine->getCredits() << std::endl;

    // 9. Cleanup
    machine->stop();

    return 0;
}
```

## Next Steps

### For Learning
1. Read [README.md](README.md) for full architecture overview
2. Review [CONVERSION_SUMMARY.md](CONVERSION_SUMMARY.md) for Java→C++ details
3. Study [src/simulator/main.cpp](src/simulator/main.cpp) demo application
4. Examine [Machine.h](include/megamic/simulator/Machine.h) API

### For Development
1. Implement SAS protocol handler (`SASCommPort`)
2. Add serial port support (termios on Linux, Win32 API on Windows)
3. Integrate logging framework (spdlog recommended)
4. Add unit tests (Google Test framework)
5. Implement OneLink XML communication

### For Testing
1. Run the demo application
2. Create custom test scenarios
3. Test progressive link failures
4. Validate meter rollover handling
5. Test multi-threading safety

## Common Tasks

### Add a new game
```cpp
auto game = machine->addGame(
    gameNumber,    // Unique game ID
    denomination,  // e.g., 0.25 for quarter
    maxBet,        // Maximum credits per spin
    "Game Name",
    "Paytable ID"
);
```

### Monitor events
```cpp
eventService->subscribe<EventType>([](const EventType& event) {
    // Handle event
});
```

### Check machine state
```cpp
bool canPlay = machine->isPlayable();
bool doorOpen = machine->isDoorOpen();
bool linkUp = machine->isProgressiveLinkUp();
bool handpay = machine->isHandpayPending();
```

### Access meters
```cpp
// Specific meter
int64_t coinIn = machine->getMeter(SASConstants::MTR_COIN_IN);

// Convenience methods
int64_t gamesPlayed = machine->getGamesPlayed();
double coinInDollars = machine->getCoinInMeter();
```

## Troubleshooting

### Build Errors

**Missing pthread**:
```bash
# Linux
sudo apt-get install libpthread-stubs0-dev

# Already included in MSYS2/MinGW
```

**C++17 not supported**:
```bash
# Update compiler
g++ --version  # Need 7.0+
clang++ --version  # Need 5.0+
```

### Runtime Issues

**Segmentation fault**:
- Ensure all shared_ptr objects are initialized
- Check that currentGame is set before calling play methods

**Progressive link down**:
- Progressive values must be updated within 5 seconds
- Call `setProgressiveValue()` periodically to maintain link

**Not playable**:
```cpp
// Check all conditions
if (machine->isDoorOpen()) std::cout << "Door open" << std::endl;
if (!machine->isProgressiveLinkUp()) std::cout << "Link down" << std::endl;
if (machine->isHandpayPending()) std::cout << "Handpay pending" << std::endl;
if (machine->isAftLocked()) std::cout << "AFT locked" << std::endl;
```

## Resources

### Documentation
- [README.md](README.md) - Full project documentation
- [CONVERSION_SUMMARY.md](CONVERSION_SUMMARY.md) - Conversion details
- [Machine.h](include/megamic/simulator/Machine.h) - API reference

### Source Code
- Java original: `C:\_code\gs-olk-product-megamic-main`
- C++ port: `C:\_code\egm-emulator\egm-emulator-cpp`

### External References
- SAS Protocol: Industry standard for slot machines
- OneLink: Casino management system
- PAL-264: Paltronics hardware platform

## License

Original Java implementation: Copyright © 2009 Paltronics, Inc.
This C++ port maintains the same architecture and functionality.

---

**Ready to start?** Run the demo application:
```bash
./build.sh && ./build_manual/egm_simulator
```

For questions or issues, review the documentation in this directory.
