#ifndef SASSERIALPORT_H
#define SASSERIALPORT_H

#include "CommChannel.h"
#include <memory>
#include <chrono>
#include <cstdint>

namespace io {

// Forward declare S7 types - will be defined in cpp file
#ifdef ZEUS_OS
class SASSerialPortImpl;
#endif

/**
 * SASSerialPort - Serial port implementation for SAS communication on Zeus OS
 *
 * This implementation is based on the master's CSASSerialWrapper and uses
 * the S7Lite API to communicate with the SAS UART hardware.
 *
 * Key differences from master implementation:
 * - Simplified for slave mode (no polling delays needed)
 * - Focused on read/write operations
 * - No timing enforcement (slave responds when polled)
 */
class SASSerialPort : public CommChannel {
public:
    SASSerialPort();
    ~SASSerialPort() override;

    // CommChannel interface
    bool open() override;
    void close() override;
    bool isOpen() const override;

    int read(uint8_t* buffer, int maxBytes,
             std::chrono::milliseconds timeout) override;
    int write(const uint8_t* buffer, int numBytes) override;
    void flush() override;
    std::string getName() const override;

private:
    bool isOpen_;
    bool dllInitialized_;

    // Platform-specific helpers (implemented in cpp)
    void GetBuffer(uint16_t *rBuffer, unsigned int bufferLen, unsigned int &lengthRead);
    int SendBuffer(uint16_t *wBuffer, unsigned int bufferLen);
};

} // namespace io

#endif // SASSERIALPORT_H
