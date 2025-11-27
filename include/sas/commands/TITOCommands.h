#ifndef SAS_COMMANDS_TITOCOMMANDS_H
#define SAS_COMMANDS_TITOCOMMANDS_H

#include "sas/SASCommands.h"
#include "simulator/Machine.h"
#include <vector>
#include <cstdint>


namespace sas {
namespace commands {

/**
 * TITOCommands - Handler for TITO (Ticket In/Ticket Out) commands
 *
 * Implements ticket validation and redemption:
 * - 0x7B: Send Validation Information
 * - 0x7C: Send Enhanced Validation Information
 * - 0x7D: Redeem Ticket
 * - 0x7E: Send Ticket Information
 * - 0x7F: Send Ticket Validation Data
 */
class TITOCommands {
public:
    /**
     * Handle "Send Validation Information" (0x7B)
     * Returns validation number for printed ticket
     * @param machine Machine instance
     * @return Response with validation number
     */
    static Message handleSendValidationInfo(simulator::Machine* machine);

    /**
     * Handle "Send Enhanced Validation Information" (0x7C)
     * Returns enhanced validation data (secure validation)
     * @param machine Machine instance
     * @return Response with validation data
     */
    static Message handleSendEnhancedValidation(simulator::Machine* machine);

    /**
     * Handle "Redeem Ticket" (0x7D)
     * Validates and redeems a ticket, adding credits to machine
     * @param machine Machine instance
     * @param data Ticket data (validation number, amount, etc.)
     * @return Response with redemption result
     */
    static Message handleRedeemTicket(simulator::Machine* machine,
                                     const std::vector<uint8_t>& data);

    /**
     * Handle "Send Ticket Information" (0x7E)
     * Returns information about last printed ticket
     * @param machine Machine instance
     * @return Response with ticket details
     */
    static Message handleSendTicketInfo(simulator::Machine* machine);

    /**
     * Handle "Send Ticket Validation Data" (0x7F)
     * Returns complete validation data for ticket
     * @param machine Machine instance
     * @return Response with validation data
     */
    static Message handleSendTicketValidationData(simulator::Machine* machine);

    /**
     * Generate validation number for ticket
     * Uses system time and machine ID to create unique number
     * @return 8-byte validation number
     */
    static std::vector<uint8_t> generateValidationNumber();

    /**
     * Print ticket (simulate cashout)
     * Creates ticket with validation number and amount
     * @param machine Machine instance
     * @param amount Amount in cents
     * @return Validation number of printed ticket
     */
    static std::vector<uint8_t> printTicket(simulator::Machine* machine, uint64_t amount);

private:
    /**
     * Build validation info response
     * @param address SAS address
     * @param validationNumber Validation number (8 bytes)
     * @param amount Ticket amount in cents
     * @return Response message
     */
    static Message buildValidationResponse(uint8_t address,
                                          const std::vector<uint8_t>& validationNumber,
                                          uint64_t amount);

    /**
     * Validate ticket redemption
     * Checks if validation number is valid and not expired
     * @param validationNumber Validation number to check
     * @return true if valid
     */
    static bool validateTicketRedemption(const std::vector<uint8_t>& validationNumber);
};

} // namespace commands
} // namespace sas


#endif // SAS_COMMANDS_TITOCOMMANDS_H
