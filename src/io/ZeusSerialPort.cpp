#include "megamic/io/ZeusSerialPort.h"
#include <cstring>
#include <stdexcept>
#include <iostream>

// Include Zeus OS / Axiomtek S7Lite API header
// This header should be in the include path when building for Zeus OS
#ifdef ZEUS_OS
extern "C" {
#include <s7lite.h>
}
#else
// Stub definitions for compilation on non-Zeus platforms
typedef int S7_Result;
typedef unsigned char BYTE;
typedef unsigned short USHORT;
typedef unsigned int UINT;
typedef void* PVOID;

#define S7DLL_STATUS_OK 0
#define S7DLL_STATUS_ERROR -1
#define CLR_RX_BUFFER 0x01
#define CLR_TX_BUFFER 0x02

// Stub functions for non-Zeus builds
inline S7_Result S7LITE_DLL_Init(void) { return S7DLL_STATUS_ERROR; }
inline S7_Result S7LITE_DLL_DeInit(void) { return S7DLL_STATUS_OK; }
inline S7_Result S7LITE_UART_SendBuffer(UINT uart, USHORT* pbuffer, UINT length) {
    (void)uart; (void)pbuffer; (void)length; return S7DLL_STATUS_ERROR;
}
inline S7_Result S7LITE_UART_GetBuffer(UINT uart, USHORT* pbuffer, UINT* plength) {
    (void)uart; (void)pbuffer; (void)plength; return S7DLL_STATUS_ERROR;
}
inline S7_Result S7LITE_UART_ClearBuffers(UINT uart, USHORT mask) {
    (void)uart; (void)mask; return S7DLL_STATUS_OK;
}
inline S7_Result S7LITE_UART_SetTimeouts(UINT uart, UINT readinterval, UINT writemultipiler, UINT writeconstant) {
    (void)uart; (void)readinterval; (void)writemultipiler; (void)writeconstant;
    return S7DLL_STATUS_OK;
}
inline S7_Result S7LITE_UART_SetBaudRate(UINT uart, UINT baudrate) {
    (void)uart; (void)baudrate; return S7DLL_STATUS_OK;
}
inline S7_Result S7LITE_UART_SetMode(UINT uart, UINT bits, UINT parity, UINT stopbits, UINT flowcontrol) {
    (void)uart; (void)bits; (void)parity; (void)stopbits; (void)flowcontrol;
    return S7DLL_STATUS_OK;
}
inline S7_Result S7LITE_UART_SetClrRTS(UINT uart, UINT value) {
    (void)uart; (void)value; return S7DLL_STATUS_OK;
}
#endif

namespace megamic {
namespace io {

ZeusSerialPort::ZeusSerialPort(const std::string& portName)
    : portName_(portName),
      isOpen_(false),
      dllInitialized_(false),
      readTimeoutMs_(DEFAULT_TIMEOUT_MS),
      lastError_(S7DLL_STATUS_OK) {
}

ZeusSerialPort::~ZeusSerialPort() {
    close();
}

bool ZeusSerialPort::open() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    if (isOpen_) {
        return true;  // Already open
    }

#ifdef ZEUS_OS
    std::cout << "  Initializing S7Lite DLL..." << std::flush;
#endif
    // Initialize Zeus S7Lite DLL
    lastError_ = S7LITE_DLL_Init();
    if (lastError_ != S7DLL_STATUS_OK) {
#ifdef ZEUS_OS
        std::cout << " FAILED (error=" << lastError_ << ")" << std::endl;
#endif
        return false;
    }
#ifdef ZEUS_OS
    std::cout << " OK" << std::endl;
#endif

    dllInitialized_ = true;

#ifdef ZEUS_OS
    std::cout << "  Configuring SAS protocol..." << std::flush;
#endif
    // Configure for SAS (though Zeus hardware is pre-configured)
    configureSAS();
#ifdef ZEUS_OS
    std::cout << " OK" << std::endl;
#endif

#ifdef ZEUS_OS
    std::cout << "  Setting timeouts..." << std::flush;
#endif
    // Set default timeout (now safe to call setTimeout with recursive_mutex)
    setTimeout(readTimeoutMs_);
#ifdef ZEUS_OS
    std::cout << " OK" << std::endl;
#endif

#ifdef ZEUS_OS
    std::cout << "  Clearing buffers..." << std::flush;
#endif
    // Clear any pending data in buffers (now safe to call with recursive_mutex)
    clearBuffers(true, true);
#ifdef ZEUS_OS
    std::cout << " OK" << std::endl;
#endif

    isOpen_ = true;
    return true;
}

void ZeusSerialPort::close() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    if (!isOpen_) {
        return;
    }

    // Clear buffers before closing
    clearBuffers(true, true);

    // Deinitialize Zeus DLL
    if (dllInitialized_) {
        S7LITE_DLL_DeInit();
        dllInitialized_ = false;
    }

    isOpen_ = false;
}

bool ZeusSerialPort::isOpen() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return isOpen_;
}

int ZeusSerialPort::read(uint8_t* buffer, int maxBytes,
                         std::chrono::milliseconds timeout) {
    if (!checkInitialized()) {
        return -1;
    }

    if (buffer == nullptr || maxBytes <= 0) {
        return -1;
    }

    std::lock_guard<std::recursive_mutex> lock(mutex_);

    // Set timeout for this read operation
    uint32_t timeoutMs = static_cast<uint32_t>(timeout.count());
    if (timeoutMs != readTimeoutMs_) {
        S7LITE_UART_SetTimeouts(0, timeoutMs, 0, 0);
    }

    // Prepare Zeus word buffer (16-bit)
    std::vector<USHORT> wordBuffer(maxBytes);
    UINT numWords = static_cast<UINT>(maxBytes);

    // Call Zeus API to read
    lastError_ = S7LITE_UART_GetBuffer(0, wordBuffer.data(), &numWords);

    // Restore original timeout if changed
    if (timeoutMs != readTimeoutMs_) {
        S7LITE_UART_SetTimeouts(0, readTimeoutMs_, 0, 0);
    }

    if (lastError_ != S7DLL_STATUS_OK) {
        return 0;  // Read failed or timeout
    }

    if (numWords == 0) {
        return 0;  // No data available
    }

    // Convert Zeus words (16-bit) to bytes (8-bit)
    // Zeus API returns data in lower 8 bits of each word
    convertFromWords(wordBuffer.data(), numWords, buffer);

    return static_cast<int>(numWords);
}

int ZeusSerialPort::write(const uint8_t* buffer, int numBytes) {
    if (!checkInitialized()) {
        return -1;
    }

    if (buffer == nullptr || numBytes <= 0) {
        return -1;
    }

    std::lock_guard<std::recursive_mutex> lock(mutex_);

    // Convert 8-bit bytes to Zeus 16-bit words
    std::vector<USHORT> wordBuffer = convertToWords(buffer, numBytes);

    // Call Zeus API to send
    lastError_ = S7LITE_UART_SendBuffer(0, wordBuffer.data(),
                                        static_cast<UINT>(wordBuffer.size()));

    if (lastError_ != S7DLL_STATUS_OK) {
        return -1;
    }

    return numBytes;
}

void ZeusSerialPort::flush() {
    if (!checkInitialized()) {
        return;
    }

    std::lock_guard<std::recursive_mutex> lock(mutex_);

    // Flush TX buffer
    clearBuffers(false, true);
}

std::string ZeusSerialPort::getName() const {
    return portName_ + " (Zeus OS Hardware)";
}

void ZeusSerialPort::configureSAS() {
    // Configure Zeus OS hardware for SAS protocol:
    // - 19200 baud
    // - 9-bit mode (8 data bits + wake bit)
    // - No parity (9th bit handles addressing)
    // - 1 stop bit
    // - No handshaking

#ifdef ZEUS_OS
    // Set baud rate to 19200 (SAS standard)
    S7_Result result = S7LITE_UART_SetBaudRate(0, 19200);
    if (result != S7DLL_STATUS_OK) {
        lastError_ = result;
        return;
    }

    // Set mode: 9-bit, no parity, 1 stop bit, no flow control
    // S7LITE_UART_SetMode(uart, bits, parity, stopbits, flowcontrol)
    // bits=9 for SAS wake-up mode
    result = S7LITE_UART_SetMode(0, 9, 0, 0, 0);
    if (result != S7DLL_STATUS_OK) {
        lastError_ = result;
        return;
    }

    // Set RTS control (1 = clear RTS)
    result = S7LITE_UART_SetClrRTS(0, 1);
    if (result != S7DLL_STATUS_OK) {
        lastError_ = result;
        return;
    }
#endif
}

bool ZeusSerialPort::setTimeout(uint32_t timeoutMs) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    if (!dllInitialized_) {
        return false;
    }

    lastError_ = S7LITE_UART_SetTimeouts(0, timeoutMs, 0, 0);
    if (lastError_ == S7DLL_STATUS_OK) {
        readTimeoutMs_ = timeoutMs;
        return true;
    }

    return false;
}

bool ZeusSerialPort::clearBuffers(bool clearRx, bool clearTx) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    if (!dllInitialized_) {
        return false;
    }

    USHORT mask = 0;
    if (clearRx) {
        mask |= CLR_RX_BUFFER;
    }
    if (clearTx) {
        mask |= CLR_TX_BUFFER;
    }

    if (mask == 0) {
        return true;  // Nothing to clear
    }

    lastError_ = S7LITE_UART_ClearBuffers(0, mask);
    return (lastError_ == S7DLL_STATUS_OK);
}

std::vector<USHORT> ZeusSerialPort::convertToWords(const uint8_t* buffer, int numBytes) {
    std::vector<USHORT> words;
    words.reserve(numBytes);

    for (int i = 0; i < numBytes; i++) {
        // Zeus API expects lower 8 bits to contain data
        // Upper 8 bits should be 0x00
        // The 9th bit (wake bit) is handled by hardware
        words.push_back(static_cast<USHORT>(buffer[i]));
    }

    return words;
}

void ZeusSerialPort::convertFromWords(const USHORT* words, int numWords,
                                      uint8_t* buffer) {
    for (int i = 0; i < numWords; i++) {
        // Extract lower 8 bits from Zeus word
        // Zeus API: "only the lower byte of the word contains the data"
        buffer[i] = static_cast<uint8_t>(words[i] & 0xFF);
    }
}

bool ZeusSerialPort::checkInitialized() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    if (!isOpen_ || !dllInitialized_) {
        lastError_ = S7DLL_STATUS_ERROR;
        return false;
    }

    return true;
}

} // namespace io
} // namespace megamic
