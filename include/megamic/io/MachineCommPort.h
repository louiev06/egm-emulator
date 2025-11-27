#ifndef MEGAMIC_IO_MACHINECOMMPORT_H
#define MEGAMIC_IO_MACHINECOMMPORT_H

#include "CommChannel.h"
#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>

namespace megamic {

// Forward declarations
namespace simulator {
    class Machine;
}

namespace io {

/**
 * MachineCommPort - Base class for protocol-specific communication ports
 *
 * This abstract base class defines the interface for communication ports
 * that connect a Machine instance to external systems via specific protocols
 * (SAS, ASP, MS25, etc.).
 *
 * Each protocol port manages:
 * - A communication channel (serial port, network, etc.)
 * - Protocol-specific message framing
 * - Exception queue for event reporting
 * - Command/response handling
 */
class MachineCommPort {
public:
    /**
     * Constructor
     * @param machine Pointer to Machine instance
     * @param channel Communication channel
     */
    MachineCommPort(simulator::Machine* machine, std::shared_ptr<CommChannel> channel);

    /**
     * Virtual destructor
     */
    virtual ~MachineCommPort();

    /**
     * Start the communication port
     * Opens the channel and begins listening for messages
     * @return true if started successfully
     */
    virtual bool start() = 0;

    /**
     * Stop the communication port
     * Closes the channel and stops message processing
     */
    virtual void stop() = 0;

    /**
     * Check if port is running
     */
    virtual bool isRunning() const = 0;

    /**
     * Get the communication channel
     */
    std::shared_ptr<CommChannel> getChannel() const { return channel_; }

    /**
     * Get the machine instance
     */
    simulator::Machine* getMachine() const { return machine_; }

    /**
     * Get port name/description
     */
    virtual std::string getName() const = 0;

    /**
     * Queue an exception for reporting
     * @param exceptionCode Protocol-specific exception code
     */
    virtual void queueException(uint8_t exceptionCode);

    /**
     * Clear all queued exceptions
     */
    virtual void clearExceptions();

    /**
     * Check if exceptions are pending
     */
    virtual bool hasExceptions() const;

protected:
    /**
     * Exception queue entry
     */
    struct Exception {
        uint8_t code;
        uint64_t timestamp;  // Milliseconds since epoch

        Exception(uint8_t c, uint64_t ts) : code(c), timestamp(ts) {}
    };

    simulator::Machine* machine_;                   // Associated machine
    std::shared_ptr<CommChannel> channel_;          // Communication channel
    std::queue<Exception> exceptionQueue_;          // Pending exceptions
    mutable std::recursive_mutex exceptionMutex_;   // Exception queue mutex
    std::condition_variable exceptionCondition_;    // Exception notification

    /**
     * Get current timestamp in milliseconds
     */
    static uint64_t getCurrentTimestamp();
};

} // namespace io
} // namespace megamic

#endif // MEGAMIC_IO_MACHINECOMMPORT_H
