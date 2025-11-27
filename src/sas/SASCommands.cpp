#include "sas/SASCommands.h"
#include "sas/CRC16.h"
#include <cstring>


namespace sas {

std::vector<uint8_t> Message::serialize() const {
    std::vector<uint8_t> buffer;
    buffer.reserve(length());

    // Add address and command
    buffer.push_back(address);
    buffer.push_back(command);

    // Add data
    buffer.insert(buffer.end(), data.begin(), data.end());

    // Calculate and append CRC
    uint16_t calculatedCrc = CRC16::calculate(buffer.data(), buffer.size());
    buffer.push_back(static_cast<uint8_t>(calculatedCrc & 0xFF));        // LSB
    buffer.push_back(static_cast<uint8_t>(calculatedCrc >> 8));          // MSB

    return buffer;
}

Message Message::parse(const uint8_t* buffer, size_t length) {
    Message msg;

    if (buffer == nullptr || length < 3) {
        // Minimum message: address + command + CRC (2 bytes)
        return msg;
    }

    // Extract address and command
    msg.address = buffer[0];
    msg.command = buffer[1];

    // Extract data (everything between command and CRC)
    if (length > 4) {  // Has data bytes
        size_t dataLength = length - 4;  // Subtract addr, cmd, and 2-byte CRC
        msg.data.assign(buffer + 2, buffer + 2 + dataLength);
    }

    // Extract CRC
    msg.crc = CRC16::extract(buffer, length);

    return msg;
}

const char* getCommandName(uint8_t command) {
    // Check if general poll first
    if (isGeneralPoll(command)) {
        return "General Poll";
    }

    // Long poll commands
    switch (command) {
        // Configuration
        case LongPoll::SEND_GAME_NUMBER:            return "Send Game Number";
        case LongPoll::SEND_GAME_CONFIG:            return "Send Game Configuration";
        case LongPoll::SEND_MACHINE_ID:             return "Send Machine ID";

        // Meters
        case LongPoll::SEND_TOTAL_COIN_IN:          return "Send Total Coin In";
        case LongPoll::SEND_TOTAL_COIN_OUT:         return "Send Total Coin Out";
        case LongPoll::SEND_TOTAL_DROP:             return "Send Total Drop";
        case LongPoll::SEND_TOTAL_JACKPOT:          return "Send Total Jackpot";
        case LongPoll::SEND_GAMES_PLAYED:           return "Send Games Played";
        case LongPoll::SEND_GAMES_WON:              return "Send Games Won";
        case LongPoll::SEND_GAMES_LOST:             return "Send Games Lost";
        case LongPoll::SEND_SELECTED_METERS:        return "Send Selected Meters";
        case LongPoll::SEND_CURRENT_HOPPER_LEVEL:   return "Send Current Hopper Level";
        case LongPoll::SEND_METER_CHANGE:           return "Send Meter Change";

        // Enable/Disable
        case LongPoll::ENABLE_GAME:                 return "Enable Game";
        case LongPoll::DISABLE_GAME:                return "Disable Game";
        case LongPoll::ENABLE_BILL_ACCEPTOR:        return "Enable Bill Acceptor";
        case LongPoll::DISABLE_BILL_ACCEPTOR:       return "Disable Bill Acceptor";

        // Legacy Bonus
        case LongPoll::LEGACY_BONUS_PAY:            return "Legacy Bonus Pay";
        case LongPoll::LEGACY_BONUS_WIN:            return "Legacy Bonus Win Amount";

        // AFT
        case LongPoll::AFT_REGISTER_LOCK:           return "AFT Register Lock";
        case LongPoll::AFT_REQUEST_LOCK:            return "AFT Request Lock";
        case LongPoll::AFT_TRANSFER_FUNDS:          return "AFT Transfer Funds";
        case LongPoll::AFT_REGISTER_UNLOCK:         return "AFT Register Unlock";
        case LongPoll::AFT_INTERROGATE_STATUS:      return "AFT Interrogate Status";

        // TITO
        case LongPoll::SEND_VALIDATION_INFO:        return "Send Validation Info";
        case LongPoll::SEND_ENHANCED_VALIDATION:    return "Send Enhanced Validation";
        case LongPoll::REDEEM_TICKET:               return "Redeem Ticket";
        case LongPoll::SEND_TICKET_INFO:            return "Send Ticket Info";
        case LongPoll::SEND_TICKET_VALIDATION_DATA: return "Send Ticket Validation Data";

        // Progressive
        case LongPoll::SEND_PROGRESSIVE_WIN:        return "Send Progressive Win";
        case LongPoll::SEND_PROGRESSIVE_LEVELS:     return "Send Progressive Levels";
        case LongPoll::SEND_MULTIPLE_PROGRESSIVE_LEVELS: return "Send Multiple Progressive Levels";

        // Real-Time Events
        case LongPoll::ENABLE_REAL_TIME_EVENTS:     return "Enable Real-Time Events";
        case LongPoll::SEND_REAL_TIME_EVENT:        return "Send Real-Time Event";

        // ROM/EEPROM
        case LongPoll::SEND_ROM_SIGNATURE:          return "Send ROM Signature";
        case LongPoll::SEND_EEPROM_DATA:            return "Send EEPROM Data";

        // Date/Time
        case LongPoll::SEND_DATE_TIME:              return "Send Date/Time";
        case LongPoll::SET_DATE_TIME:               return "Set Date/Time";

        // Credits
        case LongPoll::SEND_CASHABLE_AMOUNT:        return "Send Cashable Amount";
        case LongPoll::SEND_RESTRICTED_AMOUNT:      return "Send Restricted Amount";
        case LongPoll::SEND_NONRESTRICTED_AMOUNT:   return "Send Non-Restricted Amount";

        // Multi-Game
        case LongPoll::SEND_ENABLED_GAMES:          return "Send Enabled Games";
        case LongPoll::SELECT_GAME:                 return "Select Game";
        case LongPoll::SEND_GAME_DENOMINATION:      return "Send Game Denomination";

        // Validation
        case LongPoll::SEND_SYSTEM_VALIDATION:      return "Send System Validation";

        default:
            // Check if it's a meter command (0x10-0x1F range)
            if (command >= 0x10 && command <= 0x1F) {
                return "Send Meters";
            }
            return "Unknown Command";
    }
}

} // namespace sas

