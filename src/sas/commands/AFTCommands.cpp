#include "sas/commands/AFTCommands.h"
#include "sas/BCD.h"
#include "sas/SASConstants.h"
#include "utils/Logger.h"
#include "config/EGMConfig.h"
#include <cstring>


namespace sas {
namespace commands {

// Static storage for AFT state
static bool aftRegistered = false;
static std::vector<uint8_t> currentLockCode(2, 0);
static uint8_t currentLockStatus = AFTCommands::LOCK_AVAILABLE;
static uint8_t currentTransferStatus = AFTCommands::TRANSFER_PENDING;
static uint64_t lastTransferAmount = 0;
static std::vector<uint8_t> lastTransactionID(4, 0);
static uint8_t lastTransferType = 0;

// Additional AFT state for 0x74 response (loaded from egm-config.json)
static uint64_t assetNumber = 0;                    // Loaded from config: machineInfo.assetNumber
static uint8_t gameLockStatus = 0xFF;               // 0xFF = Not locked (runtime state)
static uint8_t availableTransfers = 0x00;           // Bitmask of available transfer types (runtime state)
static uint8_t hostCashoutStatus = 0;               // Loaded from config: aft.hostCashoutStatus
static uint8_t aftStatus = 0;                       // Loaded from config: aft.aftStatusFlags
static uint8_t maxBufferIndex = 0;                  // Loaded from config: aft.maxBufferIndex
static uint64_t currentRestrictedAmount = 0;        // Restricted promo credits (cents) - runtime state
static uint64_t currentNonRestrictedAmount = 0;     // Non-restricted promo credits (cents) - runtime state
static uint64_t gameTransferLimit = 0;              // Loaded from config: aft.transferLimit
static uint32_t restrictedExpiration = 0;           // Expiration timestamp (0 = no expiration) - runtime state
static uint16_t restrictedPoolID = 0;               // Loaded from config: aft.restrictedPoolID
static bool configLoaded = false;                   // Track if config has been loaded

// Helper function to load AFT configuration from JSON
static void loadAFTConfig() {
    if (configLoaded) {
        return;
    }

    assetNumber = config::EGMConfig::getInt("machineInfo.assetNumber", 1000000);
    hostCashoutStatus = static_cast<uint8_t>(config::EGMConfig::getInt("aft.hostCashoutStatus", 1));
    aftStatus = static_cast<uint8_t>(config::EGMConfig::getInt("aft.aftStatusFlags", 0xB1));
    maxBufferIndex = static_cast<uint8_t>(config::EGMConfig::getInt("aft.maxBufferIndex", 100));
    gameTransferLimit = config::EGMConfig::getInt("aft.transferLimit", 100000);
    restrictedPoolID = static_cast<uint16_t>(config::EGMConfig::getInt("aft.restrictedPoolID", 0));

    configLoaded = true;
    utils::Logger::log("[AFT] Configuration loaded from egm-config.json");
    utils::Logger::log("[AFT]   Asset Number: " + std::to_string(assetNumber));
    utils::Logger::log("[AFT]   Host Cashout Status: " + std::to_string(hostCashoutStatus));
    utils::Logger::log("[AFT]   AFT Status Flags: 0x" +
        [](uint8_t val) {
            char buf[3];
            snprintf(buf, sizeof(buf), "%02X", val);
            return std::string(buf);
        }(aftStatus));
    utils::Logger::log("[AFT]   Max Buffer Index: " + std::to_string(maxBufferIndex));
    utils::Logger::log("[AFT]   Transfer Limit: " + std::to_string(gameTransferLimit));
    utils::Logger::log("[AFT]   Restricted Pool ID: " + std::to_string(restrictedPoolID));
}

Message AFTCommands::handleRegisterLock(simulator::Machine* machine,
                                        const std::vector<uint8_t>& data) {
    if (!machine || data.size() < 2) {
        return Message();
    }

    Message response;
    response.address = 1;
    response.command = LongPoll::AFT_REGISTER_LOCK;

    // Extract lock code (first 2 bytes)
    std::vector<uint8_t> lockCode(data.begin(), data.begin() + 2);

    // Register and establish lock
    if (validateLockCode(lockCode)) {
        aftRegistered = true;
        currentLockCode = lockCode;
        currentLockStatus = LOCK_ESTABLISHED;

        // Update 0x74 state: game is now locked
        gameLockStatus = 0x01;  // Locked with code (non-0xFF means locked)
        availableTransfers = 0x33;  // In-house, Bonus, Debit available (bits 0,1,4,5)

        // Response includes lock status
        response.data.push_back(currentLockStatus);

        // Asset number (4 bytes BCD) - use dynamic asset number
        std::vector<uint8_t> assetNumberBCD = BCD::encode(assetNumber, 4);
        response.data.insert(response.data.end(), assetNumberBCD.begin(), assetNumberBCD.end());

        // Registration code (optional, 1 byte) - 0x00 = successful
        response.data.push_back(0x00);

        utils::Logger::log("[0x70] AFT Registration successful - Game locked");
    } else {
        // Lock forbidden
        currentLockStatus = LOCK_FORBIDDEN;
        gameLockStatus = 0xFF;  // Not locked
        availableTransfers = 0x00;  // No transfers available
        response.data.push_back(currentLockStatus);

        utils::Logger::log("[0x70] AFT Registration failed - Lock forbidden");
    }

    return response;
}

Message AFTCommands::handleLockStatus(simulator::Machine* machine,
                                      const std::vector<uint8_t>& data) {
    if (!machine || data.size() < 2) {
        return Message();
    }

    Message response;
    response.address = 1;
    response.command = LongPoll::AFT_INTERROGATE_STATUS;

    // Extract lock code
    std::vector<uint8_t> lockCode(data.begin(), data.begin() + 2);

    // Verify lock code matches
    if (aftRegistered && lockCode == currentLockCode) {
        // Return current status
        response.data.push_back(currentLockStatus);
        response.data.push_back(currentTransferStatus);

        // Asset number
        std::vector<uint8_t> assetNumber = BCD::encode(1, 4);
        response.data.insert(response.data.end(), assetNumber.begin(), assetNumber.end());

        // Current cashable amount (5 bytes BCD)
        uint64_t credits = machine->getCredits();
        std::vector<uint8_t> creditsBCD = BCD::encode(credits, 5);
        response.data.insert(response.data.end(), creditsBCD.begin(), creditsBCD.end());
    } else {
        // Invalid lock code
        response.data.push_back(LOCK_FORBIDDEN);
        response.data.push_back(GAME_NOT_REGISTERED);
    }

    return response;
}

Message AFTCommands::handleTransferFunds(simulator::Machine* machine,
                                         const std::vector<uint8_t>& data) {
    if (!machine || data.size() < 15) {
        // Need: 1 byte transfer code + 5 bytes amount + 4 bytes transaction ID + others
        return Message();
    }

    Message response;
    response.address = 1;
    response.command = LongPoll::AFT_TRANSFER_FUNDS;

    // Check if registered
    if (!aftRegistered) {
        return buildStatusResponse(1, LongPoll::AFT_TRANSFER_FUNDS,
                                  GAME_NOT_REGISTERED, 0, std::vector<uint8_t>(4, 0));
    }

    // Extract transfer data
    uint8_t transferCode = data[0];
    uint8_t transferType = transferCode & 0xF0;  // Upper nibble

    // Extract amount (5 bytes BCD)
    uint64_t amount = BCD::decode(data.data() + 1, 5);

    // Extract transaction ID (4 bytes)
    std::vector<uint8_t> transactionID(data.begin() + 6, data.begin() + 10);

    // Validate amount
    if (amount == 0) {
        return buildStatusResponse(1, LongPoll::AFT_TRANSFER_FUNDS,
                                  NOT_VALID_AMOUNT, 0, transactionID);
    }

    // Check for duplicate transaction ID
    if (transactionID == lastTransactionID && amount == lastTransferAmount) {
        return buildStatusResponse(1, LongPoll::AFT_TRANSFER_FUNDS,
                                  TRANSACTION_ID_NOT_UNIQUE, 0, transactionID);
    }

    bool success = false;
    uint8_t transferStatus = TRANSFER_PENDING;

    // Execute transfer based on type
    switch (transferType) {
        case TRANSFER_TO_GAMING_MACHINE:
            // Transfer cashable funds to machine (credits in)
            success = executeTransferToMachine(machine, amount);
            if (success) {
                transferStatus = FULL_TRANSFER_SUCCESSFUL;
                machine->incrementMeter(SASConstants::METER_AFT_IN, amount);
                // Cashable amount is reflected in machine credits (already done)
                utils::Logger::log("[0x72] AFT Transfer IN: $" + std::to_string(amount / 100.0));
            } else {
                transferStatus = GAMING_MACHINE_UNABLE;
            }
            break;

        case BONUS_TO_GAMING_MACHINE:
            // Bonus transfer - adds to non-restricted promo credits
            success = executeTransferToMachine(machine, amount);
            if (success) {
                transferStatus = FULL_TRANSFER_SUCCESSFUL;
                machine->incrementMeter(SASConstants::METER_AFT_IN, amount);
                currentNonRestrictedAmount += amount;  // Track as non-restricted promo
                utils::Logger::log("[0x72] AFT Bonus Transfer: $" + std::to_string(amount / 100.0) +
                                  " (Non-Restricted: $" + std::to_string(currentNonRestrictedAmount / 100.0) + ")");
            } else {
                transferStatus = GAMING_MACHINE_UNABLE;
            }
            break;

        case TRANSFER_FROM_GAMING_MACHINE:
            // Transfer funds from machine (cashout)
            success = executeTransferFromMachine(machine, amount);
            if (success) {
                transferStatus = FULL_TRANSFER_SUCCESSFUL;
                machine->incrementMeter(SASConstants::METER_AFT_OUT, amount);
                // Deduct from non-restricted first, then cashable
                if (currentNonRestrictedAmount > 0) {
                    uint64_t deductFromPromo = std::min(currentNonRestrictedAmount, amount);
                    currentNonRestrictedAmount -= deductFromPromo;
                }
                utils::Logger::log("[0x72] AFT Transfer OUT: $" + std::to_string(amount / 100.0));
            } else {
                transferStatus = GAMING_MACHINE_UNABLE;
            }
            break;

        case TRANSFER_TO_PRINTER:
            // Print ticket for amount
            // Similar to cashout but prints instead of electronic transfer
            if (machine->getCredits() >= static_cast<int64_t>(amount)) {
                // Would call TITOCommands::printTicket here
                transferStatus = FULL_TRANSFER_SUCCESSFUL;
                utils::Logger::log("[0x72] AFT Print Ticket: $" + std::to_string(amount / 100.0));
            } else {
                transferStatus = GAMING_MACHINE_UNABLE;
            }
            break;

        default:
            transferStatus = NOT_VALID_FUNCTION;
            break;
    }

    // Store transaction info
    lastTransactionID = transactionID;
    lastTransferAmount = amount;
    lastTransferType = transferType;
    currentTransferStatus = transferStatus;

    return buildStatusResponse(1, LongPoll::AFT_TRANSFER_FUNDS,
                              transferStatus, amount, transactionID);
}

Message AFTCommands::handleUnlock(simulator::Machine* machine,
                                  const std::vector<uint8_t>& data) {
    if (!machine || data.size() < 2) {
        return Message();
    }

    Message response;
    response.address = 1;
    response.command = LongPoll::AFT_REGISTER_UNLOCK;

    // Extract lock code
    std::vector<uint8_t> lockCode(data.begin(), data.begin() + 2);

    // Verify and unlock
    if (aftRegistered && lockCode == currentLockCode) {
        aftRegistered = false;
        currentLockStatus = LOCK_AVAILABLE;
        currentTransferStatus = TRANSFER_PENDING;
        currentLockCode = std::vector<uint8_t>(2, 0);

        // Update 0x74 state: game is now unlocked
        gameLockStatus = 0xFF;  // Not locked
        availableTransfers = 0x00;  // No transfers available when unlocked

        utils::Logger::log("[0x73] AFT Unlock successful - Game unlocked");

        // Response: unlock successful
        response.data.push_back(LOCK_AVAILABLE);
    } else {
        // Invalid lock code
        response.data.push_back(LOCK_FORBIDDEN);
    }

    return response;
}

Message AFTCommands::handleInterrogateStatus(simulator::Machine* machine) {
    if (!machine) {
        return Message();
    }

    // Load AFT configuration from JSON on first call
    loadAFTConfig();

    // 0x74: AFT Gaming Machine Lock and Status Request
    // Based on real EGM response format
    // Response: [Addr][0x74][Length][AssetNumber(4)][GameLockStatus][AvailableTransfers]
    //          [HostCashoutStatus][AFTStatus][MaxBufferIndex][CurrentCashable(5 BCD)]
    //          [CurrentRestricted(5 BCD)][CurrentNonRestricted(5 BCD)][GameTransferLimit(5 BCD)]
    //          [RestrictedExpiration(4)][RestrictedPoolID(2)][CRC]

    Message response;
    response.address = 1;
    response.command = LongPoll::AFT_INTERROGATE_STATUS;

    // Length byte (35 data bytes following, NOT including CRC)
    response.data.push_back(35);

    // Asset Number (4 bytes BCD) - use dynamic state
    std::vector<uint8_t> assetNumberBCD = BCD::encode(assetNumber, 4);
    response.data.insert(response.data.end(), assetNumberBCD.begin(), assetNumberBCD.end());

    // Game Lock Status (1 byte) - use dynamic state
    // 0xFF = Not locked, 0x00 = Game locked by other host, 0x01-0xFE = Locked with code
    response.data.push_back(gameLockStatus);

    // Available Transfers (1 byte) - use dynamic state
    // Bitmask: Bit 0=In-house, Bit 1=Bonus, Bit 2=Debit, etc.
    response.data.push_back(availableTransfers);

    // Host Cashout Status (1 byte) - use dynamic state
    // 0x00 = Not controllable, 0x01 = Controllable by host
    response.data.push_back(hostCashoutStatus);

    // AFT Status (1 byte) - use dynamic state
    // Bit 0: Printer available (1)
    // Bit 1-2: Reserved (0)
    // Bit 3: Reserved (0)
    // Bit 4: In-house transfers enabled (1)
    // Bit 5: Bonus transfers enabled (1)
    // Bit 6: Reserved (0)
    // Bit 7: Any AFT enabled (1)
    response.data.push_back(aftStatus);

    // Max Buffer Index (1 byte) - use dynamic state
    response.data.push_back(maxBufferIndex);

    // Current Cashable Amount (5 bytes BCD) - use current credits from machine
    uint64_t credits = machine->getCredits();
    std::vector<uint8_t> cashableBCD = BCD::encode(credits, 5);
    response.data.insert(response.data.end(), cashableBCD.begin(), cashableBCD.end());

    // Current Restricted Amount (5 bytes BCD) - use dynamic state
    std::vector<uint8_t> restrictedBCD = BCD::encode(currentRestrictedAmount, 5);
    response.data.insert(response.data.end(), restrictedBCD.begin(), restrictedBCD.end());

    // Current Non-Restricted Amount (5 bytes BCD) - use dynamic state
    std::vector<uint8_t> nonRestrictedBCD = BCD::encode(currentNonRestrictedAmount, 5);
    response.data.insert(response.data.end(), nonRestrictedBCD.begin(), nonRestrictedBCD.end());

    // Game Transfer Limit (5 bytes BCD) - use dynamic state
    std::vector<uint8_t> transferLimitBCD = BCD::encode(gameTransferLimit, 5);
    response.data.insert(response.data.end(), transferLimitBCD.begin(), transferLimitBCD.end());

    // Restricted Expiration (4 bytes) - use dynamic state
    // Format: MMDDYYYY in BCD, or 0x00000000 = no expiration
    response.data.push_back((restrictedExpiration >> 24) & 0xFF);
    response.data.push_back((restrictedExpiration >> 16) & 0xFF);
    response.data.push_back((restrictedExpiration >> 8) & 0xFF);
    response.data.push_back(restrictedExpiration & 0xFF);

    // Restricted Pool ID (2 bytes) - use dynamic state
    response.data.push_back((restrictedPoolID >> 8) & 0xFF);
    response.data.push_back(restrictedPoolID & 0xFF);

    utils::Logger::log("[0x74] AFT Lock and Status Response:");
    utils::Logger::log("  Asset Number: " + std::to_string(assetNumber));
    utils::Logger::log("  Game Lock Status: 0x" +
        [](uint8_t val) {
            char buf[3];
            snprintf(buf, sizeof(buf), "%02X", val);
            return std::string(buf);
        }(gameLockStatus) + (gameLockStatus == 0xFF ? " (Not locked)" : " (Locked)"));
    utils::Logger::log("  Available Transfers: 0x" +
        [](uint8_t val) {
            char buf[3];
            snprintf(buf, sizeof(buf), "%02X", val);
            return std::string(buf);
        }(availableTransfers));
    utils::Logger::log("  Host Cashout Status: 0x" +
        [](uint8_t val) {
            char buf[3];
            snprintf(buf, sizeof(buf), "%02X", val);
            return std::string(buf);
        }(hostCashoutStatus) + (hostCashoutStatus == 0x01 ? " (Controllable)" : " (Not controllable)"));
    utils::Logger::log("  AFT Status: 0xB1 (Printer, InHouse, Bonus, Any enabled)");
    utils::Logger::log("  Max Buffer Index: " + std::to_string(maxBufferIndex));
    utils::Logger::log("  Current Cashable: " + std::to_string(credits));
    utils::Logger::log("  Current Restricted: " + std::to_string(currentRestrictedAmount));
    utils::Logger::log("  Current Non-Restricted: " + std::to_string(currentNonRestrictedAmount));
    utils::Logger::log("  Transfer Limit: " + std::to_string(gameTransferLimit));

    return response;
}

bool AFTCommands::validateLockCode(const std::vector<uint8_t>& lockCode) {
    if (lockCode.size() != 2) {
        return false;
    }

    // Lock code 0x0000 is not valid
    if (lockCode[0] == 0 && lockCode[1] == 0) {
        return false;
    }

    // In production, would validate against authorized lock codes
    // For emulator, accept any non-zero code
    return true;
}

Message AFTCommands::buildStatusResponse(uint8_t address,
                                         uint8_t command,
                                         uint8_t transferStatus,
                                         uint64_t amount,
                                         const std::vector<uint8_t>& transactionID) {
    Message response;
    response.address = address;
    response.command = command;

    // Transfer status (1 byte)
    response.data.push_back(transferStatus);

    // Transfer amount (5 bytes BCD)
    std::vector<uint8_t> amountBCD = BCD::encode(amount, 5);
    response.data.insert(response.data.end(), amountBCD.begin(), amountBCD.end());

    // Transaction ID (4 bytes)
    response.data.insert(response.data.end(), transactionID.begin(), transactionID.end());

    // Transfer type (1 byte)
    response.data.push_back(lastTransferType);

    // Cashable amount on machine (5 bytes BCD)
    // This would be current credits, but we don't have machine reference here
    // Use last known amount
    std::vector<uint8_t> cashableBCD = BCD::encode(0, 5);
    response.data.insert(response.data.end(), cashableBCD.begin(), cashableBCD.end());

    // Restricted amount (5 bytes BCD) - typically 0 for non-restricted transfers
    std::vector<uint8_t> restrictedBCD = BCD::encode(0, 5);
    response.data.insert(response.data.end(), restrictedBCD.begin(), restrictedBCD.end());

    // Non-restricted amount (5 bytes BCD) - same as cashable
    response.data.insert(response.data.end(), cashableBCD.begin(), cashableBCD.end());

    return response;
}

bool AFTCommands::executeTransferToMachine(simulator::Machine* machine, uint64_t amount) {
    if (!machine) {
        return false;
    }

    // Add credits to machine
    machine->addCredits(static_cast<int64_t>(amount));
    return true;
}

bool AFTCommands::executeTransferFromMachine(simulator::Machine* machine, uint64_t amount) {
    if (!machine) {
        return false;
    }

    // Check if machine has sufficient credits
    if (machine->getCredits() < static_cast<int64_t>(amount)) {
        return false;  // Insufficient funds
    }

    // Deduct credits from machine
    machine->addCredits(-static_cast<int64_t>(amount));
    return true;
}

Message AFTCommands::handleSendAFTRegistrationMeters(simulator::Machine* machine) {
    if (!machine) {
        return Message();
    }

    // 0x1D: Send AFT Registration Meters
    // Response format (20 bytes total):
    // [Address][0x1D][Promo Credit In (4)][Non-Cash Credit In (4)]
    //                [Transferred Credits (4)][Cashable Credits (4)][CRC(2)]

    Message response;
    response.address = 1;
    response.command = 0x1D;

    // Promo Credit In (AFT Restricted To Game)
    uint64_t promoCredIn = machine->getMeter(SASConstants::METER_AFT_REST_IN);
    std::vector<uint8_t> promoCredInBCD = BCD::encode(promoCredIn, 4);
    response.data.insert(response.data.end(), promoCredInBCD.begin(), promoCredInBCD.end());

    // Non-Cash Credit In (AFT NonRestricted To Game)
    uint64_t nonCashCredIn = machine->getMeter(SASConstants::METER_AFT_NONREST_IN);
    std::vector<uint8_t> nonCashCredInBCD = BCD::encode(nonCashCredIn, 4);
    response.data.insert(response.data.end(), nonCashCredInBCD.begin(), nonCashCredInBCD.end());

    // Transferred Credits (AFT Cashable To Host)
    uint64_t transferredCred = machine->getMeter(SASConstants::METER_AFT_CASHABLE_OUT);
    std::vector<uint8_t> transferredCredBCD = BCD::encode(transferredCred, 4);
    response.data.insert(response.data.end(), transferredCredBCD.begin(), transferredCredBCD.end());

    // Cashable Credits (AFT Cashable To Game)
    uint64_t cashableCred = machine->getMeter(SASConstants::METER_AFT_CASHABLE_IN);
    std::vector<uint8_t> cashableCredBCD = BCD::encode(cashableCred, 4);
    response.data.insert(response.data.end(), cashableCredBCD.begin(), cashableCredBCD.end());

    utils::Logger::log("[0x1D] AFT Registration Meters response built");
    return response;
}

Message AFTCommands::handleSendNonCashablePromoCredits(simulator::Machine* machine) {
    if (!machine) {
        return Message();
    }

    // 0x27: Send Non-Cashable Electronic Promotion Credits
    // Response format (8 bytes total):
    // [Address][0x27][NCEP Credits (4 BCD)][CRC(2)]

    Message response;
    response.address = 1;
    response.command = 0x27;

    uint64_t ncepCredits = machine->getMeter(SASConstants::METER_NCEP_CREDITS);
    std::vector<uint8_t> ncepBCD = BCD::encode(ncepCredits, 4);
    response.data = ncepBCD;

    return response;
}

} // namespace commands
} // namespace sas

