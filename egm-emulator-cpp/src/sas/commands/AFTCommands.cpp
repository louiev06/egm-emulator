#include "megamic/sas/commands/AFTCommands.h"
#include "megamic/sas/BCD.h"
#include <cstring>

namespace megamic {
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

        // Response includes lock status
        response.data.push_back(currentLockStatus);

        // Asset number (4 bytes BCD) - use machine ID or default
        std::vector<uint8_t> assetNumber = BCD::encode(1, 4);
        response.data.insert(response.data.end(), assetNumber.begin(), assetNumber.end());

        // Registration code (optional, 1 byte) - 0x00 = successful
        response.data.push_back(0x00);
    } else {
        // Lock forbidden
        currentLockStatus = LOCK_FORBIDDEN;
        response.data.push_back(currentLockStatus);
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
    response.command = LongPoll::AFT_LOCK_STATUS;

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
        case BONUS_TO_GAMING_MACHINE:
            // Transfer funds to machine (credits in)
            success = executeTransferToMachine(machine, amount);
            if (success) {
                transferStatus = FULL_TRANSFER_SUCCESSFUL;
                machine->incrementMeter(SASConstants::METER_AFT_IN, amount);
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
    response.command = LongPoll::AFT_UNLOCK;

    // Extract lock code
    std::vector<uint8_t> lockCode(data.begin(), data.begin() + 2);

    // Verify and unlock
    if (aftRegistered && lockCode == currentLockCode) {
        aftRegistered = false;
        currentLockStatus = LOCK_AVAILABLE;
        currentTransferStatus = TRANSFER_PENDING;
        currentLockCode = std::vector<uint8_t>(2, 0);

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

    Message response;
    response.address = 1;
    response.command = LongPoll::AFT_INTERROGATE_STATUS;

    if (!aftRegistered) {
        return buildStatusResponse(1, LongPoll::AFT_INTERROGATE_STATUS,
                                  GAME_NOT_REGISTERED, 0, std::vector<uint8_t>(4, 0));
    }

    // Return current transfer status
    return buildStatusResponse(1, LongPoll::AFT_INTERROGATE_STATUS,
                              currentTransferStatus, lastTransferAmount, lastTransactionID);
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

} // namespace commands
} // namespace sas
} // namespace megamic
