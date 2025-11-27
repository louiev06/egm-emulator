#include "megamic/sas/commands/MeterCommands.h"
#include "megamic/sas/BCD.h"
#include "megamic/sas/SASConstants.h"

namespace megamic {
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

    Message response;
    response.address = 1;
    response.command = LongPoll::SEND_GAME_CONFIG;

    // Game configuration format:
    // Byte 0: Game number (BCD)
    // Byte 1-2: Additional information
    // Byte 3-6: Selected meters (varies by implementation)

    // Get current game (use first enabled game or game 1)
    auto games = machine->getGames();
    int gameNumber = games.empty() ? 1 : games[0]->getGameNumber();
    response.data.push_back(BCD::toBCD(static_cast<uint8_t>(gameNumber)));

    // Add denomination code (use first game's denomination)
    double denom = games.empty() ? 0.01 : games[0]->getDenom();
    int denomCode = SASConstants::DENOMINATIONS.getDenomCodeByDenomination(denom);
    response.data.push_back(static_cast<uint8_t>(denomCode));

    // Add max bet (in credits)
    int maxBet = machine->getMaxMaxBet();
    response.data.push_back(BCD::toBCD(static_cast<uint8_t>(maxBet & 0xFF)));

    // Add progressive group (default 0 for no progressive)
    response.data.push_back(0x00);

    // Add key game meters (4 bytes each, BCD)
    // Coin in for this game
    uint64_t gameCoinIn = machine->getMeter(SASConstants::METER_COIN_IN);
    std::vector<uint8_t> coinInBCD = BCD::encode(gameCoinIn, 4);
    response.data.insert(response.data.end(), coinInBCD.begin(), coinInBCD.end());

    // Coin out for this game
    uint64_t gameCoinOut = machine->getMeter(SASConstants::METER_COIN_OUT);
    std::vector<uint8_t> coinOutBCD = BCD::encode(gameCoinOut, 4);
    response.data.insert(response.data.end(), coinOutBCD.begin(), coinOutBCD.end());

    // Games played for this game
    uint64_t gamesPlayed = machine->getMeter(SASConstants::METER_GAMES_PLAYED);
    std::vector<uint8_t> gamesPlayedBCD = BCD::encode(gamesPlayed, 4);
    response.data.insert(response.data.end(), gamesPlayedBCD.begin(), gamesPlayedBCD.end());

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

} // namespace commands
} // namespace sas
} // namespace megamic
