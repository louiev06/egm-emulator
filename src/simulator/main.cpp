#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>
#include "simulator/Machine.h"
#include "simulator/Game.h"
#include "simulator/MachineEvents.h"
#include "event/EventService.h"
#include "sas/SASCommPort.h"
#include "sas/SASConstants.h"
#include "http/HTTPServer.h"
#include "config/EGMConfig.h"
#include "config/MeterPersistence.h"
#include "version.h"

#ifdef ZEUS_OS
#include "io/SASSerialPort.h"
#include "io/ZeusPlatform.h"
extern "C" {
#include <s7lite.h>  // For watchdog functions
}
#else
#include "io/SimulatedPlatform.h"
#endif


using namespace simulator;
using namespace event;
using namespace sas;
using namespace io;

// Global flag for graceful shutdown
std::atomic<bool> g_running(true);

void signalHandler(int signum) {
    std::cout << "\nInterrupt signal (" << signum << ") received. Shutting down..." << std::endl;
    g_running = false;
}

int main() {
    std::cout << "EGM Emulator - SAS Slave Device" << std::endl;
    std::cout << "Version " << VERSION_STRING << "." << BUILD_NUMBER << std::endl;
    std::cout << "===============================" << std::endl;

    // Load configuration from egm-config.json
    std::cout << "Loading configuration..." << std::endl;
    if (!config::EGMConfig::load("egm-config.json")) {
        std::cout << "Warning: Could not load egm-config.json, using defaults" << std::endl;
    }

    // Install signal handler for graceful shutdown
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

#ifdef ZEUS_OS
    // Initialize watchdog (30 second timeout)
    // We'll kick it every 10 seconds in the main loop
    std::cout << "Initializing watchdog..." << std::endl;
    if (S7LITE_Watchdog_Enable() == S7DLL_STATUS_OK) {
        S7LITE_Watchdog_SetTimeout(30);  // 30 second timeout
        S7LITE_Watchdog_Kick();  // Initial kick
        std::cout << "Watchdog enabled with 30 second timeout" << std::endl;
    } else {
        std::cout << "Warning: Watchdog initialization failed (may not be supported)" << std::endl;
    }
#endif

    try {
        // Create event service
        auto eventService = std::make_shared<EventService>();

#ifdef ZEUS_OS
        // Zeus OS platform - real hardware
        auto platform = std::make_shared<ZeusPlatform>();
        std::cout << "Platform: Zeus OS" << std::endl;

        // Create SAS serial port using SASSerialPort
        std::cout << "Opening SAS serial port (UART 1)..." << std::endl;
        auto channel = std::make_shared<SASSerialPort>();
        std::cout << "SAS serial port created" << std::endl;
#else
        // Simulated platform for development
        auto platform = std::make_shared<SimulatedPlatform>();
        std::cout << "Platform: Simulated" << std::endl;

        // Create simulated serial channel
        auto channel = std::make_shared<CommChannel>();
#endif

        // Create machine
        auto machine = std::make_shared<Machine>(eventService, platform);

        // Load meters from persistent storage
        std::cout << "Loading persistent meters..." << std::endl;
        config::MeterPersistence::loadMeters(machine.get());

        // Set up accounting denom (1 cent)
        machine->setAccountingDenomCode(1);

        // Add games from configuration
        std::cout << "\nAdding games from configuration..." << std::endl;
        std::shared_ptr<Game> firstGame = nullptr;

        auto doc = config::EGMConfig::getDocument();
        if (doc && doc->HasMember("games") && (*doc)["games"].IsArray()) {
            const auto& gamesArray = (*doc)["games"];

            for (rapidjson::SizeType i = 0; i < gamesArray.Size(); i++) {
                const auto& gameConfig = gamesArray[i];

                // Check if game is enabled (default to true if not specified)
                bool enabled = !gameConfig.HasMember("enabled") || gameConfig["enabled"].GetBool();
                if (!enabled) {
                    continue;
                }

                // Read game configuration
                int gameNumber = gameConfig["gameNumber"].GetInt();
                double denom = gameConfig["denomination"].GetDouble();
                int maxBet = gameConfig["maxBet"].GetInt();
                std::string gameName = gameConfig.HasMember("gameName") ?
                    gameConfig["gameName"].GetString() : "Slot Game";
                std::string gameID = gameConfig["gameID"].GetString();

                // Add the game
                auto game = machine->addGame(gameNumber, denom, maxBet, gameName, gameID);
                std::cout << "  Game " << gameNumber << ": " << game->getGameName()
                          << " ($" << game->getDenom() << " denom)" << std::endl;

                // Remember first game for default selection
                if (!firstGame) {
                    firstGame = game;
                }
            }
        }

        // Select first game as current game
        if (firstGame) {
            machine->setCurrentGame(firstGame);
            std::cout << "\nCurrent game: " << machine->getCurrentGame()->getGameName() << std::endl;
        }

        // Add progressive levels
        std::cout << "\nAdding progressive levels..." << std::endl;
        machine->addProgressive(1);
        machine->addProgressive(2);
        machine->addProgressive(3);
        machine->addProgressive(4);
        machine->setProgressiveValue(1, 100.00);   // Mini
        machine->setProgressiveValue(2, 500.00);   // Minor
        machine->setProgressiveValue(3, 2500.00);  // Major
        machine->setProgressiveValue(4, 10000.00); // Grand
        std::cout << "  Level 1 (Mini):  $" << machine->getProgressive(1) << std::endl;
        std::cout << "  Level 2 (Minor): $" << machine->getProgressive(2) << std::endl;
        std::cout << "  Level 3 (Major): $" << machine->getProgressive(3) << std::endl;
        std::cout << "  Level 4 (Grand): $" << machine->getProgressive(4) << std::endl;

        // Add initial credits for testing
        std::cout << "\nAdding $100 in credits..." << std::flush;
        machine->addCredits(100.0);
        std::cout << " Done!" << std::endl;
        std::cout << "Current credits: " << machine->getCredits()
                  << " ($" << machine->getCashableAmount() << ")" << std::flush;
        std::cout << std::endl;

        // Create SAS communication port (SLAVE - responds to polls)
        std::cout << "\nInitializing SAS communication (Slave Mode)..." << std::flush;
        auto sasPort = std::make_shared<SASCommPort>(machine.get(), channel, 1);
        std::cout << " Created!" << std::endl;

        // Start SAS port (will listen for polls from master)
        std::cout << "Starting SAS port..." << std::flush;
        if (!sasPort->start()) {
            std::cerr << "Failed to start SAS communication port!" << std::endl;
            return 1;
        }
        std::cout << " Started!" << std::endl;
        std::cout << "SAS Port started - Address: " << (int)sasPort->getAddress() << std::endl;
        std::cout << "Listening for SAS polls from master device..." << std::endl;

        // Start machine
        std::cout << "Starting machine..." << std::flush;
        machine->start();
        std::cout << " Started!" << std::endl;
        std::cout << "\nMachine started and ready!" << std::endl;
        std::cout << "Machine playable: " << (machine->isPlayable() ? "Yes" : "No") << std::endl;

        // Start HTTP server for web GUI
        std::cout << "\nStarting HTTP server for GUI..." << std::flush;
        HTTPServer httpServer(machine.get(), 8080);
        httpServer.start();
        std::cout << " Started!" << std::endl;
        std::cout << "HTTP Server listening on port 8080" << std::endl;
        std::cout << "GUI URL: http://localhost:8080/index.html" << std::endl;

        // Main loop - display statistics periodically
        std::cout << "\n===============================" << std::endl;
        std::cout << "EGM Emulator running..." << std::endl;
        std::cout << "Press Ctrl+C to stop" << std::endl;
        std::cout << "===============================" << std::endl;

        auto lastStatsTime = std::chrono::steady_clock::now();

        // Track previous values to only print on change
        struct {
            uint64_t messagesReceived = 0;
            uint64_t messagesSent = 0;
            uint64_t generalPolls = 0;
            uint64_t longPolls = 0;
            uint64_t crcErrors = 0;
            uint64_t framingErrors = 0;
            double credits = 0;
            uint64_t gamesPlayed = 0;
            int64_t gamesWon = 0;
            double coinIn = 0;
            double coinOut = 0;
        } lastStats;

        while (g_running) {
            std::this_thread::sleep_for(std::chrono::seconds(1));

            // Display statistics every 10 seconds
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastStatsTime);

            if (elapsed.count() >= 10) {
#ifdef ZEUS_OS
                // Kick watchdog every 10 seconds (timeout is 30 seconds)
                S7LITE_Watchdog_Kick();
#endif

                auto stats = sasPort->getStatistics();
                double credits = machine->getCashableAmount();
                uint64_t gamesPlayed = machine->getGamesPlayed();
                int64_t gamesWon = machine->getMeter(SASConstants::METER_GAMES_WON);
                double coinIn = machine->getCoinInMeter();
                double coinOut = machine->getCoinOutMeter();

                // Only print if any value has changed
                bool changed = (stats.messagesReceived != lastStats.messagesReceived ||
                               stats.messagesSent != lastStats.messagesSent ||
                               stats.generalPolls != lastStats.generalPolls ||
                               stats.longPolls != lastStats.longPolls ||
                               stats.crcErrors != lastStats.crcErrors ||
                               stats.framingErrors != lastStats.framingErrors ||
                               credits != lastStats.credits ||
                               gamesPlayed != lastStats.gamesPlayed ||
                               gamesWon != lastStats.gamesWon ||
                               coinIn != lastStats.coinIn ||
                               coinOut != lastStats.coinOut);

                if (changed) {
                    std::cout << "\n--- SAS Statistics ---" << std::endl;
                    std::cout << "Messages Received: " << stats.messagesReceived << std::endl;
                    std::cout << "Messages Sent:     " << stats.messagesSent << std::endl;
                    std::cout << "General Polls:     " << stats.generalPolls << std::endl;
                    std::cout << "Long Polls:        " << stats.longPolls << std::endl;
                    std::cout << "CRC Errors:        " << stats.crcErrors << std::endl;
                    std::cout << "Framing Errors:    " << stats.framingErrors << std::endl;

                    std::cout << "\n--- Machine Status ---" << std::endl;
                    std::cout << "Credits:           $" << credits << std::endl;
                    std::cout << "Games Played:      " << gamesPlayed << std::endl;
                    std::cout << "Games Won:         " << gamesWon << std::endl;
                    std::cout << "Coin In:           $" << coinIn << std::endl;
                    std::cout << "Coin Out:          $" << coinOut << std::endl;
                    std::cout << "---------------------" << std::endl;

                    // Update last values
                    lastStats.messagesReceived = stats.messagesReceived;
                    lastStats.messagesSent = stats.messagesSent;
                    lastStats.generalPolls = stats.generalPolls;
                    lastStats.longPolls = stats.longPolls;
                    lastStats.crcErrors = stats.crcErrors;
                    lastStats.framingErrors = stats.framingErrors;
                    lastStats.credits = credits;
                    lastStats.gamesPlayed = gamesPlayed;
                    lastStats.gamesWon = gamesWon;
                    lastStats.coinIn = coinIn;
                    lastStats.coinOut = coinOut;
                }

                lastStatsTime = now;
            }
        }

        // Clean shutdown
        std::cout << "\nShutting down..." << std::endl;

        // Save meters to persistent storage before shutdown
        std::cout << "Saving persistent meters..." << std::endl;
        config::MeterPersistence::saveMeters(machine.get());

        httpServer.stop();
        sasPort->stop();
        machine->stop();
        std::cout << "HTTP Server stopped" << std::endl;
        std::cout << "SAS Port stopped" << std::endl;
        std::cout << "Machine stopped" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "\n===============================" << std::endl;
    std::cout << "EGM Emulator shutdown complete" << std::endl;
    std::cout << "===============================" << std::endl;

    return 0;
}
