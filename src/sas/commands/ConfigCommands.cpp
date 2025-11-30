#include "sas/commands/ConfigCommands.h"
#include "sas/SASConstants.h"
#include "sas/BCD.h"
#include "utils/Logger.h"
#include <cstring>


namespace sas {
namespace commands {

Message ConfigCommands::handleSendMachineID(simulator::Machine* machine) {
    if (!machine) {
        return Message();
    }

    // 0x54: Send Gaming Machine ID and Serial Number
    // Response format (variable length):
    // [Address][0x54][Length][SAS Version(3 ASCII)][Serial Number(0-40 ASCII)][CRC]
    //
    // Based on real EGM behavior:
    // - Length byte = number of bytes following (NOT including CRC)
    // - SAS Version = 3 ASCII characters (e.g., "602" for SAS 6.02)
    // - Serial Number = 0-40 ASCII characters
    //
    // Example from real EGM with version 6.02 and serial "000001":
    // [01][54][09][36 30 32][30 30 30 30 30 31][CRC CRC]
    //                ^"602"    ^"000001"
    // Length = 0x09 = 9 bytes (3 version + 6 serial)

    Message response;
    response.address = 1;
    response.command = 0x54;  // Send Machine ID and Serial Number

    // Get machine serial number (default to "000001" to match real EGM)
    std::string serialNumber = "000001";

    // SAS Version as ASCII string (3 characters) - "602" for SAS Protocol 6.02
    std::string sasVersion = "602";

    // Calculate length byte (version + serial number)
    uint8_t lengthByte = static_cast<uint8_t>(sasVersion.length() + serialNumber.length());

    // Build response data:
    // [Length][SAS Version(3 ASCII)][SerialNumber(N ASCII)]
    response.data.push_back(lengthByte);

    // Add SAS version as ASCII bytes
    for (char c : sasVersion) {
        response.data.push_back(static_cast<uint8_t>(c));
    }

    // Add serial number as ASCII bytes
    for (char c : serialNumber) {
        response.data.push_back(static_cast<uint8_t>(c));
    }

    utils::Logger::log("[0x54] Machine ID Response:");
    utils::Logger::log("  SAS Version: " + sasVersion);
    utils::Logger::log("  Serial: " + serialNumber);
    utils::Logger::log("  Length byte: 0x" +
        [](uint8_t val) {
            char buf[3];
            snprintf(buf, sizeof(buf), "%02X", val);
            return std::string(buf);
        }(lengthByte));
    utils::Logger::log("  Total data bytes: " + std::to_string(response.data.size()));

    return response;
}

Message ConfigCommands::handleSendNumberOfGames(simulator::Machine* machine) {
    if (!machine) {
        return Message();
    }

    // 0x51: Send Number of Games Implemented
    // Response format (6 bytes total):
    // [Address][0x51][Number of Games (2 BCD)][CRC]

    Message response;
    response.address = 1;
    response.command = 0x51;

    // Get total number of games (sub-games) implemented
    auto games = machine->getGames();
    int numGames = static_cast<int>(games.size());

    // Encode as 2-byte BCD (max 99 games)
    std::vector<uint8_t> numGamesBCD = BCD::encode(numGames, 2);
    response.data = numGamesBCD;

    utils::Logger::log("[0x51] Number of Games: " + std::to_string(numGames));
    return response;
}

Message ConfigCommands::handleSendSelectedGameNumber(simulator::Machine* machine) {
    if (!machine) {
        return Message();
    }

    // 0x55: Send Selected Game Number
    // Response format (6 bytes total):
    // [Address][0x55][Selected Game Number (2 BCD)][CRC]

    Message response;
    response.address = 1;
    response.command = 0x55;

    // Get currently selected game number
    auto games = machine->getGames();
    int selectedGame = 0;  // Default to game 0 (main EGM)

    if (!games.empty()) {
        // Get first enabled game
        selectedGame = games[0]->getGameNumber();
    }

    // Encode as 2-byte BCD
    std::vector<uint8_t> selectedGameBCD = BCD::encode(selectedGame, 2);
    response.data = selectedGameBCD;

    utils::Logger::log("[0x55] Selected Game Number: " + std::to_string(selectedGame));
    return response;
}

Message ConfigCommands::handleSendGameNConfiguration(simulator::Machine* machine, const std::vector<uint8_t>& data) {
    if (!machine) {
        return Message();
    }

    // 0x53: Send Game N Configuration
    // Input: [Game Number (2 BCD)]
    // Response format (28 bytes total):
    // [Address][0x53][Length][Game Number(2)][Game ID(2)][Additional ID(3)]
    // [Denomination(1)][Max Bet(1)][Progressive Group(1)][Game Options(2)]
    // [Pay Table(6)][Base Percent(4)][CRC(2)]

    // Parse game number from input data
    if (data.size() < 2) {
        utils::Logger::log("[0x53] ERROR: Insufficient data for game number");
        return Message();
    }

    // Decode BCD game number (2 bytes)
    uint64_t gameNumber = BCD::decode(data.data(), 2);

    utils::Logger::log("[0x53] Send Game N Configuration for game " + std::to_string(gameNumber));

    Message response;
    response.address = 1;
    response.command = 0x53;

    // Length byte (number of bytes following, excluding CRC)
    // Total: 2 (game#) + 2 (gameID) + 3 (addID) + 1 (denom) + 1 (maxbet) +
    //        1 (prog) + 2 (options) + 6 (paytable) + 4 (basepercent) = 22 bytes
    response.data.push_back(22);

    // Echo back the game number (2 bytes BCD)
    response.data.push_back(data[0]);
    response.data.push_back(data[1]);

    // Game ID (2 bytes ASCII - use "01" for testing)
    response.data.push_back('0');
    response.data.push_back('1');

    // Additional ID (3 bytes - manufacturer specific, use 00 00 00)
    response.data.push_back(0x00);
    response.data.push_back(0x00);
    response.data.push_back(0x00);

    // Get game denomination from first game (or default to penny)
    auto games = machine->getGames();
    double denom = 0.01;  // Default penny
    if (!games.empty() && gameNumber < games.size()) {
        denom = games[gameNumber]->getDenom();
    }

    // Denomination code (1 byte)
    int denomCode = SASConstants::DENOMINATIONS.getDenomCodeByDenomination(denom);
    response.data.push_back(static_cast<uint8_t>(denomCode));

    // Max Bet (1 byte - credits)
    int maxBet = machine->getMaxMaxBet();
    response.data.push_back(static_cast<uint8_t>(maxBet));

    // Progressive Group (1 byte - 0 = not progressive)
    response.data.push_back(0x00);

    // Game Options (2 bytes - manufacturer specific, use 00 00)
    response.data.push_back(0x00);
    response.data.push_back(0x00);

    // Pay Table ID (6 bytes ASCII - use spaces for generic)
    for (int i = 0; i < 6; i++) {
        response.data.push_back(' ');
    }

    // Base Percent (4 bytes BCD - theoretical payback percentage)
    // Use 95.00% = 9500 in BCD = 00 00 95 00
    response.data.push_back(0x00);
    response.data.push_back(0x00);
    response.data.push_back(0x95);
    response.data.push_back(0x00);

    utils::Logger::log("[0x53] Response data size: " + std::to_string(response.data.size()) + " bytes (expecting 23: 1 length + 22 data)");

    return response;
}

Message ConfigCommands::handleSendEnabledGameNumbers(simulator::Machine* machine) {
    if (!machine) {
        return Message();
    }

    // 0x56: Send Enabled Game Numbers
    // Response format (variable length):
    // [Address][0x56][Length][Number of Games(1)][Game1(2 BCD)][Game2(2 BCD)]...[CRC(2)]

    Message response;
    response.address = 1;
    response.command = 0x56;

    // Get all enabled games
    auto games = machine->getGames();
    uint8_t numGames = static_cast<uint8_t>(games.size());

    utils::Logger::log("[0x56] Send Enabled Game Numbers: " + std::to_string(numGames) + " games");

    // Calculate length byte (1 byte for count + 2 bytes per game)
    uint8_t lengthByte = 1 + (numGames * 2);
    response.data.push_back(lengthByte);

    // Number of games (1 byte)
    response.data.push_back(numGames);

    // Add each game number (2 bytes BCD each)
    for (const auto& game : games) {
        int gameNum = game->getGameNumber();
        std::vector<uint8_t> gameNumBCD = BCD::encode(gameNum, 2);
        response.data.insert(response.data.end(), gameNumBCD.begin(), gameNumBCD.end());

        utils::Logger::log("[0x56]   Game " + std::to_string(gameNum) + " enabled");
    }

    utils::Logger::log("[0x56] Response data size: " + std::to_string(response.data.size()) +
                      " bytes (1 length + 1 count + " + std::to_string(numGames * 2) + " game data)");

    return response;
}

Message ConfigCommands::handleEnableDisableGameN(simulator::Machine* machine, const std::vector<uint8_t>& data) {
    if (!machine) {
        return Message();
    }

    // 0xA0: Enable/Disable Game N
    // Input: [Game Number (2 BCD)]
    // Response format (12 bytes total):
    // [Address][0xA0][Game Number(2 BCD)][Gaming Machine Capabilities Flags1][Flags2][Flags3][CRC(2)]

    // Parse game number from input data
    if (data.size() < 2) {
        utils::Logger::log("[0xA0] ERROR: Insufficient data for game number");
        return Message();
    }

    // Decode BCD game number (2 bytes)
    uint64_t gameNumber = BCD::decode(data.data(), 2);

    utils::Logger::log("[0xA0] Enable/Disable Game N for game " + std::to_string(gameNumber));

    Message response;
    response.address = 1;
    response.command = 0xA0;

    // Echo back the game number (2 bytes BCD)
    response.data.push_back(data[0]);
    response.data.push_back(data[1]);

    // Gaming Machine Capabilities Flags
    // Based on real EGM response: D7 CD 05 00 00 00
    // Response format: [Game#(2)][Flags1][Flags2][Flags3][Reserved(3)][CRC(2)]

    // Flags1 (Features1): 0xD7 = 1101 0111
    //   Bit 0: Jackpot Multiplier = 1
    //   Bit 1: AFT Bonus Awards = 1
    //   Bit 2: Legacy Bonus Awards = 1
    //   Bit 3: Tournament = 0
    //   Bit 4: Validation Extensions = 1
    //   Bits 5-6: Validation Style = 10 (System Validation)
    //   Bit 7: Ticket Redemption = 1
    uint8_t flags1 = 0xD7;

    // Flags2 (Features2): 0xCD = 1100 1101
    //   Bits 0-1: Meter Model = 01 (SAS 4.x)
    //   Bit 2: Tickets to Total Drop = 1
    //   Bit 3: Extended Meters = 1
    //   Bit 4: Component Authentication = 0
    //   Bit 5: Reserved = 0
    //   Bit 6: AFT = 1
    //   Bit 7: Multi-Denom Extensions = 1
    uint8_t flags2 = 0xCD;

    // Flags3 (Features3): 0x05 = 0000 0101
    //   Bit 0: Maximum Polling Rate (40ms) = 1
    //   Bit 1: Reserved = 0
    //   Bit 2: Multi-level progressive = 1
    //   Bits 3-7: Reserved = 0
    uint8_t flags3 = 0x05;

    response.data.push_back(flags1);
    response.data.push_back(flags2);
    response.data.push_back(flags3);

    // Add 3 reserved bytes (always 0x00)
    response.data.push_back(0x00);
    response.data.push_back(0x00);
    response.data.push_back(0x00);

    utils::Logger::log("[0xA0] Gaming Machine Capabilities:");
    utils::Logger::log("  Flags1: 0x" +
        [](uint8_t val) {
            char buf[3];
            snprintf(buf, sizeof(buf), "%02X", val);
            return std::string(buf);
        }(flags1) + " (Jackpot Mult, AFT Bonus, Legacy Bonus, Validation, Ticket Redemption)");
    utils::Logger::log("  Flags2: 0x" +
        [](uint8_t val) {
            char buf[3];
            snprintf(buf, sizeof(buf), "%02X", val);
            return std::string(buf);
        }(flags2) + " (SAS4 Meters, Tickets to Drop, Extended Meters, AFT, Multi-Denom)");
    utils::Logger::log("  Flags3: 0x" +
        [](uint8_t val) {
            char buf[3];
            snprintf(buf, sizeof(buf), "%02X", val);
            return std::string(buf);
        }(flags3) + " (40ms polling, Multi-level progressive)");

    return response;
}

} // namespace commands
} // namespace sas
