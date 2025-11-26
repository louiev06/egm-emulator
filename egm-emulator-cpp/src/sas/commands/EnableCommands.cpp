#include "megamic/sas/commands/EnableCommands.h"

namespace megamic {
namespace sas {
namespace commands {

Message EnableCommands::handleEnableGame(simulator::Machine* machine) {
    if (!machine) {
        return Message();
    }

    // Enable the machine for game play
    machine->setEnabled(true);

    // Return ACK
    return buildAckResponse(1, LongPoll::ENABLE_GAME);
}

Message EnableCommands::handleDisableGame(simulator::Machine* machine) {
    if (!machine) {
        return Message();
    }

    // Disable the machine (no new games, current game completes)
    machine->setEnabled(false);

    // Return ACK
    return buildAckResponse(1, LongPoll::DISABLE_GAME);
}

Message EnableCommands::handleEnableBillAcceptor(simulator::Machine* machine) {
    if (!machine) {
        return Message();
    }

    // Enable bill acceptor
    machine->setBillAcceptorEnabled(true);

    // Return ACK
    return buildAckResponse(1, LongPoll::ENABLE_BILL_ACCEPTOR);
}

Message EnableCommands::handleDisableBillAcceptor(simulator::Machine* machine) {
    if (!machine) {
        return Message();
    }

    // Disable bill acceptor
    machine->setBillAcceptorEnabled(false);

    // Return ACK
    return buildAckResponse(1, LongPoll::DISABLE_BILL_ACCEPTOR);
}

Message EnableCommands::buildAckResponse(uint8_t address, uint8_t command) {
    Message response;
    response.address = address;
    response.command = command;
    // No data - just ACK (address + command + CRC)
    return response;
}

} // namespace commands
} // namespace sas
} // namespace megamic
