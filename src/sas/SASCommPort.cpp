#include "sas/SASCommPort.h"
#include "sas/CRC16.h"
#include "sas/commands/MeterCommands.h"
#include "sas/commands/EnableCommands.h"
#include "sas/commands/ExceptionCommands.h"
#include "sas/commands/DateTimeCommands.h"
#include "sas/commands/ConfigCommands.h"
#include "simulator/Machine.h"
#include "sas/commands/TITOCommands.h"
#include "sas/commands/AFTCommands.h"
#include "sas/commands/ProgressiveCommands.h"
#include "utils/Logger.h"
#include <cstring>
#include <iostream>
#include <sstream>


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
        utils::Logger::log("[SAS] Opening serial channel...");
        if (!channel_->open()) {
            utils::Logger::log("[SAS] ERROR: Failed to open serial channel!");
            return false;
        }
        utils::Logger::log("[SAS] Serial channel opened successfully");
    } else {
        utils::Logger::log("[SAS] Serial channel already open");
    }

    // Start receive thread
    utils::Logger::log("[SAS] Starting receive thread...");
    running_ = true;
    receiveThread_ = std::thread(&SASCommPort::receiveThread, this);
    utils::Logger::log("[SAS] Receive thread started, waiting for data...");

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

    // Debug: Log the Message BEFORE serialize
    {
        std::stringstream ss;
        ss << "[SAS TX PRE-SERIALIZE] Message: addr=0x" << std::hex << (int)msg.address
           << " cmd=0x" << (int)msg.command << std::dec
           << " data_size=" << msg.data.size();
        utils::Logger::log(ss.str());
    }

    // Serialize message (includes CRC calculation)
    std::vector<uint8_t> buffer = msg.serialize();

    // Debug: Log what we're sending
    utils::Logger::logHexVector("[SAS TX] Sending response: ", buffer);

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
    utils::Logger::log("[SAS] Receive thread running, waiting for polls...");

    while (running_) {
        // Read ONE poll at a time from the channel
        Message msg = readMessage(std::chrono::milliseconds(READ_TIMEOUT_MS));

        if (msg.command == 0) {
            // No message or timeout
            readAttempts++;
            if (readAttempts % 100 == 0) {
                utils::Logger::log("[SAS] Still waiting... (" + std::to_string(readAttempts) + " attempts)");
            }
            continue;
        }
        readAttempts = 0;  // Reset counter when we get data

        // Print what we received
        {
            std::stringstream ss;
            ss << "\n===== RECEIVED POLL =====";
            utils::Logger::log(ss.str());
        }
        {
            std::stringstream ss;
            ss << "Address: 0x" << std::hex << (int)msg.address << std::dec;
            utils::Logger::log(ss.str());
        }
        {
            std::stringstream ss;
            ss << "Command: 0x" << std::hex << (int)msg.command << std::dec;
            utils::Logger::log(ss.str());
        }
        utils::Logger::log("Data bytes: " + std::to_string(msg.data.size()));
        if (!msg.data.empty()) {
            std::stringstream ss;
            ss << "Data: ";
            for (size_t i = 0; i < msg.data.size(); i++) {
                char hex[4];
                snprintf(hex, sizeof(hex), "%02X ", msg.data[i]);
                ss << hex;
                if ((i + 1) % 16 == 0 && (i + 1) < msg.data.size()) {
                    utils::Logger::log(ss.str());
                    ss.str("");
                    ss << "      ";
                }
            }
            if (!ss.str().empty()) {
                utils::Logger::log(ss.str());
            }
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

        // Send response to keep master happy
        Message response = processMessage(msg);
        if (response.command != 0) {
            std::stringstream ss;
            ss << "Sending response: 0x" << std::hex << (int)response.command << std::dec;
            utils::Logger::log(ss.str());
            sendMessage(response);
        } else {
            utils::Logger::log("No response (NULL ACK)");
        }

        utils::Logger::log("==============================\n");
    }
}

Message SASCommPort::processMessage(const Message& msg) {
    // Debug 0xA0 routing
    if (msg.command == 0xA0) {
        bool isGenPoll = isGeneralPoll(msg.command);
        std::stringstream ss;
        ss << "[SAS DEBUG] 0xA0 routing: isGeneralPoll()=" << (isGenPoll ? "TRUE" : "FALSE");
        utils::Logger::log(ss.str());
    }

    if (isGeneralPoll(msg.command)) {
        utils::Logger::log("[SAS] Routing to handleGeneralPoll()");
        return handleGeneralPoll(msg);
    } else {
        utils::Logger::log("[SAS] Routing to handleLongPoll()");
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

        // 0x19: Send Total Coin In and Associated Meters (fixed format)
        case LongPoll::SEND_SELECTED_METERS:
            response = commands::MeterCommands::handleSendTotalCoinInAndMeters(machine_);
            break;

        // 0x20: Send Total Bills
        case LongPoll::SEND_TOTAL_BILLS:
            response = commands::MeterCommands::handleSendTotalBills(machine_);
            break;

        // Phase 1 - Basic Meters (Simple 4-byte BCD responses)
        case 0x10:  // Send Cancelled Credits
            response = commands::MeterCommands::handleSendCancelledCredits(machine_);
            break;

        case 0x1A:  // Send Current Credits
            response = commands::MeterCommands::handleSendCurrentCredits(machine_);
            break;

        case 0x1C:  // Send Gaming Machine Meters 1-8
            response = commands::MeterCommands::handleSendGamingMachineMeters(machine_);
            break;

        case 0x1E:  // Send Bill Meters
            response = commands::MeterCommands::handleSendBillMeters(machine_);
            break;

        case 0x2A:  // Send True Coin In
            response = commands::MeterCommands::handleSendTrueCoinIn(machine_);
            break;

        case 0x2B:  // Send True Coin Out
            response = commands::MeterCommands::handleSendTrueCoinOut(machine_);
            break;

        case 0x2D:  // Send Handpay Cancelled Credits
            response = commands::MeterCommands::handleSendHandpayCancelledCredits(machine_, msg.data);
            break;

        case 0x2F:  // Send Selected Meters for Game N
            response = commands::MeterCommands::handleSendSelectedMetersForGameN(machine_, msg.data);
            break;

        // Bill Denomination Meters
        case 0x31:  // Send $1 Bills
            response = commands::MeterCommands::handleSend$1Bills(machine_);
            break;

        case 0x32:  // Send $2 Bills
            response = commands::MeterCommands::handleSend$2Bills(machine_);
            break;

        case 0x33:  // Send $5 Bills
            response = commands::MeterCommands::handleSend$5Bills(machine_);
            break;

        case 0x34:  // Send $10 Bills
            response = commands::MeterCommands::handleSend$10Bills(machine_);
            break;

        case 0x35:  // Send $20 Bills
            response = commands::MeterCommands::handleSend$20Bills(machine_);
            break;

        case 0x36:  // Send $50 Bills
            response = commands::MeterCommands::handleSend$50Bills(machine_);
            break;

        case 0x37:  // Send $100 Bills
            response = commands::MeterCommands::handleSend$100Bills(machine_);
            break;

        case 0x38:  // Send $500 Bills
            response = commands::MeterCommands::handleSend$500Bills(machine_);
            break;

        case 0x39:  // Send $1000 Bills
            response = commands::MeterCommands::handleSend$1000Bills(machine_);
            break;

        case 0x3A:  // Send $200 Bills
            response = commands::MeterCommands::handleSend$200Bills(machine_);
            break;

        case 0x46:  // Send Bills Accepted Credits
            response = commands::MeterCommands::handleSendBillsAcceptedCredits(machine_);
            break;

        // AFT Meters
        case 0x1D:  // Send AFT Registration Meters
            response = commands::AFTCommands::handleSendAFTRegistrationMeters(machine_);
            break;

        case 0x27:  // Send Non-Cashable Electronic Promotion Credits
            response = commands::AFTCommands::handleSendNonCashablePromoCredits(machine_);
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

        // Machine ID and Serial Number
        case LongPoll::SEND_MACHINE_ID_AND_SERIAL:
            response = commands::ConfigCommands::handleSendMachineID(machine_);
            break;

        // Game Configuration Commands
        case 0x51:  // Send Number of Games Implemented
            response = commands::ConfigCommands::handleSendNumberOfGames(machine_);
            break;

        case 0x52:  // Send Selected Game Meters
            response = commands::MeterCommands::handleSendSelectedGameMeters(machine_, msg.data);
            break;

        case 0x53:  // Send Game N Configuration
            response = commands::ConfigCommands::handleSendGameNConfiguration(machine_, msg.data);
            break;

        case 0x55:  // Send Selected Game Number
            response = commands::ConfigCommands::handleSendSelectedGameNumber(machine_);
            break;

        case 0x56:  // Send Enabled Game Numbers
            response = commands::ConfigCommands::handleSendEnabledGameNumbers(machine_);
            break;

        case 0xA0:  // Enable/Disable Game N
            response = commands::ConfigCommands::handleEnableDisableGameN(machine_, msg.data);
            break;

        // Variable-length meter commands with size bytes
        case 0x6F:  // Send Selected Meters for Game N (Extended)
        case 0xAF:  // Send Selected Meters for Game N (Alternate poll value)
            utils::Logger::log("[SAS] Matched case 0x6F/0xAF, calling handler");
            response = commands::MeterCommands::handleSendSelectedMetersForGameNExtended(machine_, msg.command, msg.data);
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

        // Progressive Jackpot commands (0x80+)
        case 0x80:  // SEND_PROGRESSIVE_AMOUNT
            response = commands::ProgressiveCommands::handleSendProgressiveAmount(machine_, msg.data);
            break;

        case 0x84:  // SEND_PROGRESSIVE_WIN
            response = commands::ProgressiveCommands::handleSendProgressiveWin(machine_, msg.data);
            break;

        case 0x85:  // SEND_PROGRESSIVE_LEVELS
            response = commands::ProgressiveCommands::handleSendProgressiveLevels(machine_);
            break;

        case 0x86:  // SEND_PROGRESSIVE_BROADCAST
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

    // IMPORTANT: S7Lite API strips the address byte!
    // Buffer format: cmd only (NO CRC on simple polls!)
    // Minimum: 1 byte (just the command)
    if (bytesRead < 1) {
        std::lock_guard<std::recursive_mutex> lock(statsMutex_);
        stats_.framingErrors++;
        return msg;
    }

    // Extract command (address was stripped by S7Lite API)
    // We'll use the configured address from emulator
    msg.address = address_;  // Use our configured address
    msg.command = buffer[0];

    // If there are additional bytes beyond the command, store them as data
    // (Some polls like 0x74 have parameters)
    if (bytesRead > 1) {
        // Many SAS commands include a 2-byte CRC that needs to be stripped
        // We need to check if this command includes CRC in the input
        bool hasCRC = false;
        switch (msg.command) {
            // Variable-length commands with CRC
            case 0x6F: case 0xAF:  // Send Selected Meters for Game N (Extended)
            case 0x72: case 0x73: case 0x74: case 0x75: case 0x76:  // AFT commands
            case 0x7B: case 0x7C: case 0x7D: case 0x7E: case 0x7F:  // Extended commands
            // Fixed-length long polls with CRC
            case 0xA0:  // Enable/Disable Game N
            case 0x53:  // Send Game N Configuration
            case 0x52:  // Send Selected Game Meters
                hasCRC = true;
                break;
            default:
                hasCRC = false;
                break;
        }

        // Strip CRC (last 2 bytes) if this command includes it
        int dataEnd = hasCRC ? (bytesRead - 2) : bytesRead;
        if (dataEnd > 1) {
            msg.data.assign(buffer + 1, buffer + dataEnd);
        }
    }

    // No CRC on incoming polls from master
    msg.crc = 0;

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

