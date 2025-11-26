#include <iostream>
#include <memory>
#include "megamic/simulator/Machine.h"
#include "megamic/simulator/Game.h"
#include "megamic/simulator/MachineEvents.h"
#include "megamic/event/EventService.h"
#include "megamic/ICardPlatform.h"

using namespace megamic;
using namespace megamic::simulator;
using namespace megamic::event;

int main() {
    std::cout << "EGM Emulator - C++ Version 15.4.0" << std::endl;
    std::cout << "===============================" << std::endl;

    try {
        // Create event service
        auto eventService = std::make_shared<EventService>();

        // Create platform (simulated)
        auto platform = std::make_shared<SimulatedPlatform>();

        std::cout << "Platform: " << platform->getPlatformInfo() << std::endl;

        // Create machine
        auto machine = std::make_shared<Machine>(eventService, platform);

        // Set up accounting denom (1 cent)
        machine->setAccountingDenomCode(1);

        // Add some games
        std::cout << "\nAdding games..." << std::endl;
        auto game1 = machine->addGame(1, 0.01, 5, "Double Diamond", "DD-001");
        std::cout << "  Game 1: " << game1->getGameName()
                  << " ($" << game1->getDenom() << " denom)" << std::endl;

        auto game2 = machine->addGame(2, 0.25, 3, "Triple Stars", "TS-001");
        std::cout << "  Game 2: " << game2->getGameName()
                  << " ($" << game2->getDenom() << " denom)" << std::endl;

        auto game3 = machine->addGame(3, 1.00, 10, "Bonus Wheel", "BW-001");
        std::cout << "  Game 3: " << game3->getGameName()
                  << " ($" << game3->getDenom() << " denom)" << std::endl;

        // Select current game
        machine->setCurrentGame(game1);
        std::cout << "\nCurrent game: " << machine->getCurrentGame()->getGameName() << std::endl;

        // Subscribe to game events
        eventService->subscribe<GamePlayedEvent>([](const GamePlayedEvent& event) {
            std::cout << "  EVENT: Game played - " << event.game->getGameName()
                      << ", wager $" << event.wager << std::endl;
        });

        eventService->subscribe<GameChangedEvent>([machine](const GameChangedEvent&) {
            std::cout << "  EVENT: Game changed to "
                      << machine->getCurrentGame()->getGameName() << std::endl;
        });

        // Add some credits
        std::cout << "\nAdding $100 in credits..." << std::endl;
        machine->addCredits(100.0);
        std::cout << "Current credits: " << machine->getCredits()
                  << " ($" << machine->getCashableAmount() << ")" << std::endl;

        // Add progressive levels
        std::cout << "\nAdding progressive levels..." << std::endl;
        machine->addProgressive(1);
        machine->addProgressive(2);
        machine->setProgressiveValue(1, 1250.50);
        machine->setProgressiveValue(2, 5000.75);
        std::cout << "  Level 1: $" << machine->getProgressive(1) << std::endl;
        std::cout << "  Level 2: $" << machine->getProgressive(2) << std::endl;

        // Start machine
        machine->start();
        std::cout << "\nMachine started: " << (machine->isStarted() ? "Yes" : "No") << std::endl;
        std::cout << "Machine playable: " << (machine->isPlayable() ? "Yes" : "No") << std::endl;

        // Play some games
        std::cout << "\nPlaying games..." << std::endl;
        machine->bet(3);  // Bet 3 credits
        machine->GameWon();

        machine->bet(5);  // Max bet
        machine->GameLost();

        // Show meters
        std::cout << "\nMachine meters:" << std::endl;
        std::cout << "  Games played: " << machine->getGamesPlayed() << std::endl;
        std::cout << "  Games won: " << machine->getMeter(megamic::sas::SASConstants::MTR_GAMES_WON) << std::endl;
        std::cout << "  Games lost: " << machine->getMeter(megamic::sas::SASConstants::MTR_GAMES_LOST) << std::endl;
        std::cout << "  Coin in: $" << machine->getCoinInMeter() << std::endl;
        std::cout << "  Current credits: " << machine->getCredits()
                  << " ($" << machine->getCashableAmount() << ")" << std::endl;

        // Test door open/close
        std::cout << "\nTesting door events..." << std::endl;
        machine->setDoorOpen(true);
        std::cout << "  Door open: " << (machine->isDoorOpen() ? "Yes" : "No") << std::endl;
        std::cout << "  Machine playable: " << (machine->isPlayable() ? "Yes" : "No") << std::endl;
        machine->setDoorOpen(false);
        std::cout << "  Door closed - Machine playable: " << (machine->isPlayable() ? "Yes" : "No") << std::endl;

        // Stop machine
        machine->stop();
        std::cout << "\nMachine stopped successfully" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "\n===============================" << std::endl;
    std::cout << "EGM Emulator test completed successfully!" << std::endl;

    return 0;
}
