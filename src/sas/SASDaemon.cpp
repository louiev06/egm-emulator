#include "megamic/sas/SASDaemon.h"
#include "megamic/sas/SASCommands.h"
#include <thread>

namespace megamic {
namespace sas {

SASDaemon::SASDaemon(simulator::Machine* machine, SASCommPort* port)
    : machine_(machine),
      port_(port),
      running_(false),
      mode_(Mode::OFFLINE),
      generalPollInterval_(std::chrono::milliseconds(DEFAULT_GENERAL_POLL_INTERVAL_MS)),
      longPollInterval_(std::chrono::milliseconds(DEFAULT_LONG_POLL_INTERVAL_MS)),
      pollTimeout_(std::chrono::milliseconds(DEFAULT_POLL_TIMEOUT_MS)),
      lastLongPoll_(std::chrono::steady_clock::now()),
      currentLongPollIndex_(0),
      connected_(false),
      consecutiveTimeouts_(0) {
}

SASDaemon::~SASDaemon() {
    stop();
}

bool SASDaemon::start() {
    if (running_) {
        return true;  // Already running
    }

    if (!machine_ || !port_) {
        return false;  // Invalid configuration
    }

    // Start SAS port
    if (!port_->start()) {
        return false;  // Failed to start port
    }

    // Reset statistics
    {
        std::lock_guard<std::recursive_mutex> lock(statsMutex_);
        stats_ = Statistics();
    }

    // Start polling thread
    running_ = true;
    mode_ = Mode::DISCOVERY;
    pollingThread_ = std::unique_ptr<std::thread>(new std::thread(&SASDaemon::pollingThread, this));

    return true;
}

void SASDaemon::stop() {
    if (!running_) {
        return;  // Not running
    }

    // Signal thread to stop
    running_ = false;

    // Wait for thread to finish
    if (pollingThread_ && pollingThread_->joinable()) {
        pollingThread_->join();
    }

    pollingThread_.reset();
    mode_ = Mode::OFFLINE;
}

void SASDaemon::setMode(Mode mode) {
    mode_ = mode;
}

SASDaemon::Statistics SASDaemon::getStatistics() const {
    std::lock_guard<std::recursive_mutex> lock(statsMutex_);
    return stats_;
}

void SASDaemon::resetStatistics() {
    std::lock_guard<std::recursive_mutex> lock(statsMutex_);
    stats_ = Statistics();
}

void SASDaemon::setGeneralPollInterval(std::chrono::milliseconds interval) {
    generalPollInterval_ = interval;
}

void SASDaemon::setLongPollInterval(std::chrono::milliseconds interval) {
    longPollInterval_ = interval;
}

void SASDaemon::setPollTimeout(std::chrono::milliseconds timeout) {
    pollTimeout_ = timeout;
}

void SASDaemon::pollingThread() {
    while (running_) {
        Mode currentMode = mode_;

        switch (currentMode) {
            case Mode::DISCOVERY:
                runDiscovery();
                break;

            case Mode::ONLINE:
                runOnline();
                break;

            case Mode::OFFLINE:
                // Wait and check connection
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                checkConnection();
                break;
        }
    }
}

void SASDaemon::runDiscovery() {
    // Discovery mode - query machine capabilities and configuration

    // Step 1: Send enable game command
    doLongPoll(LongPoll::ENABLE_GAME);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Step 2: Query game configuration
    queryGameConfiguration();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Step 3: Query initial meters
    queryMeters();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Step 4: Query progressive levels
    queryProgressives();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Discovery complete - switch to online mode
    connected_ = true;
    mode_ = Mode::ONLINE;
}

void SASDaemon::runOnline() {
    // Online mode - continuous polling

    // Send general poll to check for exceptions
    bool hasException = doGeneralPoll();

    if (hasException) {
        // Exception received, send another general poll immediately
        // to drain the exception queue
        return;
    }

    // Check if it's time for a long poll cycle
    auto now = std::chrono::steady_clock::now();
    auto timeSinceLastLongPoll = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - lastLongPoll_);

    if (timeSinceLastLongPoll >= longPollInterval_) {
        // Time for a long poll
        switch (currentLongPollIndex_) {
            case 0:
                // Query meters
                doLongPoll(LongPoll::SEND_TOTAL_COIN_IN);
                break;
            case 1:
                doLongPoll(LongPoll::SEND_TOTAL_COIN_OUT);
                break;
            case 2:
                doLongPoll(LongPoll::SEND_GAMES_PLAYED);
                break;
            case 3:
                doLongPoll(LongPoll::SEND_GAMES_WON);
                break;
            case 4:
                // Query progressive levels
                doLongPoll(LongPoll::SEND_PROGRESSIVE_LEVELS);
                break;
            case 5:
                // Query date/time
                doLongPoll(LongPoll::SEND_DATE_TIME);
                break;
            default:
                currentLongPollIndex_ = 0;
                break;
        }

        currentLongPollIndex_ = (currentLongPollIndex_ + 1) % 6;
        lastLongPoll_ = now;
    }

    // Sleep for general poll interval
    std::this_thread::sleep_for(generalPollInterval_);
}

bool SASDaemon::doGeneralPoll() {
    // Send general poll (0x80 + address)
    Message pollMsg;
    pollMsg.address = port_->getPollAddress();
    pollMsg.command = 0x80 + port_->getPollAddress();  // General poll base address

    // Send and wait for response
    bool success = port_->sendMessage(pollMsg);

    {
        std::lock_guard<std::recursive_mutex> lock(statsMutex_);
        stats_.totalPolls++;
        stats_.generalPolls++;
    }

    if (!success) {
        consecutiveTimeouts_++;
        {
            std::lock_guard<std::recursive_mutex> lock(statsMutex_);
            stats_.timeouts++;
        }

        if (consecutiveTimeouts_ >= MAX_CONSECUTIVE_TIMEOUTS) {
            // Lost connection
            connected_ = false;
            mode_ = Mode::OFFLINE;
        }

        return false;
    }

    // Reset timeout counter on success
    consecutiveTimeouts_ = 0;
    connected_ = true;

    // Check if we received an exception
    // The port handles exception responses internally
    // We just need to check if there was data
    return false;  // Simplified - in real implementation would check response
}

bool SASDaemon::doLongPoll(uint8_t command, const std::vector<uint8_t>& data) {
    // Send long poll command
    Message pollMsg;
    pollMsg.address = port_->getPollAddress();
    pollMsg.command = command;
    pollMsg.data = data;

    bool success = port_->sendMessage(pollMsg);

    {
        std::lock_guard<std::recursive_mutex> lock(statsMutex_);
        stats_.totalPolls++;
        stats_.longPolls++;
    }

    if (!success) {
        consecutiveTimeouts_++;
        {
            std::lock_guard<std::recursive_mutex> lock(statsMutex_);
            stats_.timeouts++;
        }

        if (consecutiveTimeouts_ >= MAX_CONSECUTIVE_TIMEOUTS) {
            connected_ = false;
            mode_ = Mode::OFFLINE;
        }
        return false;
    }

    consecutiveTimeouts_ = 0;
    connected_ = true;
    return true;
}

void SASDaemon::processException(uint8_t exceptionCode) {
    {
        std::lock_guard<std::recursive_mutex> lock(statsMutex_);
        stats_.exceptionsReceived++;
    }

    // Exception codes indicate various events:
    // 0x11 - Door opened
    // 0x51 - Handpay pending
    // 0x60 - Game started
    // etc.

    // In a real implementation, would process specific exceptions
    // and trigger appropriate actions or notifications
}

void SASDaemon::queryGameConfiguration() {
    // Query game configuration and capabilities
    doLongPoll(LongPoll::SEND_GAME_CONFIG);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    doLongPoll(LongPoll::SEND_GAME_NUMBER);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
}

void SASDaemon::queryMeters() {
    // Query all important meters
    doLongPoll(LongPoll::SEND_TOTAL_COIN_IN);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    doLongPoll(LongPoll::SEND_TOTAL_COIN_OUT);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    doLongPoll(LongPoll::SEND_TOTAL_DROP);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    doLongPoll(LongPoll::SEND_TOTAL_JACKPOT);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    doLongPoll(LongPoll::SEND_GAMES_PLAYED);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    doLongPoll(LongPoll::SEND_GAMES_WON);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    doLongPoll(LongPoll::SEND_GAMES_LOST);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
}

void SASDaemon::queryProgressives() {
    // Query progressive jackpot levels
    doLongPoll(LongPoll::SEND_PROGRESSIVE_LEVELS);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
}

void SASDaemon::checkConnection() {
    // Try to start port and enter discovery mode
    if (port_->start()) {
        mode_ = Mode::DISCOVERY;
    }
}

} // namespace sas
} // namespace megamic
