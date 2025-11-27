#ifndef MEGAMIC_SAS_COMMANDS_ENABLECOMMANDS_H
#define MEGAMIC_SAS_COMMANDS_ENABLECOMMANDS_H

#include "megamic/sas/SASCommands.h"
#include "megamic/simulator/Machine.h"

namespace megamic {
namespace sas {
namespace commands {

/**
 * EnableCommands - Handler for SAS enable/disable commands
 *
 * Implements game control commands:
 * - 0x01: Disable Gaming Machine
 * - 0x02: Enable Gaming Machine
 * - 0x03: Enable Bill Acceptor
 * - 0x04: Disable Bill Acceptor
 */
class EnableCommands {
public:
    /**
     * Handle "Enable Gaming Machine" (0x02)
     * Allows game play to proceed
     * @param machine Machine instance
     * @return ACK response
     */
    static Message handleEnableGame(simulator::Machine* machine);

    /**
     * Handle "Disable Gaming Machine" (0x01)
     * Prevents new game play (current game completes)
     * @param machine Machine instance
     * @return ACK response
     */
    static Message handleDisableGame(simulator::Machine* machine);

    /**
     * Handle "Enable Bill Acceptor" (0x03)
     * Allows bill validator to accept bills
     * @param machine Machine instance
     * @return ACK response
     */
    static Message handleEnableBillAcceptor(simulator::Machine* machine);

    /**
     * Handle "Disable Bill Acceptor" (0x04)
     * Prevents bill validator from accepting bills
     * @param machine Machine instance
     * @return ACK response
     */
    static Message handleDisableBillAcceptor(simulator::Machine* machine);

private:
    /**
     * Build ACK response
     * @param address SAS address
     * @param command Original command code
     * @return ACK message
     */
    static Message buildAckResponse(uint8_t address, uint8_t command);
};

} // namespace commands
} // namespace sas
} // namespace megamic

#endif // MEGAMIC_SAS_COMMANDS_ENABLECOMMANDS_H
