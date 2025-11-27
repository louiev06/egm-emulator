#ifndef MEGAMIC_IO_ZEUSSERIALPORT_H
#define MEGAMIC_IO_ZEUSSERIALPORT_H

#include "CommChannel.h"
#include <vector>
#include <mutex>
#include <memory>

// Forward declare Zeus/S7Lite API types (to avoid pulling in full header)
typedef int S7_Result;
typedef unsigned short USHORT;

namespace megamic {
namespace io {

/**
 * ZeusSerialPort - Serial port implementation for Zeus OS using Axiomtek S7Lite API
 *
 * This class wraps the Zeus OS S7LITE_UART_* functions to provide SAS protocol
 * communication through the hardware serial port.
 *
 * Key Zeus API Functions Used:
 * - S7LITE_DLL_Init() - Initialize hardware
 * - S7LITE_UART_SendBuffer() - Send data (9-bit SAS)
 * - S7LITE_UART_GetBuffer() - Receive data (9-bit SAS)
 * - S7LITE_UART_ClearBuffers() - Clear TX/RX buffers
 * - S7LITE_UART_SetTimeouts() - Configure read timeout
 * - S7LITE_DLL_DeInit() - Cleanup hardware
 *
 * Note: Zeus OS API uses 16-bit words (USHORT) where only the lower 8 bits
 * contain data for SAS protocol (9-bit mode with 8th bit as wake bit).
 */
class ZeusSerialPort : public CommChannel {
public:
    /**
     * Constructor
     * @param portName Not used on Zeus (fixed hardware port), kept for compatibility
     */
    explicit ZeusSerialPort(const std::string& portName = "SAS");

    /**
     * Destructor - ensures proper cleanup
     */
    ~ZeusSerialPort() override;

    // CommChannel interface implementation
    bool open() override;
    void close() override;
    bool isOpen() const override;
    int read(uint8_t* buffer, int maxBytes, std::chrono::milliseconds timeout) override;
    int write(const uint8_t* buffer, int numBytes) override;
    void flush() override;
    std::string getName() const override;

    /**
     * Configure SAS-specific parameters
     * Note: Zeus OS hardware is pre-configured for SAS:
     * - 19200 baud
     * - 9-bit mode (8 data + wake bit)
     * - No parity (handled by 9th bit)
     * - 1 stop bit
     * - No handshaking
     */
    void configureSAS();

    /**
     * Set read timeout (Zeus API: S7LITE_UART_SetTimeouts)
     * @param timeout Timeout in milliseconds
     * @return true if successful
     */
    bool setTimeout(uint32_t timeoutMs);

    /**
     * Clear receive and/or transmit buffers
     * @param clearRx Clear receive buffer
     * @param clearTx Clear transmit buffer
     * @return true if successful
     */
    bool clearBuffers(bool clearRx = true, bool clearTx = true);

    /**
     * Get the last Zeus API error code
     */
    S7_Result getLastError() const { return lastError_; }

private:
    /**
     * Convert 8-bit buffer to Zeus 16-bit word buffer
     * Zeus API expects USHORT array where lower 8 bits = data
     */
    std::vector<USHORT> convertToWords(const uint8_t* buffer, int numBytes);

    /**
     * Convert Zeus 16-bit word buffer to 8-bit buffer
     * Extract lower 8 bits from each word (mask out wake bit)
     */
    void convertFromWords(const USHORT* words, int numWords, uint8_t* buffer);

    /**
     * Check if Zeus DLL is initialized
     */
    bool checkInitialized();

    std::string portName_;
    bool isOpen_;
    bool dllInitialized_;
    uint32_t readTimeoutMs_;
    S7_Result lastError_;

    mutable std::recursive_mutex mutex_;  // Thread safety for Zeus API calls

    static constexpr uint32_t DEFAULT_TIMEOUT_MS = 20;  // SAS typical timeout
    static constexpr uint32_t MAX_BUFFER_SIZE = 256;    // Safe buffer size
};

} // namespace io
} // namespace megamic

#endif // MEGAMIC_IO_ZEUSSERIALPORT_H
