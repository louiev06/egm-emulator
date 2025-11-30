#include "io/SASSerialPort.h"
#include "utils/Logger.h"
#include <iostream>
#include <cstring>
#include <vector>
#include <thread>
#include <chrono>

// Helper macro to flush after every log
#define LOG_FLUSH() std::cout << std::flush

#ifdef ZEUS_OS
extern "C" {
#include <s7lite.h>
}
// s7lite.h defines min/max macros that conflict with std::min/std::max
#undef min
#undef max
#else
// Stub implementations for non-Zeus builds
typedef int S7_Result;
#define S7DLL_STATUS_OK 0
#define S7DLL_STATUS_ERROR -1
#define CLR_RX_BUFFER 1
inline S7_Result S7LITE_DLL_Init(void) { return S7DLL_STATUS_ERROR; }
inline S7_Result S7LITE_DLL_DeInit(void) { return S7DLL_STATUS_OK; }
inline S7_Result S7LITE_UART_SendBuffer(UINT uart, USHORT* pbuffer, UINT length) {
    (void)uart; (void)pbuffer; (void)length; return S7DLL_STATUS_ERROR;
}
inline S7_Result S7LITE_UART_GetBuffer(UINT uart, USHORT* pbuffer, UINT* plength) {
    (void)uart; (void)pbuffer; *plength = 0; return S7DLL_STATUS_ERROR;
}
inline S7_Result S7LITE_UART_SetTimeouts(UINT uart, UINT readinterval, UINT writemultipiler, UINT writeconstant) {
    (void)uart; (void)readinterval; (void)writemultipiler; (void)writeconstant; return S7DLL_STATUS_OK;
}
inline S7_Result S7LITE_UART_SetBaudRate(UINT uart, UINT baudrate) {
    (void)uart; (void)baudrate; return S7DLL_STATUS_OK;
}
inline S7_Result S7LITE_UART_SetMode(UINT uart, UINT bits, UINT parity, UINT stopbits, UINT flowcontrol) {
    (void)uart; (void)bits; (void)parity; (void)stopbits; (void)flowcontrol; return S7DLL_STATUS_OK;
}
inline S7_Result S7LITE_UART_ClearBuffers(UINT uart, UINT flags) {
    (void)uart; (void)flags; return S7DLL_STATUS_OK;
}
#endif

namespace io {

// SAS UART configuration constants (from master CSASSerialWrapper)
static constexpr unsigned int SASUART = 1;
static constexpr unsigned int SASWORDLENGTH = 9;
static constexpr unsigned int SASBAUDRATE = 19200;
static constexpr unsigned int SASREADINTERVAL = 100;
static constexpr unsigned int SASWRITEMULTIPLIER = 2;
static constexpr unsigned int SASWRITECONSTANT = 10;
static constexpr uint16_t SER9BIT_NOMARK = 0x0000;
static constexpr uint16_t SER9BIT_MARK = 0xff00;

SASSerialPort::SASSerialPort()
    : isOpen_(false),
      dllInitialized_(false) {
}

SASSerialPort::~SASSerialPort() {
    close();
}

bool SASSerialPort::open() {
    if (isOpen_) {
        return true;
    }

#ifdef ZEUS_OS
    utils::Logger::logPart("  Initializing S7Lite DLL...");
#endif

    // Initialize S7Lite DLL
    S7_Result result = S7LITE_DLL_Init();
    if (result != S7DLL_STATUS_OK) {
#ifdef ZEUS_OS
        utils::Logger::logPart(" FAILED (error=" + std::to_string(result) + ")");
#endif
        return false;
    }

#ifdef ZEUS_OS
    utils::Logger::logPart(" OK");
#endif

    dllInitialized_ = true;

#ifdef ZEUS_OS
    utils::Logger::logPart("  Configuring SAS UART " + std::to_string(SASUART) + "...");
#endif

    // Configure UART (based on Axiomtek s7uart.c example and nCompass master)
    result = S7LITE_UART_SetMode(SASUART, SASWORDLENGTH, NO_PARITY, STOP_BIT_1, SERIAL_NO_HANDSHAKE);
    if (result != S7DLL_STATUS_OK) {
#ifdef ZEUS_OS
        utils::Logger::logPart(" SetMode FAILED (error=" + std::to_string(result) + ")");
#endif
        S7LITE_DLL_DeInit();
        dllInitialized_ = false;
        return false;
    }

    result = S7LITE_UART_SetBaudRate(SASUART, SASBAUDRATE);
    if (result != S7DLL_STATUS_OK) {
#ifdef ZEUS_OS
        utils::Logger::logPart(" SetBaudRate FAILED (error=" + std::to_string(result) + ")");
#endif
        S7LITE_DLL_DeInit();
        dllInitialized_ = false;
        return false;
    }

    result = S7LITE_UART_SetTimeouts(SASUART, SASREADINTERVAL, SASWRITEMULTIPLIER, SASWRITECONSTANT);
    if (result != S7DLL_STATUS_OK) {
#ifdef ZEUS_OS
        utils::Logger::logPart(" SetTimeouts FAILED (error=" + std::to_string(result) + ")");
#endif
        S7LITE_DLL_DeInit();
        dllInitialized_ = false;
        return false;
    }

#ifdef ZEUS_OS
    utils::Logger::logPart(" OK");
    utils::Logger::log("  SAS UART configured: " + std::to_string(SASBAUDRATE) + " baud, " + std::to_string(SASWORDLENGTH) + "-bit mode");
    utils::Logger::logPart("  Clearing RX buffer...");
#endif

    // Clear any stale data from RX buffer (critical for avoiding 512-byte accumulation)
    result = S7LITE_UART_ClearBuffers(SASUART, CLR_RX_BUFFER);
    if (result != S7DLL_STATUS_OK) {
#ifdef ZEUS_OS
        utils::Logger::logPart(" Clear FAILED (error=" + std::to_string(result) + ")");
#endif
        // Non-fatal, continue anyway
    } else {
#ifdef ZEUS_OS
        utils::Logger::logPart(" OK");
#endif
    }

    isOpen_ = true;
    return true;
}

void SASSerialPort::close() {
    if (!isOpen_) {
        return;
    }

    if (dllInitialized_) {
        S7LITE_DLL_DeInit();
        dllInitialized_ = false;
    }

    isOpen_ = false;
}

bool SASSerialPort::isOpen() const {
    return isOpen_;
}

// GetBuffer - based on master's implementation
// Reads data from SAS UART via S7Lite API
void SASSerialPort::GetBuffer(uint16_t *rBuffer, unsigned int bufferLen, unsigned int &lengthRead) {
    static int debugCounter = 0;
    lengthRead = 0;

    if (!dllInitialized_ || !rBuffer || bufferLen == 0) {
        return;
    }

#ifdef ZEUS_OS
    unsigned int len = bufferLen;
    S7_Result result = S7LITE_UART_GetBuffer(SASUART, (USHORT*)rBuffer, &len);
    lengthRead = len;

    // Debug: Log every 100th call to show we're actually polling
    debugCounter++;
    if (debugCounter % 100 == 0) {
        utils::Logger::log("[SAS UART DEBUG] GetBuffer called " + std::to_string(debugCounter) +
                          " times, result=" + std::to_string(result) + ", read=" + std::to_string(len) + " bytes");
    }

    if (result != S7DLL_STATUS_OK && result != S7DLL_STATUS_ERROR) {
        utils::Logger::log("[SAS UART] GetBuffer error: " + std::to_string(result));
    }
#else
    lengthRead = 0;
#endif
}

// SendBuffer - based on master's implementation
// Writes data to SAS UART via S7Lite API
int SASSerialPort::SendBuffer(uint16_t *wBuffer, unsigned int bufferLen) {
    if (!dllInitialized_ || !wBuffer || bufferLen == 0) {
        return -1;
    }

#ifdef ZEUS_OS
    S7_Result result = S7LITE_UART_SendBuffer(SASUART, (USHORT*)wBuffer, bufferLen);

    if (result != S7DLL_STATUS_OK) {
        utils::Logger::log("[SAS UART] SendBuffer error: " + std::to_string(result));
        return -1;
    }

    return 0;
#else
    return -1;
#endif
}

// Helper: Check if a SAS command has a length field as the second byte
// These are variable-length commands where byte[1] is the data length
static bool hasLengthField(uint8_t cmd) {
    switch (cmd) {
        // Variable-length meter commands
        case 0x6F: // Send Selected Meters for Game N (Extended)
        case 0xAF: // Send Selected Meters for Game N (Alternate poll value)

        // AFT and gaming machine data commands with length fields
        case 0x72: // AFT Transfer Funds
        case 0x73: // AFT Registration
        // NOTE: 0x74 is NOT variable-length! It's fixed 8-byte format
        case 0x75: // AFT Request Lock
        case 0x76: // AFT Game Lock and Status Request
        case 0x7B: // Extended meters (your example)
        case 0x7C: // Set Secure Enhanced Configuration
        case 0x7D: // Send Enabled Game Numbers
        case 0x7E: // Send Current Date and Time
        case 0x7F: // Send Gaming Machine ID and Information
            return true;

        default:
            return false;
    }
}

// Helper: Get expected message length for a SAS command (after address byte stripped)
// Returns number of bytes to read INCLUDING the command byte itself
// For variable-length commands, returns JUST the command byte (1)
// The caller should then read the length field and calculate remaining bytes
static size_t getSASCommandLength(uint8_t cmd) {
    // Note: These are poll lengths (from master to slave)
    // The address byte has already been stripped by S7Lite API

    // For variable-length commands, return 1 so caller reads just the command first
    if (hasLengthField(cmd)) {
        return 1;
    }

    switch (cmd) {
        // General polls - 1 byte (just command)
        case 0x80: // General Poll
        case 0x81: // General Poll
            return 1;

        // Long polls - most are 1 byte
        case 0x19: // Send Total Coin In
        case 0x1A: // Send Total Coin Out
        case 0x1B: // Send Total Drop
        case 0x1C: // Send Total Jackpot
        case 0x1D: // Send Games Played
        case 0x1E: // Send Games Won
        case 0x1F: // Send Credits Wagered
        case 0x20: // Send Total Hand Pays
            return 1;

        // Long polls with parameters (fixed length)
        case 0xA0:  // Enable/Disable Game N: [cmd][game# 2 BCD][CRC 2] = 5 bytes
            return 5;

        case 0x52:  // Send Selected Game Meters: [cmd][game# 2 BCD][CRC 2] = 5 bytes
        case 0x53:  // Send Game N Configuration: [cmd][game# 2 BCD][CRC 2] = 5 bytes
            return 5;

        case 0x74:  // AFT Interrogate Status: [cmd][lock code][xfer cond][timeout 2 BCD][CRC 2] = 8 bytes
            return 8;

        // Commands that may have parameters - need to determine actual lengths
        // For now, assume 1 byte and let's see what we get
        default:
            return 1;  // Default: assume 1 byte (just the command)
    }
}

// Helper: Check if byte is a valid SAS command
static bool isValidSASCommand(uint8_t cmd) {
    // Accept any non-zero byte as potentially valid
    // The command length table above determines how many bytes to read
    return cmd != 0x00;
}

// Helper: Check if command is a general poll (always 4 bytes)
static bool isGeneralPollCommand(uint8_t cmd) {
    return (cmd == 0x81 || cmd == 0x80 || cmd == 0x01 ||  // General poll variants
            cmd == 0x08 || cmd == 0x09 || cmd == 0x0A);    // More general polls
}

// read - buffered reading with message boundary detection
int SASSerialPort::read(uint8_t* buffer, int maxBytes,
                        std::chrono::milliseconds timeout) {
    if (!isOpen_ || !buffer || maxBytes <= 0) {
        return -1;
    }

    // Static buffer to hold data across calls
    static const size_t STATIC_BUFFER_SIZE = 512;
    static uint8_t staticRxBuffer[STATIC_BUFFER_SIZE];
    static size_t staticRxBufferLen = 0;

    auto startTime = std::chrono::steady_clock::now();
    // Minimum message size: just 1 byte (cmd only, no CRC on polls!)
    constexpr unsigned int MIN_MSG_SIZE = 1;

    // Poll for initial command byte if buffer is empty
    while (staticRxBufferLen < MIN_MSG_SIZE) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime);
        if (elapsed >= timeout) {
            return 0;  // Timeout - no data available
        }

        // Request 1 byte at a time - poll constantly
        constexpr unsigned int READ_SIZE = 1;
        std::vector<uint16_t> tempBuffer(READ_SIZE);
        unsigned int len = READ_SIZE;
        GetBuffer(tempBuffer.data(), len, len);

        if (len > 0) {
            // Append to static buffer
            for (unsigned int i = 0; i < len && (staticRxBufferLen + i) < STATIC_BUFFER_SIZE; i++) {
                staticRxBuffer[staticRxBufferLen + i] = static_cast<uint8_t>(tempBuffer[i] & 0xFF);
            }
            staticRxBufferLen += len;

#ifdef ZEUS_OS
            utils::Logger::log("[UART" + std::to_string(SASUART) + " RX] Got " + std::to_string(len) +
                              " bytes from hardware, buffer now has " + std::to_string(staticRxBufferLen) + " bytes");
#endif
            // Got first byte, exit loop to process and determine message length
            break;
        }

        // No data yet, loop continues to poll again
    }

    // Now we have data in staticRxBuffer - find the first valid message
    // NOTE: S7Lite API strips the address byte! Buffer contains: cmd + data + CRC
    // Skip any junk bytes at the start by looking for valid command bytes
    size_t msgStart = 0;
    while (msgStart < staticRxBufferLen && !isValidSASCommand(staticRxBuffer[msgStart])) {
        msgStart++;
    }

    if (msgStart >= staticRxBufferLen) {
        // No valid command found, discard all data and try again
        staticRxBufferLen = 0;
        return 0;
    }

    // Smart variable-length reading:
    // 1. Read command byte
    // 2. Check if it has a length field
    // 3. If yes, read length byte and calculate total message length
    // 4. Poll for remaining bytes if needed

    uint8_t cmd = staticRxBuffer[msgStart];
    size_t messageLength = 0;

    // Only log the first time we process this command (not on retries with partial data)
    static uint8_t lastCmdLogged = 0;
    static auto lastLogTime = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    bool shouldLog = (cmd != lastCmdLogged) ||
                     (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastLogTime).count() > 500);

#ifdef ZEUS_OS
    if (shouldLog) {
        std::stringstream ss;
        ss << "[SAS READ] Processing cmd=0x" << std::hex << (int)cmd << std::dec
           << ", hasLengthField=" << (hasLengthField(cmd) ? "YES" : "NO");
        utils::Logger::log(ss.str());
        lastCmdLogged = cmd;
        lastLogTime = now;
    }
#endif

    if (hasLengthField(cmd)) {
        // Variable-length command: format is [cmd][length][data...][crc16]
        // Need: cmd (1) + length (1) + data (length) + crc (2)

        // First, ensure we have at least cmd + length
        while ((msgStart + 2) > staticRxBufferLen) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - startTime);
            if (elapsed >= timeout) {
                return 0;  // Timeout
            }

            // Poll for length byte
            constexpr unsigned int READ_SIZE = 1;
            std::vector<uint16_t> tempBuffer(READ_SIZE);
            unsigned int len = READ_SIZE;
            GetBuffer(tempBuffer.data(), len, len);

            if (len > 0) {
                for (unsigned int i = 0; i < len && (staticRxBufferLen + i) < STATIC_BUFFER_SIZE; i++) {
                    staticRxBuffer[staticRxBufferLen + i] = static_cast<uint8_t>(tempBuffer[i] & 0xFF);
                }
                staticRxBufferLen += len;

#ifdef ZEUS_OS
                utils::Logger::log("[UART" + std::to_string(SASUART) + " RX] Got length byte, buffer now has " +
                                  std::to_string(staticRxBufferLen) + " bytes");
#endif
            }
        }

        // Now we have cmd + length, calculate total message length
        uint8_t dataLength = staticRxBuffer[msgStart + 1];
        messageLength = 1 + 1 + dataLength + 2;  // cmd + length + data + crc16

#ifdef ZEUS_OS
        {
            std::stringstream ss;
            ss << "[SAS SMART READ] Cmd 0x" << std::hex << (int)cmd
               << " has length field=" << std::dec << (int)dataLength
               << ", total msg=" << messageLength << " bytes";
            utils::Logger::log(ss.str());
        }
#endif

        // Poll for remaining bytes if we don't have the complete message yet
        // MCU delivers data in bursts - keep polling until we get the complete message
        while ((msgStart + messageLength) > staticRxBufferLen) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - startTime);

            // Only timeout if we've been waiting for the ENTIRE message for too long
            // Don't timeout on idle - MCU can have long gaps between bursts
            if (elapsed >= timeout) {
#ifdef ZEUS_OS
                utils::Logger::log("[UART" + std::to_string(SASUART) + " RX] Read timeout after " + std::to_string(elapsed.count()) +
                                  "ms (limit=" + std::to_string(timeout.count()) + "ms) - returning partial message " +
                                  "(got " + std::to_string(staticRxBufferLen) + " bytes, need " +
                                  std::to_string(msgStart + messageLength) + ") - will retry on next read()");
#endif
                // Don't discard! Keep partial data in buffer for next read() call
                // Just return 0 to indicate no complete message yet
                return 0;
            }

            // Poll for more data - request exactly how many bytes we still need
            size_t bytesNeeded = (msgStart + messageLength) - staticRxBufferLen;
            unsigned int READ_SIZE = std::min(static_cast<size_t>(256), bytesNeeded);  // Read up to 256 bytes at once
            std::vector<uint16_t> tempBuffer(READ_SIZE);
            unsigned int len = READ_SIZE;
            GetBuffer(tempBuffer.data(), len, len);

            if (len > 0) {
                for (unsigned int i = 0; i < len && (staticRxBufferLen + i) < STATIC_BUFFER_SIZE; i++) {
                    staticRxBuffer[staticRxBufferLen + i] = static_cast<uint8_t>(tempBuffer[i] & 0xFF);
                }
                staticRxBufferLen += len;

#ifdef ZEUS_OS
                utils::Logger::log("[UART" + std::to_string(SASUART) + " RX] Got " + std::to_string(len) +
                                  " bytes, buffer now has " + std::to_string(staticRxBufferLen) +
                                  " bytes (need " + std::to_string(msgStart + messageLength) + ")");
#endif
            }
        }
    } else {
        // Fixed-length command (no length field)
        messageLength = getSASCommandLength(cmd);

        // Poll for remaining bytes if we don't have the complete message yet
        while ((msgStart + messageLength) > staticRxBufferLen) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - startTime);

            if (elapsed >= timeout) {
#ifdef ZEUS_OS
                utils::Logger::log("[UART" + std::to_string(SASUART) + " RX] Fixed-length timeout - " +
                                  "cmd=0x" + [](uint8_t c){ char b[3]; snprintf(b,3,"%02X",c); return std::string(b); }(cmd) +
                                  " needs " + std::to_string(messageLength) + " bytes, have " +
                                  std::to_string(staticRxBufferLen - msgStart));
#endif
                return 0;  // Timeout - will retry on next read()
            }

            // Poll for more data
            size_t bytesNeeded = (msgStart + messageLength) - staticRxBufferLen;
            unsigned int READ_SIZE = std::min(static_cast<size_t>(256), bytesNeeded);
            std::vector<uint16_t> tempBuffer(READ_SIZE);
            unsigned int len = READ_SIZE;
            GetBuffer(tempBuffer.data(), len, len);

            if (len > 0) {
                for (unsigned int i = 0; i < len && (staticRxBufferLen + i) < STATIC_BUFFER_SIZE; i++) {
                    staticRxBuffer[staticRxBufferLen + i] = static_cast<uint8_t>(tempBuffer[i] & 0xFF);
                }
                staticRxBufferLen += len;

#ifdef ZEUS_OS
                utils::Logger::log("[UART" + std::to_string(SASUART) + " RX] Got " + std::to_string(len) +
                                  " bytes for fixed-length cmd, buffer now has " + std::to_string(staticRxBufferLen) +
                                  " bytes (need " + std::to_string(msgStart + messageLength) + ")");
#endif
            }
        }
    }

    // Copy message to output buffer
    size_t toCopy = std::min(messageLength, static_cast<size_t>(maxBytes));
    memcpy(buffer, staticRxBuffer + msgStart, toCopy);

    // Remove processed data from static buffer (including skipped junk)
    size_t consumed = msgStart + toCopy;
    if (consumed < staticRxBufferLen) {
        memmove(staticRxBuffer, staticRxBuffer + consumed, staticRxBufferLen - consumed);
        staticRxBufferLen -= consumed;
    } else {
        staticRxBufferLen = 0;
    }

#ifdef ZEUS_OS
    {
        std::stringstream ss;
        ss << "[UART" << SASUART << " RX] Returning " << toCopy << " bytes (skipped " << msgStart << " junk bytes): ";
        for (size_t i = 0; i < toCopy && i < 16; i++) {
            char hex[4];
            snprintf(hex, sizeof(hex), "%02X ", buffer[i]);
            ss << hex;
        }
        if (toCopy > 16) ss << "...";
        ss << " | Buffer remaining: " << staticRxBufferLen << " bytes";
        utils::Logger::log(ss.str());
    }
#endif

    return static_cast<int>(toCopy);
}

// write - based on master's WriteSAS implementation
int SASSerialPort::write(const uint8_t* buffer, int numBytes) {
    if (!isOpen_ || !buffer || numBytes <= 0) {
        return -1;
    }

    // Convert byte buffer to uint16_t buffer for S7Lite API
    std::vector<uint16_t> wBuffer(numBytes);

    // EGM responses: ALL bytes get space parity (no mark bit)
    // Per SAS spec wakeup mode: "Gaming machines clear the wakeup bit for all bytes when responding"
    // Only the MASTER sets the mark bit on the first byte (address) when sending polls
    for (int i = 0; i < numBytes; i++) {
        wBuffer[i] = static_cast<uint16_t>(buffer[i] | SER9BIT_NOMARK);
    }

#ifdef ZEUS_OS
    {
        std::stringstream ss;
        ss << "[UART" << SASUART << " TX] Sending " << numBytes << " bytes: ";
        for (int i = 0; i < numBytes && i < 16; i++) {
            char hex[4];
            snprintf(hex, sizeof(hex), "%02X ", buffer[i]);
            ss << hex;
        }
        utils::Logger::log(ss.str());
    }
#endif

    int result = SendBuffer(wBuffer.data(), static_cast<unsigned int>(numBytes));

    if (result != 0) {
        return -1;
    }

    return numBytes;
}

void SASSerialPort::flush() {
    // S7Lite API doesn't expose a flush function
    // Data is sent immediately via SendBuffer
}

std::string SASSerialPort::getName() const {
    return "SAS UART 1 (Zeus S7Lite)";
}

} // namespace io
