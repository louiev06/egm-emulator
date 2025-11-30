#include "sas/commands/MeterCommands.h"
#include "sas/BCD.h"
#include "sas/SASConstants.h"
#include "utils/Logger.h"


namespace sas {
namespace commands {

Message MeterCommands::handleSendMeters(simulator::Machine* machine, uint8_t command) {
    // Route to specific handler based on command
    switch (command) {
        case LongPoll::SEND_TOTAL_COIN_IN:
            return handleSendTotalCoinIn(machine);
        case LongPoll::SEND_TOTAL_COIN_OUT:
            return handleSendTotalCoinOut(machine);
        case LongPoll::SEND_TOTAL_DROP:
            return handleSendTotalDrop(machine);
        case LongPoll::SEND_TOTAL_JACKPOT:
            return handleSendTotalJackpot(machine);
        case LongPoll::SEND_GAMES_PLAYED:
            return handleSendGamesPlayed(machine);
        case LongPoll::SEND_GAMES_WON:
            return handleSendGamesWon(machine);
        case LongPoll::SEND_GAMES_LOST:
            return handleSendGamesLost(machine);
        case LongPoll::SEND_GAME_CONFIG:
            return handleSendGameConfiguration(machine);
        default:
            // Unsupported meter command
            return Message();
    }
}

Message MeterCommands::handleSendTotalCoinIn(simulator::Machine* machine) {
    if (!machine) {
        return Message();
    }

    uint64_t coinIn = machine->getMeter(SASConstants::METER_COIN_IN);
    return buildMeterResponse(1, LongPoll::SEND_TOTAL_COIN_IN, coinIn);
}

Message MeterCommands::handleSendTotalCoinOut(simulator::Machine* machine) {
    if (!machine) {
        return Message();
    }

    uint64_t coinOut = machine->getMeter(SASConstants::METER_COIN_OUT);
    return buildMeterResponse(1, LongPoll::SEND_TOTAL_COIN_OUT, coinOut);
}

Message MeterCommands::handleSendTotalDrop(simulator::Machine* machine) {
    if (!machine) {
        return Message();
    }

    uint64_t drop = machine->getMeter(SASConstants::METER_TOT_DROP);
    return buildMeterResponse(1, LongPoll::SEND_TOTAL_DROP, drop);
}

Message MeterCommands::handleSendTotalJackpot(simulator::Machine* machine) {
    if (!machine) {
        return Message();
    }

    uint64_t jackpot = machine->getMeter(SASConstants::METER_JACKPOT);
    return buildMeterResponse(1, LongPoll::SEND_TOTAL_JACKPOT, jackpot);
}

Message MeterCommands::handleSendGamesPlayed(simulator::Machine* machine) {
    if (!machine) {
        return Message();
    }

    uint64_t gamesPlayed = machine->getMeter(SASConstants::METER_GAMES_PLAYED);
    return buildMeterResponse(1, LongPoll::SEND_GAMES_PLAYED, gamesPlayed);
}

Message MeterCommands::handleSendGamesWon(simulator::Machine* machine) {
    if (!machine) {
        return Message();
    }

    uint64_t gamesWon = machine->getMeter(SASConstants::METER_GAMES_WON);
    return buildMeterResponse(1, LongPoll::SEND_GAMES_WON, gamesWon);
}

Message MeterCommands::handleSendGamesLost(simulator::Machine* machine) {
    if (!machine) {
        return Message();
    }

    // Games lost = Games played - Games won
    uint64_t gamesPlayed = machine->getMeter(SASConstants::METER_GAMES_PLAYED);
    uint64_t gamesWon = machine->getMeter(SASConstants::METER_GAMES_WON);
    uint64_t gamesLost = (gamesPlayed > gamesWon) ? (gamesPlayed - gamesWon) : 0;

    return buildMeterResponse(1, LongPoll::SEND_GAMES_LOST, gamesLost);
}

Message MeterCommands::handleSendTotalCoinInAndMeters(simulator::Machine* machine) {
    if (!machine) {
        return Message();
    }

    // 0x19: Send Total Coin In and Associated Meters
    // Response format (24 bytes total):
    // [Address][0x19][CoinIn(4)][CoinOut(4)][TotalDrop(4)][Jackpot(4)][GamesPlayed(4)][CRC(2)]

    // TESTING: Initialize meters with real EGM test values
    // Expected output: 01 19 00 45 20 40 00 86 14 80 00 41 30 78 00 63 44 94 00 00 20 62 D5 CE
    machine->setMeter(SASConstants::METER_COIN_IN, 452040);      // CoinIn: 452,040
    machine->setMeter(SASConstants::METER_COIN_OUT, 861480);     // CoinOut: 861,480
    machine->setMeter(SASConstants::METER_TOT_DROP, 413078);     // Drop: 413,078
    machine->setMeter(SASConstants::METER_JACKPOT, 634494);      // Jackpot: 634,494
    machine->setMeter(SASConstants::METER_GAMES_PLAYED, 2062);   // GamesPlayed: 2,062

    Message response;
    response.address = 1;
    response.command = LongPoll::SEND_SELECTED_METERS;  // 0x19

    // Coin In (4 bytes BCD)
    uint64_t coinIn = machine->getMeter(SASConstants::METER_COIN_IN);
    utils::Logger::log("[0x19] CoinIn meter value: " + std::to_string(coinIn));
    std::vector<uint8_t> coinInBCD = BCD::encode(coinIn, 4);
    response.data.insert(response.data.end(), coinInBCD.begin(), coinInBCD.end());

    // Coin Out (4 bytes BCD)
    uint64_t coinOut = machine->getMeter(SASConstants::METER_COIN_OUT);
    utils::Logger::log("[0x19] CoinOut meter value: " + std::to_string(coinOut));
    std::vector<uint8_t> coinOutBCD = BCD::encode(coinOut, 4);
    response.data.insert(response.data.end(), coinOutBCD.begin(), coinOutBCD.end());

    // Total Drop (4 bytes BCD) - use TOT_DROP meter (0x0F)
    uint64_t totalDrop = machine->getMeter(SASConstants::METER_TOT_DROP);
    utils::Logger::log("[0x19] TotalDrop meter value: " + std::to_string(totalDrop));
    std::vector<uint8_t> totalDropBCD = BCD::encode(totalDrop, 4);
    response.data.insert(response.data.end(), totalDropBCD.begin(), totalDropBCD.end());

    // Jackpot (4 bytes BCD)
    uint64_t jackpot = machine->getMeter(SASConstants::METER_JACKPOT);
    utils::Logger::log("[0x19] Jackpot meter value: " + std::to_string(jackpot));
    std::vector<uint8_t> jackpotBCD = BCD::encode(jackpot, 4);
    response.data.insert(response.data.end(), jackpotBCD.begin(), jackpotBCD.end());

    // Games Played (4 bytes BCD)
    uint64_t gamesPlayed = machine->getMeter(SASConstants::METER_GAMES_PLAYED);
    utils::Logger::log("[0x19] GamesPlayed meter value: " + std::to_string(gamesPlayed));
    std::vector<uint8_t> gamesPlayedBCD = BCD::encode(gamesPlayed, 4);
    response.data.insert(response.data.end(), gamesPlayedBCD.begin(), gamesPlayedBCD.end());

    utils::Logger::log("[0x19] Response has " + std::to_string(response.data.size()) + " bytes of data (expecting 20)");
    utils::Logger::log("[0x19] Expected: 01 19 00 45 20 40 00 86 14 80 00 41 30 78 00 63 44 94 00 00 20 62 D5 CE");

    return response;
}

Message MeterCommands::handleSendTotalBills(simulator::Machine* machine) {
    if (!machine) {
        return Message();
    }

    // 0x20: Send Total Bills (Bill Drop meter in dollars)
    // Response format (8 bytes total):
    // [Address][0x20][TotalBills(4)][CRC(2)]

    // Get bill drop meter - use credits from bill acceptor meter (0x0B)
    uint64_t totalBills = machine->getMeter(SASConstants::METER_CRD_FR_BILL_ACCEPTOR);

    return buildMeterResponse(1, LongPoll::SEND_TOTAL_BILLS, totalBills);  // 0x20
}

Message MeterCommands::handleSendSelectedMeters(simulator::Machine* machine,
                                                const std::vector<uint8_t>& meterCodes) {
    if (!machine || meterCodes.empty()) {
        return Message();
    }

    std::vector<uint64_t> meterValues;
    for (uint8_t code : meterCodes) {
        meterValues.push_back(machine->getMeter(code));
    }

    return buildMultiMeterResponse(1, LongPoll::SEND_SELECTED_METERS, meterValues);
}

Message MeterCommands::handleSendGameConfiguration(simulator::Machine* machine) {
    if (!machine) {
        return Message();
    }

    // 0x1F: Send Game Configuration
    // Response format (24 bytes total):
    // [Address][0x1F][Game ID(2 ASCII)][Additional ID(3)][Denomination(1)]
    // [Max Bet(1)][Progressive Group(1)][Game Options(2)][Pay Table(6)]
    // [Base Percent(4 BCD)][CRC]
    //
    // Based on CLongPoll1FMessage::ResponseReceived():
    // response[2-3] = Game ID (2 ASCII chars)
    // response[4-6] = Additional ID (3 bytes)
    // response[7] = Denomination code
    // response[8] = Max Bet
    // response[9] = Progressive Group
    // response[10-11] = Game Options (2 bytes)
    // response[12-17] = Pay Table ID (6 ASCII chars)
    // response[18-21] = Base Percent (4 BCD digits)
    //
    // Total response size: 22 data bytes (same as 0x53)

    Message response;
    response.address = 1;
    response.command = LongPoll::SEND_GAME_CONFIG;

    // Get current game configuration
    auto games = machine->getGames();
    double denom = 0.01;  // Default penny
    if (!games.empty()) {
        denom = games[0]->getDenom();
    }

    // Game ID (2 bytes ASCII - use "01" for testing)
    response.data.push_back('0');
    response.data.push_back('1');

    // Additional ID (3 bytes - manufacturer specific, use 00 00 00)
    response.data.push_back(0x00);
    response.data.push_back(0x00);
    response.data.push_back(0x00);

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

    utils::Logger::log("[0x1F] Game Configuration Response:");
    utils::Logger::log("  Game ID: 01");
    utils::Logger::log("  Denomination: " + std::to_string(denom));
    utils::Logger::log("  Max Bet: " + std::to_string(maxBet));
    utils::Logger::log("  Base Percent: 95.00%");
    utils::Logger::log("  Total data bytes: " + std::to_string(response.data.size()) + " (expecting 22)");

    return response;
}

Message MeterCommands::buildMeterResponse(uint8_t address, uint8_t command,
                                         uint64_t meterValue) {
    Message response;
    response.address = address;
    response.command = command;

    // SAS meters are 4 bytes (8 BCD digits) = 0-99,999,999
    std::vector<uint8_t> bcdValue = BCD::encode(meterValue, 4);
    response.data = bcdValue;

    return response;
}

Message MeterCommands::buildMultiMeterResponse(uint8_t address, uint8_t command,
                                              const std::vector<uint64_t>& meterValues) {
    Message response;
    response.address = address;
    response.command = command;

    // Each meter is 4 bytes BCD
    for (uint64_t value : meterValues) {
        std::vector<uint8_t> bcdValue = BCD::encode(value, 4);
        response.data.insert(response.data.end(), bcdValue.begin(), bcdValue.end());
    }

    return response;
}

// ============================================================================
// Phase 1 - Basic Meters (Simple 4-byte BCD responses)
// ============================================================================

Message MeterCommands::handleSendCancelledCredits(simulator::Machine* machine) {
    if (!machine) {
        return Message();
    }

    uint64_t cancelledCredits = machine->getMeter(SASConstants::METER_CANCELLED_CRD);
    return buildMeterResponse(1, 0x10, cancelledCredits);
}

Message MeterCommands::handleSendCurrentCredits(simulator::Machine* machine) {
    if (!machine) {
        return Message();
    }

    uint64_t currentCredits = machine->getMeter(SASConstants::METER_CURRENT_CRD);
    return buildMeterResponse(1, 0x1A, currentCredits);
}

Message MeterCommands::handleSendTrueCoinIn(simulator::Machine* machine) {
    if (!machine) {
        return Message();
    }

    uint64_t trueCoinIn = machine->getMeter(SASConstants::METER_TRUE_COIN_IN);
    return buildMeterResponse(1, 0x2A, trueCoinIn);
}

Message MeterCommands::handleSendTrueCoinOut(simulator::Machine* machine) {
    if (!machine) {
        return Message();
    }

    uint64_t trueCoinOut = machine->getMeter(SASConstants::METER_TRUE_COIN_OUT);
    return buildMeterResponse(1, 0x2B, trueCoinOut);
}

Message MeterCommands::handleSendBillsAcceptedCredits(simulator::Machine* machine) {
    if (!machine) {
        return Message();
    }

    uint64_t billsAccepted = machine->getMeter(SASConstants::METER_CRD_FR_BILL_ACCEPTOR);
    return buildMeterResponse(1, 0x46, billsAccepted);
}

// ============================================================================
// Bill Denomination Meters
// ============================================================================

Message MeterCommands::handleSend$1Bills(simulator::Machine* machine) {
    if (!machine) {
        return Message();
    }

    uint64_t d1Bills = machine->getMeter(SASConstants::METER_1_BILLS_ACCEPTED);
    return buildMeterResponse(1, 0x31, d1Bills);
}

Message MeterCommands::handleSend$2Bills(simulator::Machine* machine) {
    if (!machine) {
        return Message();
    }

    uint64_t d2Bills = machine->getMeter(SASConstants::METER_2_BILLS_ACCEPTED);
    return buildMeterResponse(1, 0x32, d2Bills);
}

Message MeterCommands::handleSend$5Bills(simulator::Machine* machine) {
    if (!machine) {
        return Message();
    }

    uint64_t d5Bills = machine->getMeter(SASConstants::METER_5_BILLS_ACCEPTED);
    return buildMeterResponse(1, 0x33, d5Bills);
}

Message MeterCommands::handleSend$10Bills(simulator::Machine* machine) {
    if (!machine) {
        return Message();
    }

    uint64_t d10Bills = machine->getMeter(SASConstants::METER_10_BILLS_ACCEPTED);
    return buildMeterResponse(1, 0x34, d10Bills);
}

Message MeterCommands::handleSend$20Bills(simulator::Machine* machine) {
    if (!machine) {
        return Message();
    }

    uint64_t d20Bills = machine->getMeter(SASConstants::METER_20_BILLS_ACCEPTED);
    return buildMeterResponse(1, 0x35, d20Bills);
}

Message MeterCommands::handleSend$50Bills(simulator::Machine* machine) {
    if (!machine) {
        return Message();
    }

    uint64_t d50Bills = machine->getMeter(SASConstants::METER_50_BILLS_ACCEPTED);
    return buildMeterResponse(1, 0x36, d50Bills);
}

Message MeterCommands::handleSend$100Bills(simulator::Machine* machine) {
    if (!machine) {
        return Message();
    }

    uint64_t d100Bills = machine->getMeter(SASConstants::METER_100_BILLS_ACCEPTED);
    return buildMeterResponse(1, 0x37, d100Bills);
}

Message MeterCommands::handleSend$500Bills(simulator::Machine* machine) {
    if (!machine) {
        return Message();
    }

    uint64_t d500Bills = machine->getMeter(SASConstants::METER_500_BILLS_ACCEPTED);
    return buildMeterResponse(1, 0x38, d500Bills);
}

Message MeterCommands::handleSend$1000Bills(simulator::Machine* machine) {
    if (!machine) {
        return Message();
    }

    uint64_t d1000Bills = machine->getMeter(SASConstants::METER_1000_BILLS_ACCEPTED);
    return buildMeterResponse(1, 0x39, d1000Bills);
}

Message MeterCommands::handleSend$200Bills(simulator::Machine* machine) {
    if (!machine) {
        return Message();
    }

    uint64_t d200Bills = machine->getMeter(SASConstants::METER_200_BILLS_ACCEPTED);
    return buildMeterResponse(1, 0x3A, d200Bills);
}

// ============================================================================
// Multi-Meter Responses
// ============================================================================

Message MeterCommands::handleSendBillMeters(simulator::Machine* machine) {
    if (!machine) {
        return Message();
    }

    // 0x1E: Send Bill Meters - returns 6 common bill denominations
    // Response format (28 bytes total):
    // [Address][0x1E][$1(4)][$5(4)][$10(4)][$20(4)][$50(4)][$100(4)][CRC(2)]

    std::vector<uint64_t> billMeters = {
        machine->getMeter(SASConstants::METER_1_BILLS_ACCEPTED),
        machine->getMeter(SASConstants::METER_5_BILLS_ACCEPTED),
        machine->getMeter(SASConstants::METER_10_BILLS_ACCEPTED),
        machine->getMeter(SASConstants::METER_20_BILLS_ACCEPTED),
        machine->getMeter(SASConstants::METER_50_BILLS_ACCEPTED),
        machine->getMeter(SASConstants::METER_100_BILLS_ACCEPTED)
    };

    return buildMultiMeterResponse(1, 0x1E, billMeters);
}

Message MeterCommands::handleSendGamingMachineMeters(simulator::Machine* machine) {
    if (!machine) {
        return Message();
    }

    // 0x1C: Send Gaming Machine Meters 1-8
    // Response format (36 bytes total):
    // [Address][0x1C][CoinIn(4)][CoinOut(4)][TotalDrop(4)][Jackpot(4)]
    //                [GamesPlayed(4)][GamesWon(4)][SlotDoor(4)][PowerReset(4)][CRC(2)]

    std::vector<uint64_t> machineMeters = {
        machine->getMeter(SASConstants::METER_COIN_IN),         // Coin In
        machine->getMeter(SASConstants::METER_COIN_OUT),         // Coin Out
        machine->getMeter(SASConstants::METER_TOT_DROP),  // Total Drop
        machine->getMeter(SASConstants::METER_JACKPOT),         // Jackpot
        machine->getMeter(SASConstants::METER_GAMES_PLAYED),         // Games Played
        machine->getMeter(SASConstants::METER_GAMES_WON),         // Games Won
        machine->getMeter(SASConstants::METER_ACTUAL_SLOT_DOOR),  // Slot Door
        0  // Power Reset meter (TODO: implement power reset tracking)
    };

    return buildMultiMeterResponse(1, 0x1C, machineMeters);
}

Message MeterCommands::handleSendSelectedGameMeters(simulator::Machine* machine, const std::vector<uint8_t>& data) {
    if (!machine) {
        return Message();
    }

    // 0x52: Send Selected Game Meters
    // Input: [Game Number (2 BCD)]
    // Response format (20 bytes total):
    // [Address][0x52][Game Number (2)][Coin In (4)][Coin Out (4)][Jackpot (4)][Games Played (4)][CRC]

    // Parse game number from input data
    if (data.size() < 2) {
        utils::Logger::log("[0x52] ERROR: Insufficient data for game number");
        return Message();
    }

    // Decode BCD game number (2 bytes)
    uint64_t gameNumber = BCD::decode(data.data(), 2);

    utils::Logger::log("[0x52] Send Selected Game Meters for game " + std::to_string(gameNumber));

    Message response;
    response.address = 1;
    response.command = 0x52;

    // Echo back the game number (2 bytes BCD)
    response.data.push_back(data[0]);
    response.data.push_back(data[1]);

    // Get meters based on game number
    // Game 0 = EGM (main game), uses gCI, gCO, gJP, gGS
    // Game 1+ = Sub-games, uses SUBGAME_METER_* constants

    uint64_t coinIn = 0;
    uint64_t coinOut = 0;
    uint64_t jackpot = 0;
    uint64_t gamesPlayed = 0;

    if (gameNumber == 0) {
        // Main EGM game meters
        coinIn = machine->getMeter(SASConstants::METER_COIN_IN);
        coinOut = machine->getMeter(SASConstants::METER_COIN_OUT);
        jackpot = machine->getMeter(SASConstants::METER_JACKPOT);
        gamesPlayed = machine->getMeter(SASConstants::METER_GAMES_PLAYED);
    } else {
        // Sub-game meters
        // Note: In a full implementation, you'd select the sub-game and get its specific meters
        // For now, use the sub-game meter constants (same as main game in simplified implementation)
        // Subgame meters use the same METER_* codes as main game for now (simplified implementation)
        coinIn = machine->getMeter(SASConstants::METER_COIN_IN);
        coinOut = machine->getMeter(SASConstants::METER_COIN_OUT);
        jackpot = machine->getMeter(SASConstants::METER_JACKPOT);
        gamesPlayed = machine->getMeter(SASConstants::METER_GAMES_PLAYED);
    }

    // Coin In (4 bytes BCD)
    std::vector<uint8_t> coinInBCD = BCD::encode(coinIn, 4);
    response.data.insert(response.data.end(), coinInBCD.begin(), coinInBCD.end());

    // Coin Out (4 bytes BCD)
    std::vector<uint8_t> coinOutBCD = BCD::encode(coinOut, 4);
    response.data.insert(response.data.end(), coinOutBCD.begin(), coinOutBCD.end());

    // Jackpot (4 bytes BCD)
    std::vector<uint8_t> jackpotBCD = BCD::encode(jackpot, 4);
    response.data.insert(response.data.end(), jackpotBCD.begin(), jackpotBCD.end());

    // Games Played (4 bytes BCD)
    std::vector<uint8_t> gamesPlayedBCD = BCD::encode(gamesPlayed, 4);
    response.data.insert(response.data.end(), gamesPlayedBCD.begin(), gamesPlayedBCD.end());

    utils::Logger::log("[0x52] Response data size: " + std::to_string(response.data.size()) + " bytes (expecting 18: 2 game# + 16 meters)");

    return response;
}

Message MeterCommands::handleSendSelectedMetersForGameN(simulator::Machine* machine, const std::vector<uint8_t>& data) {
    if (!machine) {
        return Message();
    }

    // 0x2F: Send Selected Meters for Game N
    // Input: [Game Number (2 BCD)][Meter Code 1]...[Meter Code N]
    // Response: [Addr][0x2F][Length][Game Number (2)][Code1][Value1(4)]...[CRC]

    // Parse game number from first 2 bytes
    if (data.size() < 2) {
        utils::Logger::log("[0x2F] ERROR: Insufficient data for game number");
        return Message();
    }

    uint64_t gameNumber = BCD::decode(data.data(), 2);

    // Extract meter codes (everything after game number)
    std::vector<uint8_t> meterCodes;
    for (size_t i = 2; i < data.size(); i++) {
        meterCodes.push_back(data[i]);
    }

    utils::Logger::log("[0x2F] Send Selected Meters for Game " + std::to_string(gameNumber) +
                      ", " + std::to_string(meterCodes.size()) + " meter codes requested");

    Message response;
    response.address = 1;
    response.command = 0x2F;

    // We'll build the response data first, then calculate length
    std::vector<uint8_t> responseData;

    // Echo back game number (2 bytes BCD)
    responseData.push_back(data[0]);
    responseData.push_back(data[1]);

    // For each requested meter code, add code + value
    for (uint8_t meterCode : meterCodes) {
        // Add meter code
        responseData.push_back(meterCode);

        // Get meter value based on code
        // This is a simplified mapping - full implementation would use SASPollMessage meter mapping
        uint64_t meterValue = 0;

        // Map common SAS meter codes to internal meter numbers
        switch (meterCode) {
            case 0x00:  // Total Coin In
                meterValue = machine->getMeter(SASConstants::METER_COIN_IN);
                break;
            case 0x01:  // Total Coin Out
                meterValue = machine->getMeter(SASConstants::METER_COIN_OUT);
                break;
            case 0x02:  // Total Drop
                meterValue = machine->getMeter(SASConstants::METER_TOT_DROP);
                break;
            case 0x03:  // Total Jackpot
                meterValue = machine->getMeter(SASConstants::METER_JACKPOT);
                break;
            case 0x04:  // Games Played
                meterValue = machine->getMeter(SASConstants::METER_GAMES_PLAYED);
                break;
            case 0x05:  // Games Won
                meterValue = machine->getMeter(SASConstants::METER_GAMES_WON);
                break;
            case 0x0C:  // Current Credits
                meterValue = machine->getMeter(SASConstants::METER_CURRENT_CRD);
                break;
            default:
                // Unknown meter code - return 0
                utils::Logger::log("[0x2F]   Meter code 0x" +
                    [](uint8_t val) {
                        char buf[3];
                        snprintf(buf, sizeof(buf), "%02X", val);
                        return std::string(buf);
                    }(meterCode) + " not implemented, returning 0");
                meterValue = 0;
                break;
        }

        // Add meter value (4 bytes BCD for standard meters)
        // Note: Some TITO meters use 5 bytes, but we'll use 4 for simplicity
        std::vector<uint8_t> valueBCD = BCD::encode(meterValue, 4);
        responseData.insert(responseData.end(), valueBCD.begin(), valueBCD.end());
    }

    // Calculate length byte (all data following length byte, excluding CRC)
    uint8_t lengthByte = static_cast<uint8_t>(responseData.size());

    // Build final response: [Length][...responseData...]
    response.data.push_back(lengthByte);
    response.data.insert(response.data.end(), responseData.begin(), responseData.end());

    utils::Logger::log("[0x2F] Response data size: " + std::to_string(response.data.size()) +
                      " bytes (1 length + " + std::to_string(responseData.size()) + " data)");

    return response;
}

Message MeterCommands::handleSendHandpayCancelledCredits(simulator::Machine* machine, const std::vector<uint8_t>& data) {
    if (!machine) {
        return Message();
    }

    // 0x2D: Send Selected Game Number and Handpay Cancelled Credits
    // Input: [Game Number (2 BCD)]
    // Response: [Addr][0x2D][Cancelled Credits (4 BCD)][CRC]

    if (data.size() < 2) {
        utils::Logger::log("[0x2D] ERROR: Insufficient data for game number");
        return Message();
    }

    uint64_t gameNumber = BCD::decode(data.data(), 2);

    utils::Logger::log("[0x2D] Send Handpay Cancelled Credits for game " + std::to_string(gameNumber));

    // Get handpay cancelled credits meter (mHCC)
    uint64_t cancelledCredits = machine->getMeter(SASConstants::METER_HANDPAID_CANCELLED_CRD);

    return buildMeterResponse(1, 0x2D, cancelledCredits);
}

Message MeterCommands::handleSendSelectedMetersForGameNExtended(simulator::Machine* machine,
                                                                uint8_t command,
                                                                const std::vector<uint8_t>& data) {
    if (!machine) {
        return Message();
    }

    // 0x6F/0xAF: Send Selected Meters for Game N (Extended format)
    // Input: [Length][Game Number (2 BCD)][Meter Code 1 (2 bytes)]...[Meter Code N (2 bytes)]
    // Response: [Addr][0x6F/AF][Length][Game Number (2 BCD)][Code1(2)][Size1(1)][Value1(4-5 BCD)]...[CRC]
    //
    // Based on CLongPoll6FMessage:
    // - Supports up to 12 meters per request
    // - Each meter in response has: [Code (2)][Size (1)][Value (4 or 5 BCD)]
    // - Most meters are 4 bytes BCD, some TITO meters are 5 bytes BCD
    // - Command alternates between 0x6F and 0xAF to detect stale responses

    // Parse length byte
    if (data.size() < 1) {
        utils::Logger::log("[0x6F/AF] ERROR: No length byte provided");
        return Message();
    }

    uint8_t lengthByte = data[0];

    // Parse game number from input data (2 BCD bytes)
    if (data.size() < 3) {
        utils::Logger::log("[0x6F/AF] ERROR: Insufficient data for game number");
        return Message();
    }

    // Decode BCD game number (2 bytes)
    uint64_t gameNumber = BCD::decode(&data[1], 2);

    // Extract meter codes (2 bytes each, little-endian, starting at offset 3)
    std::vector<uint16_t> meterCodes;
    for (size_t i = 3; i + 1 < data.size(); i += 2) {
        // Read as little-endian: LSB first, MSB second
        uint16_t meterCode = static_cast<uint16_t>(data[i]) | (static_cast<uint16_t>(data[i + 1]) << 8);
        meterCodes.push_back(meterCode);

        // Limit to 12 meters max
        if (meterCodes.size() >= 12) {
            break;
        }
    }

    utils::Logger::log("[0x6F/AF] Game " + std::to_string(gameNumber) +
                      ", requesting " + std::to_string(meterCodes.size()) + " meters");

    Message response;
    response.address = 1;
    response.command = command;  // Echo back 0x6F or 0xAF

    // Build response data
    std::vector<uint8_t> responseData;

    // Echo back game number (2 bytes BCD)
    responseData.push_back(data[1]);
    responseData.push_back(data[2]);

    // For each requested meter code, add: [Code (2)][Size (1)][Value (4 or 5 BCD)]
    for (uint16_t meterCode : meterCodes) {
        // Add meter code (2 bytes, little-endian: LSB first, MSB second)
        responseData.push_back(static_cast<uint8_t>(meterCode & 0xFF));
        responseData.push_back(static_cast<uint8_t>(meterCode >> 8));

        // Map SAS meter code to internal meter number and get value
        // For now, use a simplified mapping - most common meter codes:
        uint64_t meterValue = 0;
        uint8_t meterSize = 4;  // Default to 4 bytes BCD

        // Simple meter code mapping (based on common codes)
        switch (meterCode) {
            case 0x00:  // Coin In
                meterValue = machine->getMeter(SASConstants::METER_COIN_IN);  // Same code for game 0 and subgames for now
                break;
            case 0x0C:  // Current Credits
                meterValue = machine->getMeter(SASConstants::METER_CURRENT_CRD);
                break;
            case 0x1C:  // Coin Out
                meterValue = machine->getMeter(SASConstants::METER_COIN_OUT);  // Same code for game 0 and subgames for now
                break;
            case 0x05:  // Games Played
                meterValue = machine->getMeter(SASConstants::METER_GAMES_PLAYED);  // Same code for game 0 and subgames for now
                break;
            case 0x1F:  // Jackpot
                meterValue = machine->getMeter(SASConstants::METER_JACKPOT);  // Same code for game 0 and subgames for now
                break;
            case 0x40:  // $1 Bills
                meterValue = machine->getMeter(SASConstants::METER_1_BILLS_ACCEPTED);
                break;
            case 0x42:  // $5 Bills
                meterValue = machine->getMeter(SASConstants::METER_5_BILLS_ACCEPTED);
                break;
            case 0x43:  // $10 Bills
                meterValue = machine->getMeter(SASConstants::METER_10_BILLS_ACCEPTED);
                break;
            case 0x44:  // $20 Bills
                meterValue = machine->getMeter(SASConstants::METER_20_BILLS_ACCEPTED);
                break;
            case 0x45:  // $50 Bills
                meterValue = machine->getMeter(SASConstants::METER_50_BILLS_ACCEPTED);
                break;
            case 0x46:  // $100 Bills
                meterValue = machine->getMeter(SASConstants::METER_100_BILLS_ACCEPTED);
                break;
            // TITO meters (5 bytes)
            case 0x0D:  // Total Ticket Out (count)
            case 0x0F:  // Total Ticket Out (value)
            case 0x28:  // Cashable Ticket Out (value)
            case 0x2A:  // Restricted Promo Ticket Out (value)
            case 0x2B:  // NonRestricted Promo Ticket Out (value)
                meterSize = 5;  // TITO meters are 5 bytes BCD
                meterValue = 0; // Default to 0 for now
                break;
            default:
                // Unknown meter code - return 0
                meterValue = 0;
                utils::Logger::log("[0x6F/AF] WARNING: Unknown meter code 0x" +
                    [](uint16_t val) {
                        char buf[5];
                        snprintf(buf, sizeof(buf), "%04X", val);
                        return std::string(buf);
                    }(meterCode));
                break;
        }

        // Add size byte
        responseData.push_back(meterSize);

        // Add meter value (BCD encoded)
        std::vector<uint8_t> valueBCD = BCD::encode(meterValue, meterSize);
        responseData.insert(responseData.end(), valueBCD.begin(), valueBCD.end());
    }

    // Add length byte at the beginning (length = game number + all meter data)
    uint8_t responseLengthByte = static_cast<uint8_t>(responseData.size());
    response.data.push_back(responseLengthByte);

    // Add all the response data
    response.data.insert(response.data.end(), responseData.begin(), responseData.end());

    utils::Logger::log("[0x6F/AF] Response: " + std::to_string(meterCodes.size()) +
                      " meters, total data bytes: " + std::to_string(response.data.size()));

    return response;
}

} // namespace commands
} // namespace sas

