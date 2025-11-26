#ifndef MEGAMIC_SAS_SASDAEMON_H
#define MEGAMIC_SAS_SASDAEMON_H

#include "megamic/sas/SASCommPort.h"
#include "megamic/simulator/Machine.h"
#include <thread>
#include <atomic>
#include <chrono>
#include <memory>

namespace megamic {
namespace sas {

/**
 * SASDaemon - SAS Protocol Polling Daemon
 *
 * Manages continuous polling of SAS protocol, alternating between:
 * - General polls (0x80) - Check for exceptions/events
 * - Long polls - Query meters, configuration, status
 *
 * Operating Modes:
 * - Discovery: Initial connection, query machine capabilities
 * - Online: Normal operation with continuous polling
 */
class SASDaemon {
public:
    /**
     * Operating mode
     */
    enum class Mode {
        DISCOVERY,  // Initial discovery and configuration
        ONLINE,     // Normal operation
        OFFLINE     // Not connected
    };

    /**
     * Daemon statistics
     */
    struct Statistics {
        uint64_t totalPolls;            // Total polls sent
        uint64_t generalPolls;          // General polls sent
        uint64_t longPolls;             // Long polls sent
        uint64_t exceptionsReceived;    // Exceptions received
        uint64_t timeouts;              // Poll timeouts
        uint64_t errors;                // Communication errors
        std::chrono::steady_clock::time_point startTime;

        Statistics() : totalPolls(0), generalPolls(0), longPolls(0),
                      exceptionsReceived(0), timeouts(0), errors(0),
                      startTime(std::chrono::steady_clock::now()) {}
    };

    /**
     * Constructor
     * @param machine Machine instance to poll
     * @param port SAS communication port
     */
    SASDaemon(simulator::Machine* machine, SASCommPort* port);

    /**
     * Destructor - stops polling thread
     */
    ~SASDaemon();

    /**
     * Start polling daemon
     * @return true if started successfully
     */
    bool start();

    /**
     * Stop polling daemon
     */
    void stop();

    /**
     * Check if daemon is running
     * @return true if running
     */
    bool isRunning() const { return running_; }

    /**
     * Get current operating mode
     * @return Current mode
     */
    Mode getMode() const { return mode_; }

    /**
     * Set operating mode
     * @param mode New mode
     */
    void setMode(Mode mode);

    /**
     * Get polling statistics
     * @return Statistics structure
     */
    Statistics getStatistics() const;

    /**
     * Reset statistics
     */
    void resetStatistics();

    /**
     * Set general poll interval
     * @param interval Time between general polls (default: 40ms)
     */
    void setGeneralPollInterval(std::chrono::milliseconds interval);

    /**
     * Set long poll interval
     * @param interval Time between long poll cycles (default: 1000ms)
     */
    void setLongPollInterval(std::chrono::milliseconds interval);

    /**
     * Set poll timeout
     * @param timeout Maximum time to wait for response (default: 100ms)
     */
    void setPollTimeout(std::chrono::milliseconds timeout);

private:
    /**
     * Main polling thread function
     */
    void pollingThread();

    /**
     * Discovery mode - query machine capabilities
     */
    void runDiscovery();

    /**
     * Online mode - continuous polling
     */
    void runOnline();

    /**
     * Send general poll and process response
     * @return true if exception received
     */
    bool doGeneralPoll();

    /**
     * Send a long poll command
     * @param command Command code
     * @param data Optional command data
     * @return true if response received
     */
    bool doLongPoll(uint8_t command, const std::vector<uint8_t>& data = {});

    /**
     * Process exception received from general poll
     * @param exceptionCode Exception code
     */
    void processException(uint8_t exceptionCode);

    /**
     * Query game configuration (discovery)
     */
    void queryGameConfiguration();

    /**
     * Query all meters (online)
     */
    void queryMeters();

    /**
     * Query progressive levels (online)
     */
    void queryProgressives();

    /**
     * Check for connection status
     */
    void checkConnection();

    // Member variables
    simulator::Machine* machine_;
    SASCommPort* port_;
    std::unique_ptr<std::thread> pollingThread_;
    std::atomic<bool> running_;
    std::atomic<Mode> mode_;

    // Timing configuration
    std::chrono::milliseconds generalPollInterval_;  // Time between general polls
    std::chrono::milliseconds longPollInterval_;     // Time between long poll cycles
    std::chrono::milliseconds pollTimeout_;          // Response timeout

    // Statistics
    mutable std::mutex statsMutex_;
    Statistics stats_;

    // Long poll cycle tracking
    std::chrono::steady_clock::time_point lastLongPoll_;
    uint8_t currentLongPollIndex_;

    // Connection state
    bool connected_;
    int consecutiveTimeouts_;
    static constexpr int MAX_CONSECUTIVE_TIMEOUTS = 10;

    // Default timing values
    static constexpr int DEFAULT_GENERAL_POLL_INTERVAL_MS = 40;   // 40ms between general polls
    static constexpr int DEFAULT_LONG_POLL_INTERVAL_MS = 1000;    // 1 second between long poll cycles
    static constexpr int DEFAULT_POLL_TIMEOUT_MS = 100;           // 100ms timeout for responses
};

} // namespace sas
} // namespace megamic

#endif // MEGAMIC_SAS_SASDAEMON_H
