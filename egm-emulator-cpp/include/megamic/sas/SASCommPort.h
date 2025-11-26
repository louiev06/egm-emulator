#ifndef MEGAMIC_SAS_SASCOMMPORT_H
#define MEGAMIC_SAS_SASCOMMPORT_H

#include "megamic/io/MachineCommPort.h"
#include "megamic/sas/SASCommands.h"
#include <thread>
#include <atomic>
#include <vector>

namespace megamic {
namespace sas {

/**
 * SASCommPort - SAS protocol communication port
 *
 * Implements the SAS (Slot Accounting System) protocol for communication
 * with casino management systems.
 *
 * Features:
 * - General poll handling (0x80-0x9F)
 * - Long poll command processing (0x00-0x7F)
 * - Exception queue for real-time event reporting
 * - CRC-16 validation
 * - Message framing with 9-bit addressing
 *
 * Thread Model:
 * - Receive thread continuously monitors for incoming messages
 * - Messages are processed synchronously and responses sent immediately
 * - Exception queue is thread-safe for cross-thread access
 */
class SASCommPort : public io::MachineCommPort {
public:
    /**
     * Constructor
     * @param machine Pointer to Machine instance
     * @param channel Communication channel (must support 9-bit mode for SAS)
     * @param address SAS machine address (1-127, default 1)
     */
    SASCommPort(simulator::Machine* machine,
                std::shared_ptr<io::CommChannel> channel,
                uint8_t address = 1);

    /**
     * Destructor - stops port and cleans up
     */
    ~SASCommPort() override;

    // MachineCommPort interface
    bool start() override;
    void stop() override;
    bool isRunning() const override;
    std::string getName() const override;

    /**
     * Get SAS address
     */
    uint8_t getAddress() const { return address_; }

    /**
     * Get SAS poll address (alias for getAddress)
     */
    uint8_t getPollAddress() const { return address_; }

    /**
     * Set SAS address (must be 1-127)
     */
    void setAddress(uint8_t address);

    /**
     * Send a SAS message
     * @param msg Message to send (CRC will be calculated automatically)
     * @return true if sent successfully
     */
    bool sendMessage(const Message& msg);

    /**
     * Get statistics
     */
    struct Statistics {
        uint64_t messagesReceived;
        uint64_t messagesSent;
        uint64_t crcErrors;
        uint64_t framingErrors;
        uint64_t generalPolls;
        uint64_t longPolls;

        Statistics() : messagesReceived(0), messagesSent(0), crcErrors(0),
                      framingErrors(0), generalPolls(0), longPolls(0) {}
    };

    Statistics getStatistics() const;
    void resetStatistics();

protected:
    /**
     * Receive thread entry point
     */
    void receiveThread();

    /**
     * Process a received SAS message
     * @param msg Received message
     * @return Response message (if any)
     */
    Message processMessage(const Message& msg);

    /**
     * Handle general poll (0x80-0x9F)
     * @param msg Received general poll message
     * @return Exception response (if pending)
     */
    Message handleGeneralPoll(const Message& msg);

    /**
     * Handle long poll commands (0x00-0x7F)
     * @param msg Received long poll message
     * @return Command response
     */
    Message handleLongPoll(const Message& msg);

    /**
     * Read a complete SAS message from channel
     * @param timeout Read timeout
     * @return Received message (empty if timeout or error)
     */
    Message readMessage(std::chrono::milliseconds timeout);

    /**
     * Send raw bytes to channel
     * @param buffer Data buffer
     * @param length Buffer length
     * @return true if sent successfully
     */
    bool sendRaw(const uint8_t* buffer, size_t length);

private:
    uint8_t address_;                       // SAS machine address (1-127)
    std::atomic<bool> running_;             // Port running flag
    std::thread receiveThread_;             // Receive thread
    Statistics stats_;                      // Communication statistics
    mutable std::mutex statsMutex_;         // Statistics mutex

    static constexpr size_t MAX_MESSAGE_SIZE = 256;
    static constexpr int READ_TIMEOUT_MS = 50;
};

} // namespace sas
} // namespace megamic

#endif // MEGAMIC_SAS_SASCOMMPORT_H
