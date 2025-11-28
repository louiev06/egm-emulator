#include "io/SASSerialPort.h"
#include <iostream>
#include <cstring>
#include <vector>
#include <thread>
#include <chrono>

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
    std::cout << "  Initializing S7Lite DLL..." << std::flush;
#endif

    // Initialize S7Lite DLL
    S7_Result result = S7LITE_DLL_Init();
    if (result != S7DLL_STATUS_OK) {
#ifdef ZEUS_OS
        std::cout << " FAILED (error=" << result << ")" << std::endl;
#endif
        return false;
    }

#ifdef ZEUS_OS
    std::cout << " OK" << std::endl;
#endif

    dllInitialized_ = true;

#ifdef ZEUS_OS
    std::cout << "  Configuring SAS UART " << SASUART << "..." << std::flush;
#endif

    // Configure UART (based on master's OpenSAS)
    // Note: Per master implementation, these functions validate params but
    // the actual UART is pre-configured by the MCU firmware

    result = S7LITE_UART_SetMode(SASUART, SASWORDLENGTH, NO_PARITY, STOP_BIT_1, SERIAL_NO_HANDSHAKE);
    if (result != S7DLL_STATUS_OK) {
#ifdef ZEUS_OS
        std::cout << " SetMode FAILED (error=" << result << ")" << std::endl;
#endif
        S7LITE_DLL_DeInit();
        dllInitialized_ = false;
        return false;
    }

    result = S7LITE_UART_SetBaudRate(SASUART, SASBAUDRATE);
    if (result != S7DLL_STATUS_OK) {
#ifdef ZEUS_OS
        std::cout << " SetBaudRate FAILED (error=" << result << ")" << std::endl;
#endif
        S7LITE_DLL_DeInit();
        dllInitialized_ = false;
        return false;
    }

    result = S7LITE_UART_SetTimeouts(SASUART, SASREADINTERVAL, SASWRITEMULTIPLIER, SASWRITECONSTANT);
    if (result != S7DLL_STATUS_OK) {
#ifdef ZEUS_OS
        std::cout << " SetTimeouts FAILED (error=" << result << ")" << std::endl;
#endif
        S7LITE_DLL_DeInit();
        dllInitialized_ = false;
        return false;
    }

#ifdef ZEUS_OS
    std::cout << " OK" << std::endl;
    std::cout << "  SAS UART configured: " << SASBAUDRATE << " baud, " << SASWORDLENGTH << "-bit mode" << std::endl;
#endif

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
        std::cout << "[SAS UART DEBUG] GetBuffer called " << debugCounter
                  << " times, result=" << result << ", read=" << len << " bytes" << std::endl;
    }

    if (result != S7DLL_STATUS_OK && result != S7DLL_STATUS_ERROR) {
        std::cout << "[SAS UART] GetBuffer error: " << result << std::endl;
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
        std::cout << "[SAS UART] SendBuffer error: " << result << std::endl;
        return -1;
    }

    return 0;
#else
    return -1;
#endif
}

// read - simple direct reading from GetBuffer (build 24 version)
int SASSerialPort::read(uint8_t* buffer, int maxBytes,
                        std::chrono::milliseconds timeout) {
    if (!isOpen_ || !buffer || maxBytes <= 0) {
        return -1;
    }

    auto startTime = std::chrono::steady_clock::now();

    // Allocate temporary 16-bit buffer for S7Lite API
    std::vector<uint16_t> tempBuffer(maxBytes);
    unsigned int bytesRead = 0;

    // Keep trying to read until timeout or we get at least MIN_MSG_SIZE bytes
    constexpr unsigned int MIN_MSG_SIZE = 4;
    while (bytesRead < MIN_MSG_SIZE) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime);
        if (elapsed >= timeout) {
            return bytesRead > 0 ? static_cast<int>(bytesRead) : 0;  // Return what we have or timeout
        }

        // Try to read from hardware
        unsigned int len = maxBytes;
        GetBuffer(tempBuffer.data(), len, len);

        if (len > 0) {
            bytesRead = len;
            break;  // Got data, stop polling
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // Convert 16-bit words to bytes
    for (unsigned int i = 0; i < bytesRead && i < static_cast<unsigned int>(maxBytes); i++) {
        buffer[i] = static_cast<uint8_t>(tempBuffer[i] & 0xFF);
    }

#ifdef ZEUS_OS
    std::cout << "[UART" << SASUART << " RX] Read " << bytesRead << " bytes: ";
    for (unsigned int i = 0; i < bytesRead && i < 16; i++) {
        printf("%02X ", buffer[i]);
    }
    if (bytesRead > 16) std::cout << "...";
    std::cout << std::endl;
#endif

    return static_cast<int>(bytesRead);
}

// write - based on master's WriteSAS implementation
int SASSerialPort::write(const uint8_t* buffer, int numBytes) {
    if (!isOpen_ || !buffer || numBytes <= 0) {
        return -1;
    }

    // Convert byte buffer to uint16_t buffer for S7Lite API
    // Based on master's WriteSAS implementation
    std::vector<uint16_t> wBuffer(numBytes);

    // First byte gets mark parity (0xFF00), rest get space parity (0x0000)
    // This is how the master marks the address byte in SAS protocol
    wBuffer[0] = static_cast<uint16_t>(buffer[0] | SER9BIT_MARK);

    for (int i = 1; i < numBytes; i++) {
        wBuffer[i] = static_cast<uint16_t>(buffer[i] | SER9BIT_NOMARK);
    }

#ifdef ZEUS_OS
    std::cout << "[UART" << SASUART << " TX] Sending " << numBytes << " bytes: ";
    for (int i = 0; i < numBytes && i < 16; i++) {
        printf("%02X ", buffer[i]);
    }
    std::cout << std::endl;
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
