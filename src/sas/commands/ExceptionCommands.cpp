#include "sas/commands/ExceptionCommands.h"


namespace sas {
namespace commands {

Message ExceptionCommands::handleGeneralPoll(io::MachineCommPort* port) {
    if (!port || !port->hasExceptions()) {
        // No exceptions - return empty response (NAK)
        return Message();
    }

    // Get and remove the next exception from queue
    // This is already handled by MachineCommPort base class
    // The exception queue is managed in the port

    return Message();  // Port handles exception response in SASCommPort::handleGeneralPoll
}

void ExceptionCommands::queueDoorOpened(io::MachineCommPort* port) {
    if (port) {
        port->queueException(Exception::DOOR_OPEN);
    }
}

void ExceptionCommands::queueDoorClosed(io::MachineCommPort* port) {
    if (port) {
        // Door closed is typically not reported as exception in SAS
        // But we can queue it for completeness
        // Use a custom code or omit
    }
}

void ExceptionCommands::queueGameStarted(io::MachineCommPort* port) {
    if (port) {
        port->queueException(Exception::GAME_STARTED);
    }
}

void ExceptionCommands::queueGameEnded(io::MachineCommPort* port) {
    if (port) {
        // Game ended is typically implied by game started, not a separate exception
        // Can be omitted or use custom code
    }
}

void ExceptionCommands::queueHandpayPending(io::MachineCommPort* port) {
    if (port) {
        port->queueException(Exception::HANDPAY_PENDING);
    }
}

void ExceptionCommands::queueProgressiveWin(io::MachineCommPort* port) {
    if (port) {
        port->queueException(Exception::PROGRESSIVE_WIN);
    }
}

void ExceptionCommands::queueBillAccepted(io::MachineCommPort* port, uint8_t billCode) {
    if (port) {
        // Bill accepted exceptions are codes 0x28-0x2D for $1,$2,$5,$10,$20,$50,$100
        // Map bill code to exception
        // For simplicity, we'll use a base code + bill type
        port->queueException(0x28 + billCode);  // Simplified mapping
    }
}

void ExceptionCommands::queueCashout(io::MachineCommPort* port) {
    if (port) {
        // Cashout/ticket printed exception
        port->queueException(0x60);  // Ticket printed code
    }
}

void ExceptionCommands::queuePowerOn(io::MachineCommPort* port) {
    if (port) {
        port->queueException(0x19);  // AC power applied
    }
}

void ExceptionCommands::queueRAMError(io::MachineCommPort* port) {
    if (port) {
        port->queueException(Exception::RAM_ERROR);
    }
}

Message ExceptionCommands::buildExceptionResponse(uint8_t address, uint8_t exceptionCode) {
    Message response;
    response.address = address;
    response.command = exceptionCode;
    // No data for basic exceptions - just address + exception code + CRC
    return response;
}

} // namespace commands
} // namespace sas

