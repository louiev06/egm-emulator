#include "sas/commands/TITOCommands.h"
#include "sas/BCD.h"
#include "sas/SASConstants.h"
#include <ctime>
#include <cstring>


namespace sas {
namespace commands {

// Static storage for last printed ticket (in a real system, this would be persistent)
static std::vector<uint8_t> lastValidationNumber(8, 0);
static uint64_t lastTicketAmount = 0;
static time_t lastTicketTime = 0;

Message TITOCommands::handleSendValidationInfo(simulator::Machine* machine) {
    if (!machine) {
        return Message();
    }

    Message response;
    response.address = 1;
    response.command = LongPoll::SEND_VALIDATION_INFO;

    // Return last printed ticket validation number
    // Format: 8 bytes validation number + 5 bytes BCD amount
    response.data = lastValidationNumber;

    // Add amount in BCD (5 bytes = 10 digits for up to $99,999,999.99)
    std::vector<uint8_t> amountBCD = BCD::encode(lastTicketAmount, 5);
    response.data.insert(response.data.end(), amountBCD.begin(), amountBCD.end());

    return response;
}

Message TITOCommands::handleSendEnhancedValidation(simulator::Machine* machine) {
    if (!machine) {
        return Message();
    }

    Message response;
    response.address = 1;
    response.command = LongPoll::SEND_ENHANCED_VALIDATION;

    // Enhanced validation includes additional security data
    // Format: 8 bytes validation + 5 bytes amount + additional fields
    response.data = lastValidationNumber;

    // Amount
    std::vector<uint8_t> amountBCD = BCD::encode(lastTicketAmount, 5);
    response.data.insert(response.data.end(), amountBCD.begin(), amountBCD.end());

    // Validation type (0x00 = system validation)
    response.data.push_back(Validation::SYSTEM);

    // Expiration date (7 days from print) - MMDDYYYY format
    time_t expiration = lastTicketTime + (7 * 24 * 60 * 60);  // 7 days
    struct tm* exp_tm = localtime(&expiration);
    if (exp_tm) {
        response.data.push_back(BCD::toBCD(exp_tm->tm_mon + 1));  // Month
        response.data.push_back(BCD::toBCD(exp_tm->tm_mday));     // Day
        std::vector<uint8_t> yearBCD = BCD::encode(exp_tm->tm_year + 1900, 2);
        response.data.insert(response.data.end(), yearBCD.begin(), yearBCD.end());
    }

    return response;
}

Message TITOCommands::handleRedeemTicket(simulator::Machine* machine,
                                        const std::vector<uint8_t>& data) {
    if (!machine || data.size() < 13) {
        // Need at least 8 bytes validation + 5 bytes amount
        return Message();
    }

    // Extract validation number (first 8 bytes)
    std::vector<uint8_t> validationNumber(data.begin(), data.begin() + 8);

    // Extract amount (next 5 bytes BCD)
    uint64_t amount = BCD::decode(data.data() + 8, 5);

    Message response;
    response.address = 1;
    response.command = LongPoll::REDEEM_TICKET;

    // Validate ticket
    bool valid = validateTicketRedemption(validationNumber);

    if (valid && amount > 0) {
        // Redeem ticket - add credits to machine
        machine->addCredits(static_cast<int64_t>(amount));

        // Response: Transfer status (0x00 = success)
        response.data.push_back(0x00);  // Full transfer

        // Echo validation number
        response.data.insert(response.data.end(), validationNumber.begin(), validationNumber.end());

        // Echo amount
        std::vector<uint8_t> amountBCD = BCD::encode(amount, 5);
        response.data.insert(response.data.end(), amountBCD.begin(), amountBCD.end());

        // Parsing code (0x00 = valid)
        response.data.push_back(0x00);
    } else {
        // Invalid ticket
        response.data.push_back(0x80);  // Transfer failed
        response.data.insert(response.data.end(), validationNumber.begin(), validationNumber.end());
        std::vector<uint8_t> zeroBCD = BCD::encode(0, 5);
        response.data.insert(response.data.end(), zeroBCD.begin(), zeroBCD.end());
        response.data.push_back(0xFF);  // Invalid validation number
    }

    return response;
}

Message TITOCommands::handleSendTicketInfo(simulator::Machine* machine) {
    if (!machine) {
        return Message();
    }

    Message response;
    response.address = 1;
    response.command = LongPoll::SEND_TICKET_INFO;

    // Ticket information format:
    // Byte 0: Number of tickets printed (1 byte BCD)
    // Byte 1-2: Total value of tickets (2 bytes BCD)

    // For simplicity, report last ticket only
    response.data.push_back(BCD::toBCD(lastTicketAmount > 0 ? 1 : 0));  // 1 ticket if any

    // Total value (in dollars, 2 bytes BCD)
    uint64_t dollars = lastTicketAmount / 100;  // Convert cents to dollars
    std::vector<uint8_t> dollarsBCD = BCD::encode(dollars, 2);
    response.data.insert(response.data.end(), dollarsBCD.begin(), dollarsBCD.end());

    return response;
}

Message TITOCommands::handleSendTicketValidationData(simulator::Machine* machine) {
    if (!machine) {
        return Message();
    }

    // Similar to enhanced validation but includes all ticket data
    return handleSendEnhancedValidation(machine);
}

std::vector<uint8_t> TITOCommands::generateValidationNumber() {
    std::vector<uint8_t> validation(8);

    // Generate validation number based on timestamp and random component
    time_t now = time(nullptr);

    // Use timestamp for first 4 bytes (seconds since epoch)
    validation[0] = (now >> 24) & 0xFF;
    validation[1] = (now >> 16) & 0xFF;
    validation[2] = (now >> 8) & 0xFF;
    validation[3] = now & 0xFF;

    // Use pseudo-random for last 4 bytes
    // In production, use cryptographic random
    for (int i = 4; i < 8; i++) {
        validation[i] = (rand() & 0xFF);
    }

    return validation;
}

std::vector<uint8_t> TITOCommands::printTicket(simulator::Machine* machine, uint64_t amount) {
    if (!machine) {
        return std::vector<uint8_t>(8, 0);
    }

    // Generate validation number
    lastValidationNumber = generateValidationNumber();
    lastTicketAmount = amount;
    lastTicketTime = time(nullptr);

    // In a real system:
    // 1. Send ticket data to printer
    // 2. Wait for print confirmation
    // 3. Store ticket in database
    // 4. Update meters

    // Update ticket out meter
    machine->incrementMeter(SASConstants::METER_TICKET_OUT, amount);

    // Deduct credits from machine
    machine->addCredits(-static_cast<int64_t>(amount));

    return lastValidationNumber;
}

bool TITOCommands::validateTicketRedemption(const std::vector<uint8_t>& validationNumber) {
    if (validationNumber.size() != 8) {
        return false;
    }

    // Check if validation number matches last printed ticket
    if (validationNumber != lastValidationNumber) {
        return false;
    }

    // Check if ticket is not expired (7 days)
    time_t now = time(nullptr);
    if (now - lastTicketTime > (7 * 24 * 60 * 60)) {
        return false;  // Expired
    }

    // Check if ticket has already been redeemed
    // In a real system, check database for redeemed tickets
    // For now, assume valid

    return true;
}

} // namespace commands
} // namespace sas

