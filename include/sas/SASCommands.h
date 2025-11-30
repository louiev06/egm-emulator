#ifndef SAS_SASCOMMANDS_H
#define SAS_SASCOMMANDS_H

#include <cstdint>
#include <vector>
#include <string>


namespace sas {

/**
 * SAS Command codes and structures
 *
 * SAS Protocol uses two types of polls:
 * 1. General Poll (0x80-0x9F): Quick status check, returns exceptions
 * 2. Long Poll (0x00-0x7F): Specific commands with data
 */

// ============================================================================
// GENERAL POLL COMMANDS (0x80-0x9F)
// ============================================================================

namespace GeneralPoll {
    constexpr uint8_t ADDRESS_MASK = 0x1F;  // Lower 5 bits = address (0-31)
    constexpr uint8_t COMMAND = 0x80;       // Base command (0x80 + address)

    // General poll for address N: 0x80 | N
    // Example: Address 1 = 0x81, Address 5 = 0x85
}

// ============================================================================
// LONG POLL COMMANDS (0x00-0x7F)
// ============================================================================

namespace LongPoll {

// --- Gaming Machine Configuration ---
constexpr uint8_t SEND_GAME_NUMBER = 0x00;          // Send current game number
constexpr uint8_t SEND_METERS = 0x10;               // Send selected meters (10-1F)
constexpr uint8_t SEND_TOTAL_COIN_IN = 0x11;        // Meter: Total coin in
constexpr uint8_t SEND_TOTAL_COIN_OUT = 0x12;       // Meter: Total coin out
constexpr uint8_t SEND_TOTAL_DROP = 0x13;           // Meter: Total drop
constexpr uint8_t SEND_TOTAL_JACKPOT = 0x14;        // Meter: Total jackpot
constexpr uint8_t SEND_GAMES_PLAYED = 0x15;         // Meter: Games played
constexpr uint8_t SEND_GAMES_WON = 0x16;            // Meter: Games won
constexpr uint8_t SEND_GAMES_LOST = 0x17;           // Meter: Games lost

constexpr uint8_t SEND_PHYSICAL_REEL_STOP = 0x18;   // Physical reel stop info
constexpr uint8_t SEND_SELECTED_METERS = 0x19;      // Send multiple meters
constexpr uint8_t SEND_CURRENT_HOPPER_LEVEL = 0x1A; // Hopper status
constexpr uint8_t SEND_GAME_CONFIG = 0x1F;          // Game configuration

// --- Game Enable/Disable ---
constexpr uint8_t ENABLE_GAME = 0x01;               // Enable gaming
constexpr uint8_t DISABLE_GAME = 0x02;              // Disable gaming
constexpr uint8_t ENABLE_BILL_ACCEPTOR = 0x03;      // Enable bill validator
constexpr uint8_t DISABLE_BILL_ACCEPTOR = 0x04;     // Disable bill validator

// --- Legacy Bonus ---
constexpr uint8_t LEGACY_BONUS_PAY = 0x2F;          // Legacy bonus award
constexpr uint8_t LEGACY_BONUS_WIN = 0x8A;          // Legacy bonus win amount

// --- AFT (Account Funds Transfer) ---
constexpr uint8_t AFT_REGISTER_LOCK = 0x70;         // Register gaming machine
constexpr uint8_t AFT_REQUEST_LOCK = 0x71;          // Request lock
constexpr uint8_t AFT_TRANSFER_FUNDS = 0x72;        // Initiate transfer
constexpr uint8_t AFT_REGISTER_UNLOCK = 0x73;       // Unregister
constexpr uint8_t AFT_INTERROGATE_STATUS = 0x74;    // Query AFT status

// --- TITO (Ticket In/Ticket Out) ---
constexpr uint8_t SEND_VALIDATION_INFO = 0x7B;      // Validation number
constexpr uint8_t SEND_ENHANCED_VALIDATION = 0x7C;  // Enhanced validation
constexpr uint8_t REDEEM_TICKET = 0x7D;             // Redeem ticket
constexpr uint8_t SEND_TICKET_INFO = 0x7E;          // Ticket information
constexpr uint8_t SEND_TICKET_VALIDATION_DATA = 0x7F; // Ticket validation

// --- Progressive Jackpots ---
// NOTE: These are placeholder values - actual progressive commands are in 0x80+ range
// The real 0x51-0x53 are game configuration commands, not progressive
constexpr uint8_t SEND_PROGRESSIVE_AMOUNT = 0x80;   // Current progressive amount (placeholder)
constexpr uint8_t SEND_PROGRESSIVE_WIN = 0x84;      // Progressive win amount (placeholder)
constexpr uint8_t SEND_PROGRESSIVE_LEVELS = 0x85;   // Progressive level info (placeholder)
constexpr uint8_t SEND_PROGRESSIVE_BROADCAST = 0x86; // Broadcast progressive values (placeholder)

// --- Real-Time Event Reporting ---
constexpr uint8_t ENABLE_REAL_TIME_EVENTS = 0x1D;   // Enable event reporting
constexpr uint8_t SEND_REAL_TIME_EVENT = 0x50;      // Send pending events

// --- ROM and EEPROM ---
constexpr uint8_t SEND_ROM_SIGNATURE = 0x0F;        // ROM signature/checksum
constexpr uint8_t SEND_EEPROM_DATA = 0x21;          // EEPROM data

// --- Date/Time ---
constexpr uint8_t SEND_DATE_TIME = 0x1B;            // Send current date/time

// --- Additional Meters ---
constexpr uint8_t SEND_TOTAL_BILLS = 0x20;          // Send total bills (bill drop in dollars)

// --- Machine Status ---
constexpr uint8_t SEND_MACHINE_ID = 0x2E;           // Machine ID and info
constexpr uint8_t SEND_MACHINE_ID_AND_SERIAL = 0x54; // Gaming Machine ID and Serial Number
constexpr uint8_t SEND_CASHABLE_AMOUNT = 0x6F;      // Current cashable credits
constexpr uint8_t SEND_RESTRICTED_AMOUNT = 0x6D;    // Restricted credits
constexpr uint8_t SEND_NONRESTRICTED_AMOUNT = 0x6E; // Non-restricted credits

// --- Multi-Game ---
constexpr uint8_t SEND_ENABLED_GAMES = 0x55;        // List enabled games
constexpr uint8_t SELECT_GAME = 0x56;               // Select specific game
constexpr uint8_t SEND_GAME_DENOMINATION = 0x5F;    // Game denomination

// --- Meter Change Notification ---
constexpr uint8_t SEND_METER_CHANGE = 0x31;         // Meter delta since last poll

// --- System Validation ---
constexpr uint8_t SEND_SYSTEM_VALIDATION = 0x4C;    // System validation number

} // namespace LongPoll

// ============================================================================
// EXCEPTION CODES
// ============================================================================

namespace Exception {
    constexpr uint8_t HANDPAY_PENDING = 0x00;           // Handpay is pending
    constexpr uint8_t PROGRESSIVE_WIN = 0x01;           // Progressive win
    constexpr uint8_t DOOR_OPEN = 0x10;                 // Main door open
    constexpr uint8_t CASHBOX_DOOR_OPEN = 0x11;         // Cashbox door open
    constexpr uint8_t BILL_ACCEPTOR_ERROR = 0x20;       // Bill validator error
    constexpr uint8_t HOPPER_EMPTY = 0x30;              // Hopper empty
    constexpr uint8_t HOPPER_JAM = 0x31;                // Hopper jammed
    constexpr uint8_t PRINTER_ERROR = 0x40;             // Printer malfunction
    constexpr uint8_t PRINTER_PAPER_OUT = 0x41;         // Printer out of paper
    constexpr uint8_t REEL_TILT = 0x50;                 // Reel malfunction
    constexpr uint8_t RAM_ERROR = 0x60;                 // RAM error
    constexpr uint8_t POWER_OFF_CARD_CAGE = 0x70;       // Power failure detected
    constexpr uint8_t GAME_RECALLED = 0x80;             // Game in recall mode
    constexpr uint8_t GAME_STARTED = 0x90;              // Game started
}

// ============================================================================
// COMMAND STRUCTURES
// ============================================================================

/**
 * SAS Message header (address + command)
 */
struct MessageHeader {
    uint8_t address;    // Machine address (0-31 for general poll, 1-127 for long poll)
    uint8_t command;    // Command code

    MessageHeader() : address(0), command(0) {}
    MessageHeader(uint8_t addr, uint8_t cmd) : address(addr), command(cmd) {}
};

/**
 * Complete SAS message with CRC
 */
struct Message {
    uint8_t address;                // Machine address
    uint8_t command;                // Command code
    std::vector<uint8_t> data;      // Command data (variable length)
    uint16_t crc;                   // CRC-16 (calculated/verified separately)

    Message() : address(0), command(0), crc(0) {}

    /**
     * Get total message length (address + command + data + CRC)
     */
    size_t length() const {
        return 1 + 1 + data.size() + 2;  // addr + cmd + data + 2-byte CRC
    }

    /**
     * Serialize message to byte array (including CRC)
     */
    std::vector<uint8_t> serialize() const;

    /**
     * Parse message from byte array
     */
    static Message parse(const uint8_t* buffer, size_t length);
};

// ============================================================================
// AFT TRANSFER TYPES
// ============================================================================

namespace AFT {
    // Transfer codes
    constexpr uint8_t TRANSFER_IN_HOUSE = 0x00;         // Casino to game
    constexpr uint8_t TRANSFER_BONUS_COINOUT = 0x01;    // Bonus award (cashable)
    constexpr uint8_t TRANSFER_BONUS_JACKPOT = 0x02;    // Bonus jackpot
    constexpr uint8_t TRANSFER_IN_HOUSE_RESTRICTED = 0x08; // Restricted funds
    constexpr uint8_t TRANSFER_DEBIT = 0x80;            // Game to casino
    constexpr uint8_t TRANSFER_WIN = 0x81;              // Win amount transfer

    // Transfer status codes
    constexpr uint8_t STATUS_FULL_TRANSFER = 0x00;      // Success
    constexpr uint8_t STATUS_PARTIAL_TRANSFER = 0x01;   // Partial success
    constexpr uint8_t STATUS_NO_TRANSFER = 0x80;        // Failed
    constexpr uint8_t STATUS_UNSUPPORTED = 0xFF;        // Not supported

    // Receipt status
    constexpr uint8_t RECEIPT_PRINTED = 0x00;
    constexpr uint8_t RECEIPT_NOT_PRINTED = 0x80;
}

// ============================================================================
// VALIDATION METHODS
// ============================================================================

namespace Validation {
    constexpr uint8_t SYSTEM = 0x00;        // System validation
    constexpr uint8_t SECURE_ENHANCED = 0x01; // Secure enhanced validation
}

// ============================================================================
// GAME STATUS FLAGS
// ============================================================================

namespace GameStatus {
    constexpr uint8_t ENABLED = 0x01;           // Game is enabled
    constexpr uint8_t DISABLED = 0x00;          // Game is disabled
    constexpr uint8_t IN_PLAY = 0x02;           // Game in progress
    constexpr uint8_t IDLE = 0x00;              // Game idle
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

/**
 * Get human-readable command name
 */
const char* getCommandName(uint8_t command);

/**
 * Check if command is a general poll
 * Note: 0xA0 and 0xAF are excluded as they're long polls with data
 */
inline bool isGeneralPoll(uint8_t command) {
    return (command >= 0x80 && command <= 0x9F && command != 0xA0 && command != 0xAF);
}

/**
 * Check if command is a long poll
 */
inline bool isLongPoll(uint8_t command) {
    return (command < 0x80);
}

/**
 * Extract address from general poll command
 */
inline uint8_t getGeneralPollAddress(uint8_t command) {
    return command & GeneralPoll::ADDRESS_MASK;
}

/**
 * Build general poll command for address
 */
inline uint8_t makeGeneralPoll(uint8_t address) {
    return GeneralPoll::COMMAND | (address & GeneralPoll::ADDRESS_MASK);
}

} // namespace sas


#endif // SAS_SASCOMMANDS_H
