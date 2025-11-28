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
#include <iostream>


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
        std::cout << "[SAS] Opening serial channel..." << std::endl;
        if (!channel_->open()) {
            std::cout << "[SAS] ERROR: Failed to open serial channel!" << std::endl;
            return false;
        }
        std::cout << "[SAS] Serial channel opened successfully" << std::endl;
    } else {
        std::cout << "[SAS] Serial channel already open" << std::endl;
    }

    // Start receive thread
    std::cout << "[SAS] Starting receive thread..." << std::endl;
    running_ = true;
    receiveThread_ = std::thread(&SASCommPort::receiveThread, this);
    std::cout << "[SAS] Receive thread started, waiting for data..." << std::endl;

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
    static int readAttempts = 0;
    while (running_) {
        // Read a message from the channel
        Message msg = readMessage(std::chrono::milliseconds(READ_TIMEOUT_MS));

        if (msg.command == 0) {
            // No message or timeout
            readAttempts++;
            if (readAttempts % 100 == 0) {
                std::cout << "[SAS] Waiting for data... (" << readAttempts << " read attempts)" << std::endl;
            }
            continue;
        }
        readAttempts = 0;  // Reset counter when we get data

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
        std::cout << "[SAS] Processing message..." << std::endl;
        Message response = processMessage(msg);

        // Send response if there is one
        if (response.command != 0) {
            std::cout << "[SAS TX] Sending response, command: 0x" << std::hex << (int)response.command << std::dec << std::endl;
            sendMessage(response);
        } else {
            std::cout << "[SAS] No response needed for this message" << std::endl;
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

    // Read with accumulating timeout - SASSerialPort will loop calling GetBuffer
    // to accumulate all bytes as they arrive over the timeout period
    int bytesRead = channel_->read(buffer, MAX_MESSAGE_SIZE, timeout);

    if (bytesRead <= 0) {
        // Timeout or error - no data available
        return msg;
    }

    if (bytesRead < 4) {
        std::lock_guard<std::recursive_mutex> lock(statsMutex_);
        stats_.framingErrors++;
        std::cout << "[SAS] Incomplete message: received only " << bytesRead << " byte(s), need at least 4" << std::endl;
        return msg;
    }

    size_t totalBytesRead = static_cast<size_t>(bytesRead);

    // Extract address and command
    msg.address = buffer[0];
    msg.command = buffer[1];

    std::cout << "[SAS RX] Address: 0x" << std::hex << (int)buffer[0] << std::dec << std::endl;
    std::cout << "[SAS RX] Command: 0x" << std::hex << (int)buffer[1] << std::dec << std::endl;

    // Determine message length
    size_t messageLength;
    if (isGeneralPoll(msg.command)) {
        // General poll is always exactly 4 bytes
        messageLength = 4;
    } else {
        // Long poll - use all bytes we read
        messageLength = totalBytesRead;
    }

    // Verify CRC
    std::cout << "[SAS RX] Message length: " << messageLength << " bytes, verifying CRC..." << std::endl;
    std::cout << "[SAS RX] Message bytes: ";
    for (size_t i = 0; i < messageLength; i++) {
        printf("%02X ", buffer[i]);
    }
    std::cout << std::endl;

    if (!CRC16::verify(buffer, messageLength)) {
        std::lock_guard<std::recursive_mutex> lock(statsMutex_);
        stats_.crcErrors++;
        msg.command = 0;  // Invalidate message
        std::cout << "[SAS] CRC error!" << std::endl;
        return msg;
    }
    std::cout << "[SAS RX] CRC OK!" << std::endl;

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

