#ifndef MEGAMIC_IO_COMMCHANNEL_H
#define MEGAMIC_IO_COMMCHANNEL_H

#include <cstdint>
#include <vector>
#include <memory>
#include <chrono>

namespace megamic {
namespace io {

/**
 * Abstract communication channel for serial port I/O
 * This is the C++ port of CommChannel.java
 */
class CommChannel {
public:
    virtual ~CommChannel() = default;

    /**
     * Open the communication channel
     * @return true if successful
     */
    virtual bool open() = 0;

    /**
     * Close the communication channel
     */
    virtual void close() = 0;

    /**
     * Check if the channel is open
     */
    virtual bool isOpen() const = 0;

    /**
     * Read bytes from the channel
     * @param buffer Buffer to read into
     * @param maxBytes Maximum number of bytes to read
     * @param timeout Timeout duration
     * @return Number of bytes actually read
     */
    virtual int read(uint8_t* buffer, int maxBytes,
                    std::chrono::milliseconds timeout) = 0;

    /**
     * Write bytes to the channel
     * @param buffer Buffer to write from
     * @param numBytes Number of bytes to write
     * @return Number of bytes actually written
     */
    virtual int write(const uint8_t* buffer, int numBytes) = 0;

    /**
     * Flush any pending output
     */
    virtual void flush() = 0;

    /**
     * Get the channel name/identifier
     */
    virtual std::string getName() const = 0;
};

/**
 * Simulated communication channel using pipes
 * This is the C++ port of PipedCommChannel.java
 */
class PipedCommChannel : public CommChannel {
public:
    PipedCommChannel(const std::string& name);
    ~PipedCommChannel() override;

    bool open() override;
    void close() override;
    bool isOpen() const override;
    int read(uint8_t* buffer, int maxBytes,
            std::chrono::milliseconds timeout) override;
    int write(const uint8_t* buffer, int numBytes) override;
    void flush() override;
    std::string getName() const override;

    /**
     * Connect this channel to another for bidirectional communication
     */
    void connectTo(std::shared_ptr<PipedCommChannel> other);

private:
    std::string name_;
    bool isOpen_;
    std::vector<uint8_t> inputBuffer_;
    std::shared_ptr<PipedCommChannel> connectedChannel_;
};

} // namespace io
} // namespace megamic

#endif // MEGAMIC_IO_COMMCHANNEL_H
