#include "sas/SASCommPort.h"
#include "sas/CRC16.h"
#include "sas/commands/MeterCommands.h"
#include "sas/commands/EnableCommands.h"
#include "sas/commands/ExceptionCommands.h"
#include "sas/commands/DateTimeCommands.h"
#include "simulator/Machine.h"
#include "sas/commands/TITOCommands.h"
#include "sas/commands/AFTCommands.h"
#include "sas/commands/ProgressiveCommands.h"
#include <cstring>


namespace sas {

SASCommPort::SASCommPort(simulator::Machine* machine,
                         std::shared_ptr<io::CommChannel> channel,
                         uint8_t address)
    : io::MachineCommPort(machine, channel),
      address_(address),
      running_(false) {

    if (address_ < 1 || address_ > 127) {
        address_ = 1;  // Default to address 1
    }
}

SASCommPort::~SASCommPort() {
    stop();
}

bool SASCommPort::start() {
    if (running_) {
        return true;  // Already running
    }

    if (!channel_) {
        return false;  // No channel
    }

    // Open channel if not already open
    if (!channel_->isOpen()) {
        if (!channel_->open()) {
            return false;
        }
    }

    // Start receive thread
    running_ = true;
    receiveThread_ = std::thread(&SASCommPort::receiveThread, this);

    return true;
}

void SASCommPort::stop() {
    if (!running_) {
        return;
    }

    // Signal thread to stop
    running_ = false;

    // Wait for thread to finish
    if (receiveThread_.joinable()) {
        receiveThread_.join();
    }

    // Close channel
    if (channel_ && channel_->isOpen()) {
        channel_->close();
    }
}

bool SASCommPort::isRunning() const {
    return running_;
}

std::string SASCommPort::getName() const {
    return "SAS Port (Address " + std::to_string(address_) + ")";
}

void SASCommPort::setAddress(uint8_t address) {
    if (address >= 1 && address <= 127) {
        address_ = address;
    }
}

bool SASCommPort::sendMessage(const Message& msg) {
    if (!channel_ || !channel_->isOpen()) {
        return false;
    }

    // Serialize message (includes CRC calculation)
    std::vector<uint8_t> buffer = msg.serialize();

    // Send to channel
    bool success = sendRaw(buffer.data(), buffer.size());

    if (success) {
        std::lock_guard<std::recursive_mutex> lock(statsMutex_);
        stats_.messagesSent++;
    }

    return success;
}

SASCommPort::Statistics SASCommPort::getStatistics() const {
    std::lock_guard<std::recursive_mutex> lock(statsMutex_);
    return stats_;
}

void SASCommPort::resetStatistics() {
    std::lock_guard<std::recursive_mutex> lock(statsMutex_);
    stats_ = Statistics();
}

void SASCommPort::receiveThread() {
    while (running_) {
        // Read a message from the channel
        Message msg = readMessage(std::chrono::milliseconds(READ_TIMEOUT_MS));

        if (msg.command == 0) {
            // No message or timeout
            continue;
        }

        // Check if message is for this address
        if (msg.address != address_ && msg.address != 0) {
            // Not for us, ignore
            continue;
        }

        // Update statistics
        {
            std::lock_guard<std::recursive_mutex> lock(statsMutex_);
            stats_.messagesReceived++;

            if (isGeneralPoll(msg.command)) {
                stats_.generalPolls++;
            } else {
                stats_.longPolls++;
            }
        }

        // Process message and get response
        Message response = processMessage(msg);

        // Send response if there is one
        if (response.command != 0) {
            sendMessage(response);
        }
    }
}

Message SASCommPort::processMessage(const Message& msg) {
    if (isGeneralPoll(msg.command)) {
        return handleGeneralPoll(msg);
    } else {
        return handleLongPoll(msg);
    }
}

Message SASCommPort::handleGeneralPoll(const Message& msg) {
    Message response;

    // Check if we have pending exceptions
    if (!hasExceptions()) {
        // No exceptions, no response (NULL response)
        return response;
    }

    // Get next exception from queue
    uint8_t exceptionCode = 0;
    {
        std::lock_guard<std::recursive_mutex> lock(exceptionMutex_);
        if (!exceptionQueue_.empty()) {
            exceptionCode = exceptionQueue_.front().code;
            exceptionQueue_.pop();
        }
    }

    // Build exception response
    response.address = address_;
    response.command = exceptionCode;
    // No additional data for simple exceptions

    return response;
}

Message SASCommPort::handleLongPoll(const Message& msg) {
    Message response;
    response.address = address_;

    // Route command to appropriate handler
    switch (msg.command) {
        // Enable/Disable commands
        case LongPoll::ENABLE_GAME:
            response = commands::EnableCommands::handleEnableGame(machine_);
            break;

        case LongPoll::DISABLE_GAME:
            response = commands::EnableCommands::handleDisableGame(machine_);
            break;

        case LongPoll::ENABLE_BILL_ACCEPTOR:
            response = commands::EnableCommands::handleEnableBillAcceptor(machine_);
            break;

        case LongPoll::DISABLE_BILL_ACCEPTOR:
            response = commands::EnableCommands::handleDisableBillAcceptor(machine_);
            break;

        // Meter commands (0x10-0x1F)
        case LongPoll::SEND_TOTAL_COIN_IN:
        case LongPoll::SEND_TOTAL_COIN_OUT:
        case LongPoll::SEND_TOTAL_DROP:
        case LongPoll::SEND_TOTAL_JACKPOT:
        case LongPoll::SEND_GAMES_PLAYED:
        case LongPoll::SEND_GAMES_WON:
        case LongPoll::SEND_GAMES_LOST:
        case LongPoll::SEND_GAME_CONFIG:
            response = commands::MeterCommands::handleSendMeters(machine_, msg.command);
            break;

        // Selected meters
        case LongPoll::SEND_SELECTED_METERS:
            response = commands::MeterCommands::handleSendSelectedMeters(machine_, msg.data);
            break;

        // Game number query
        case LongPoll::SEND_GAME_NUMBER:
            response.command = msg.command;
            // Get first enabled game number, or 1 if no games
            {
                auto games = machine_->getGames();
                int gameNum = games.empty() ? 1 : games[0]->getGameNumber();
                response.data.push_back(static_cast<uint8_t>(gameNum));
            }
            break;

        // Date/Time commands
        case LongPoll::SEND_DATE_TIME:
            response = commands::DateTimeCommands::handleSendDateTime(machine_);
            break;

        case LongPoll::SET_DATE_TIME:
            response = commands::DateTimeCommands::handleSetDateTime(machine_, msg.data);
            break;

        // TITO (Ticket In/Ticket Out) commands
        case LongPoll::SEND_VALIDATION_INFO:
            response = commands::TITOCommands::handleSendValidationInfo(machine_);
            break;

        case LongPoll::SEND_ENHANCED_VALIDATION:
            response = commands::TITOCommands::handleSendEnhancedValidation(machine_);
            break;

        case LongPoll::REDEEM_TICKET:
            response = commands::TITOCommands::handleRedeemTicket(machine_, msg.data);
            break;

        case LongPoll::SEND_TICKET_INFO:
            response = commands::TITOCommands::handleSendTicketInfo(machine_);
            break;

        case LongPoll::SEND_TICKET_VALIDATION_DATA:
            response = commands::TITOCommands::handleSendTicketValidationData(machine_);
            break;

        // AFT (Account Funds Transfer) commands
        case LongPoll::AFT_REGISTER_LOCK:
            response = commands::AFTCommands::handleRegisterLock(machine_, msg.data);
            break;

        case LongPoll::AFT_REQUEST_LOCK:
            response = commands::AFTCommands::handleLockStatus(machine_, msg.data);
            break;

        case LongPoll::AFT_TRANSFER_FUNDS:
            response = commands::AFTCommands::handleTransferFunds(machine_, msg.data);
            break;

        case LongPoll::AFT_REGISTER_UNLOCK:
            response = commands::AFTCommands::handleUnlock(machine_, msg.data);
            break;

        case LongPoll::AFT_INTERROGATE_STATUS:
            response = commands::AFTCommands::handleInterrogateStatus(machine_);
            break;

        // Progressive Jackpot commands
        case LongPoll::SEND_PROGRESSIVE_AMOUNT:
            response = commands::ProgressiveCommands::handleSendProgressiveAmount(machine_, msg.data);
            break;

        case LongPoll::SEND_PROGRESSIVE_WIN:
            response = commands::ProgressiveCommands::handleSendProgressiveWin(machine_, msg.data);
            break;

        case LongPoll::SEND_PROGRESSIVE_LEVELS:
            response = commands::ProgressiveCommands::handleSendProgressiveLevels(machine_);
            break;

        case LongPoll::SEND_PROGRESSIVE_BROADCAST:
            response = commands::ProgressiveCommands::handleSendProgressiveBroadcast(machine_);
            break;

        default:
            // Unsupported command - no response (NAK)
            break;
    }

    return response;
}

Message SASCommPort::readMessage(std::chrono::milliseconds timeout) {
    Message msg;

    if (!channel_ || !channel_->isOpen()) {
        return msg;
    }

    uint8_t buffer[MAX_MESSAGE_SIZE];

    // Read first byte (address)
    int bytesRead = channel_->read(buffer, 1, timeout);
    if (bytesRead != 1) {
        return msg;  // Timeout or error
    }

    msg.address = buffer[0];

    // Read second byte (command)
    bytesRead = channel_->read(buffer + 1, 1, timeout);
    if (bytesRead != 1) {
        std::lock_guard<std::recursive_mutex> lock(statsMutex_);
        stats_.framingErrors++;
        return msg;
    }

    msg.command = buffer[1];

    // General polls have no data, just 2-byte CRC
    // Long polls may have data followed by 2-byte CRC
    // We need to read until we have a valid CRC or timeout

    size_t totalBytesRead = 2;  // address + command
    const size_t MIN_MESSAGE_SIZE = 4;  // addr + cmd + 2-byte CRC

    // Read remaining bytes (data + CRC)
    // For simplicity, try to read up to MAX_MESSAGE_SIZE
    // In a real implementation, we'd use message-specific lengths
    bytesRead = channel_->read(buffer + 2, MAX_MESSAGE_SIZE - 2, timeout);

    if (bytesRead < 2) {
        // Need at least 2 bytes for CRC
        std::lock_guard<std::recursive_mutex> lock(statsMutex_);
        stats_.framingErrors++;
        return msg;
    }

    totalBytesRead += bytesRead;

    // Try to parse with minimum message size first
    size_t messageLength = MIN_MESSAGE_SIZE;  // Start with addr + cmd + CRC

    // For general polls, message is exactly 4 bytes
    if (isGeneralPoll(msg.command)) {
        messageLength = 4;
    } else {
        // For long polls, we need to determine actual length
        // This would be command-specific in a real implementation
        // For now, assume the data we read is correct
        messageLength = totalBytesRead;
    }

    // Verify CRC
    if (!CRC16::verify(buffer, messageLength)) {
        std::lock_guard<std::recursive_mutex> lock(statsMutex_);
        stats_.crcErrors++;
        msg.command = 0;  // Invalidate message
        return msg;
    }

    // Extract data (if any)
    if (messageLength > 4) {
        size_t dataLength = messageLength - 4;  // Subtract addr, cmd, 2-byte CRC
        msg.data.assign(buffer + 2, buffer + 2 + dataLength);
    }

    msg.crc = CRC16::extract(buffer, messageLength);

    return msg;
}

bool SASCommPort::sendRaw(const uint8_t* buffer, size_t length) {
    if (!channel_ || !channel_->isOpen()) {
        return false;
    }

    int written = channel_->write(buffer, static_cast<int>(length));
    return (written == static_cast<int>(length));
}

} // namespace sas

