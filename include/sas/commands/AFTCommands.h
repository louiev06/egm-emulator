#ifndef SAS_COMMANDS_AFTCOMMANDS_H
#define SAS_COMMANDS_AFTCOMMANDS_H

#include "sas/SASCommands.h"
#include "simulator/Machine.h"
#include <vector>
#include <cstdint>


namespace sas {
namespace commands {

/**
 * AFTCommands - Handler for AFT (Account Funds Transfer) commands
 *
 * Implements electronic funds transfer between host and gaming machine:
 * - 0x70: AFT Register Gaming Machine Lock and Status Request
 * - 0x71: AFT Gaming Machine Lock and Status Request
 * - 0x72: AFT Transfer Funds
 * - 0x73: AFT Register Gaming Machine Lock and Status Request (unlock)
 * - 0x74: AFT Interrogate Current Transfer Status
 */
class AFTCommands {
public:
    /**
     * Handle "AFT Register Gaming Machine Lock and Status Request" (0x70)
     * Registers gaming machine with host for AFT capability
     * @param machine Machine instance
     * @param data Lock code and registration data
     * @return Response with lock status
     */
    static Message handleRegisterLock(simulator::Machine* machine,
                                      const std::vector<uint8_t>& data);

    /**
     * Handle "AFT Gaming Machine Lock and Status Request" (0x71)
     * Requests lock status and current AFT state
     * @param machine Machine instance
     * @param data Lock code
     * @return Response with current lock and transfer status
     */
    static Message handleLockStatus(simulator::Machine* machine,
                                    const std::vector<uint8_t>& data);

    /**
     * Handle "AFT Transfer Funds" (0x72)
     * Transfers funds to/from gaming machine
     * @param machine Machine instance
     * @param data Transfer data (amount, direction, transaction ID, etc.)
     * @return Response with transfer status
     */
    static Message handleTransferFunds(simulator::Machine* machine,
                                       const std::vector<uint8_t>& data);

    /**
     * Handle "AFT Register Gaming Machine Unlock" (0x73)
     * Unregisters gaming machine from AFT
     * @param machine Machine instance
     * @param data Unlock code
     * @return Response with unlock status
     */
    static Message handleUnlock(simulator::Machine* machine,
                                const std::vector<uint8_t>& data);

    /**
     * Handle "AFT Interrogate Current Transfer Status" (0x74)
     * Queries current transfer status without initiating new transfer
     * @param machine Machine instance
     * @return Response with current transfer status
     */
    static Message handleInterrogateStatus(simulator::Machine* machine);

    /**
     * Handle "Send AFT Registration Meters" (0x1D)
     * Returns AFT-related meters
     * Response: [Addr][0x1D][Promo Credit In(4)][Non-Cash Credit In(4)]
     *                      [Transferred Credits(4)][Cashable Credits(4)][CRC]
     * Total: 20 bytes
     * @param machine Machine instance
     * @return Response with AFT meters
     */
    static Message handleSendAFTRegistrationMeters(simulator::Machine* machine);

    /**
     * Handle "Send Non-Cashable Electronic Promotion Credits" (0x27)
     * Returns current NCEP credits on credit meter
     * Response: [Addr][0x27][NCEP Credits(4 BCD)][CRC]
     * Total: 8 bytes
     * @param machine Machine instance
     * @return Response with NCEP credits
     */
    static Message handleSendNonCashablePromoCredits(simulator::Machine* machine);

    // AFT Transfer Types
    enum TransferType {
        TRANSFER_TO_GAMING_MACHINE = 0x00,      // Host -> Game (credits in)
        TRANSFER_FROM_GAMING_MACHINE = 0x80,    // Game -> Host (cashout)
        TRANSFER_TO_PRINTER = 0x40,             // Print cashout ticket
        BONUS_TO_GAMING_MACHINE = 0x01,         // Bonus award
        DEBIT_TO_GAMING_MACHINE = 0x10          // Debit transfer
    };

    // AFT Transfer Status Codes
    enum TransferStatus {
        TRANSFER_PENDING = 0x00,
        FULL_TRANSFER_SUCCESSFUL = 0x01,
        PARTIAL_TRANSFER_SUCCESSFUL = 0x02,
        TRANSFER_CANCELLED_BY_HOST = 0x40,
        TRANSFER_CANCELLED_BY_GAME = 0x80,
        GAME_NOT_REGISTERED = 0x81,
        TRANSACTION_ID_NOT_UNIQUE = 0x82,
        NOT_VALID_FUNCTION = 0x83,
        NOT_VALID_AMOUNT = 0x84,
        TRANSFER_AMOUNT_EXCEEDS_LIMIT = 0x85,
        GAMING_MACHINE_UNABLE = 0xFF
    };

    // Lock Status Codes
    enum LockStatus {
        LOCK_AVAILABLE = 0x00,
        LOCK_PENDING = 0x01,
        LOCK_ESTABLISHED = 0x02,
        LOCK_FORBIDDEN = 0xFF
    };

private:
    /**
     * Validate lock code
     * @param lockCode Lock code from host (2 bytes)
     * @return true if valid
     */
    static bool validateLockCode(const std::vector<uint8_t>& lockCode);

    /**
     * Build AFT status response
     * @param address SAS address
     * @param command Command code
     * @param transferStatus Current transfer status
     * @param amount Transfer amount (in cents)
     * @param transactionID Transaction ID
     * @return Response message
     */
    static Message buildStatusResponse(uint8_t address,
                                       uint8_t command,
                                       uint8_t transferStatus,
                                       uint64_t amount,
                                       const std::vector<uint8_t>& transactionID);

    /**
     * Execute transfer to gaming machine (credits in)
     * @param machine Machine instance
     * @param amount Amount in cents
     * @return true if successful
     */
    static bool executeTransferToMachine(simulator::Machine* machine, uint64_t amount);

    /**
     * Execute transfer from gaming machine (cashout)
     * @param machine Machine instance
     * @param amount Amount in cents
     * @return true if successful
     */
    static bool executeTransferFromMachine(simulator::Machine* machine, uint64_t amount);
};

} // namespace commands
} // namespace sas


#endif // SAS_COMMANDS_AFTCOMMANDS_H
