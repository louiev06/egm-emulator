#ifndef MEGAMIC_SAS_COMMANDS_EXCEPTIONCOMMANDS_H
#define MEGAMIC_SAS_COMMANDS_EXCEPTIONCOMMANDS_H

#include "megamic/sas/SASCommands.h"
#include "megamic/simulator/Machine.h"
#include "megamic/io/MachineCommPort.h"

namespace megamic {
namespace sas {
namespace commands {

/**
 * ExceptionCommands - Handler for SAS exception and event reporting
 *
 * Implements exception-related commands:
 * - General Poll (0x80-0x9F): Returns priority exceptions
 * - Exception queue management
 * - Real-time event reporting
 */
class ExceptionCommands {
public:
    /**
     * Handle general poll - returns highest priority exception
     * @param port Communication port with exception queue
     * @return Exception response or empty if no exceptions
     */
    static Message handleGeneralPoll(io::MachineCommPort* port);

    /**
     * Queue door opened exception
     * @param port Communication port
     */
    static void queueDoorOpened(io::MachineCommPort* port);

    /**
     * Queue door closed exception
     * @param port Communication port
     */
    static void queueDoorClosed(io::MachineCommPort* port);

    /**
     * Queue game started exception
     * @param port Communication port
     */
    static void queueGameStarted(io::MachineCommPort* port);

    /**
     * Queue game ended exception
     * @param port Communication port
     */
    static void queueGameEnded(io::MachineCommPort* port);

    /**
     * Queue handpay pending exception
     * @param port Communication port
     */
    static void queueHandpayPending(io::MachineCommPort* port);

    /**
     * Queue progressive win exception
     * @param port Communication port
     */
    static void queueProgressiveWin(io::MachineCommPort* port);

    /**
     * Queue bill accepted exception
     * @param port Communication port
     * @param billCode Bill denomination code
     */
    static void queueBillAccepted(io::MachineCommPort* port, uint8_t billCode);

    /**
     * Queue cashout (ticket out) exception
     * @param port Communication port
     */
    static void queueCashout(io::MachineCommPort* port);

    /**
     * Queue power on exception
     * @param port Communication port
     */
    static void queuePowerOn(io::MachineCommPort* port);

    /**
     * Queue RAM error exception
     * @param port Communication port
     */
    static void queueRAMError(io::MachineCommPort* port);

private:
    /**
     * Build exception response message
     * @param address SAS address
     * @param exceptionCode Exception code
     * @return Exception message
     */
    static Message buildExceptionResponse(uint8_t address, uint8_t exceptionCode);
};

} // namespace commands
} // namespace sas
} // namespace megamic

#endif // MEGAMIC_SAS_COMMANDS_EXCEPTIONCOMMANDS_H
