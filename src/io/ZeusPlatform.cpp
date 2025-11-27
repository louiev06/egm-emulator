#include "megamic/io/ZeusPlatform.h"
#include "megamic/io/ZeusSerialPort.h"
#include <cstring>
#include <sstream>

// Include Zeus OS / Axiomtek S7Lite API header
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
typedef unsigned char UCHAR;
typedef unsigned char BOOLEAN;
typedef unsigned char* PUCHAR;

#define S7DLL_STATUS_OK 0
#define S7DLL_STATUS_ERROR -1
#define TRUE 1
#define FALSE 0
#define SRAM_ACCESS_MAX 256
#define SRAM_ACCESS_BOLCKS 4

#pragma pack(push, 1)
typedef struct {
    PUCHAR buffer;
    UINT offset;
    UINT length;
} S7LITE_SRAMACCESSBLOCK;

typedef struct {
    S7LITE_SRAMACCESSBLOCK block[SRAM_ACCESS_BOLCKS];
} S7LITE_SRAMACCESS;
#pragma pack(pop)

// Stub functions for non-Zeus builds
inline S7_Result S7LITE_DLL_Init(void) { return S7DLL_STATUS_ERROR; }
inline S7_Result S7LITE_DLL_DeInit(void) { return S7DLL_STATUS_OK; }
inline S7_Result S7LITE_DLL_GetDLLVersion(BYTE* pVersion) {
    if (pVersion) { pVersion[0] = 1; pVersion[1] = 0; pVersion[2] = 0; }
    return S7DLL_STATUS_OK;
}
inline S7_Result S7LITE_Watchdog_Enable(void) { return S7DLL_STATUS_OK; }
inline S7_Result S7LITE_Watchdog_SetTimeout(UINT time) { (void)time; return S7DLL_STATUS_OK; }
inline S7_Result S7LITE_Watchdog_Kick(void) { return S7DLL_STATUS_OK; }
inline S7_Result S7LITE_SRAM_Size(UINT* pSize) { if (pSize) *pSize = 0; return S7DLL_STATUS_ERROR; }
inline S7_Result S7LITE_SRAM_Read(S7LITE_SRAMACCESSBLOCK access, void (*fn)(PVOID), PVOID context) {
    (void)access; (void)fn; (void)context; return S7DLL_STATUS_ERROR;
}
inline S7_Result S7LITE_SRAM_Write(S7LITE_SRAMACCESS access, void (*fn)(PVOID), PVOID context) {
    (void)access; (void)fn; (void)context; return S7DLL_STATUS_ERROR;
}
inline S7_Result S7LITE_Firmware_Version(char* version, size_t* size) {
    (void)version; (void)size; return S7DLL_STATUS_ERROR;
}
inline S7_Result S7LITE_Battery_GetStatus(BOOLEAN* pStatus) {
    if (pStatus) *pStatus = TRUE; return S7DLL_STATUS_OK;
}
inline S7_Result S7LITE_Battery_GetVoltage(USHORT* voltage) {
    if (voltage) *voltage = 3300; return S7DLL_STATUS_OK;
}
inline S7_Result S7LITE_RemoteLCD_SetBacklightPWM(UINT brightness) {
    (void)brightness; return S7DLL_STATUS_OK;
}
#endif

namespace megamic {

ZeusPlatform::ZeusPlatform(bool enableWatchdog, uint32_t watchdogTimeout)
    : initialized_(false),
      watchdogEnabled_(enableWatchdog),
      watchdogTimeout_(watchdogTimeout),
      sramSize_(0),
      sasPort_(nullptr) {

    // Initialize LED state cache
    for (int i = 0; i < 4; i++) {
        currentLEDState_[i] = -1;  // Unknown state
    }
}

ZeusPlatform::~ZeusPlatform() {
    shutdown();
}

bool ZeusPlatform::initialize() {
    if (initialized_) {
        return true;  // Already initialized
    }

    // Initialize Zeus S7Lite DLL
    // Note: This is also called by ZeusSerialPort, but it's safe to call multiple times
    S7_Result result = S7LITE_DLL_Init();
    if (result != S7DLL_STATUS_OK) {
        return false;
    }

    // Query SRAM size
    if (!querySRAMSize()) {
        S7LITE_DLL_DeInit();
        return false;
    }

    // Initialize watchdog if enabled
    if (watchdogEnabled_) {
        if (!initializeWatchdog()) {
            // Watchdog init failed, but continue anyway (non-critical)
        }
    }

    initialized_ = true;
    return true;
}

void ZeusPlatform::shutdown() {
    if (!initialized_) {
        return;
    }

    // Close SAS port if cached
    if (sasPort_) {
        sasPort_->close();
        sasPort_.reset();
    }

    // Deinitialize Zeus DLL
    // Note: Only call if we're the ones who initialized it
    S7LITE_DLL_DeInit();

    initialized_ = false;
}

std::shared_ptr<io::CommChannel> ZeusPlatform::createSASPort() {
    // Return cached instance if available
    if (sasPort_ && sasPort_->isOpen()) {
        return sasPort_;
    }

    // Create new Zeus serial port
    sasPort_ = std::make_shared<io::ZeusSerialPort>("SAS");

    // Open the port
    if (!sasPort_->open()) {
        sasPort_.reset();
        return nullptr;
    }

    return sasPort_;
}

void ZeusPlatform::setLED(int ledId, bool state) {
    if (ledId < 0 || ledId >= 4) {
        return;  // Invalid LED ID
    }

    // Check if state changed (optimization)
    int stateValue = state ? 1 : 0;
    if (currentLEDState_[ledId] == stateValue) {
        return;  // No change
    }

    // TODO: Implement LED control via Zeus OS API
    // The S7Lite API doesn't expose LED control directly
    // This would typically be done through GPIO or a custom API extension

    // Update cache
    currentLEDState_[ledId] = stateValue;
}

std::string ZeusPlatform::getPlatformInfo() const {
    std::ostringstream oss;

    oss << "Zeus OS / Axiomtek S7 Lite";

    // Add firmware version
    std::string fwVer = getFirmwareVersion();
    if (!fwVer.empty()) {
        oss << " (FW: " << fwVer << ")";
    }

    // Add library version
    uint8_t major = 0, minor = 0, patch = 0;
    if (getLibraryVersion(major, minor, patch)) {
        oss << " [Lib: " << static_cast<int>(major) << "."
            << static_cast<int>(minor) << "."
            << static_cast<int>(patch) << "]";
    }

    // Add SRAM size
    if (sramSize_ > 0) {
        oss << " - SRAM: " << sramSize_ << " bytes";
    }

    return oss.str();
}

void ZeusPlatform::kickWatchdog() {
    if (!watchdogEnabled_) {
        return;
    }

    S7LITE_Watchdog_Kick();
}

uint32_t ZeusPlatform::getSRAMSize() const {
    return sramSize_;
}

bool ZeusPlatform::readSRAM(uint32_t offset, uint8_t* buffer, uint32_t length) {
    if (!initialized_ || buffer == nullptr || length == 0) {
        return false;
    }

    // Check bounds
    if ((offset + length) * sizeof(USHORT) > sramSize_) {
        return false;
    }

    // Prepare S7Lite SRAM access structure
    S7LITE_SRAMACCESSBLOCK access;
    access.buffer = buffer;
    access.offset = offset;  // In WORD units
    access.length = length;  // In WORD units

    // Call Zeus API
    S7_Result result = S7LITE_SRAM_Read(access, nullptr, nullptr);

    return (result == S7DLL_STATUS_OK);
}

bool ZeusPlatform::writeSRAM(uint32_t offset, const uint8_t* buffer, uint32_t length) {
    if (!initialized_ || buffer == nullptr || length == 0) {
        return false;
    }

    // Check bounds
    if ((offset + length) * sizeof(USHORT) > sramSize_) {
        return false;
    }

    // Zeus API requires non-const buffer pointer
    uint8_t* nonConstBuffer = const_cast<uint8_t*>(buffer);

    // Prepare S7Lite SRAM access structure
    S7LITE_SRAMACCESS access;
    memset(&access, 0, sizeof(access));

    // Fill first block (Zeus API supports up to 4 blocks)
    access.block[0].buffer = nonConstBuffer;
    access.block[0].offset = offset;  // In WORD units
    access.block[0].length = length;  // In WORD units

    // Call Zeus API
    S7_Result result = S7LITE_SRAM_Write(access, nullptr, nullptr);

    return (result == S7DLL_STATUS_OK);
}

std::string ZeusPlatform::getFirmwareVersion() const {
    if (!initialized_) {
        return "";
    }

    char version[64];
    size_t size = sizeof(version);

    S7_Result result = S7LITE_Firmware_Version(version, &size);
    if (result != S7DLL_STATUS_OK) {
        return "";
    }

    return std::string(version);
}

bool ZeusPlatform::getLibraryVersion(uint8_t& major, uint8_t& minor, uint8_t& patch) const {
    BYTE version[3];

    S7_Result result = S7LITE_DLL_GetDLLVersion(version);
    if (result != S7DLL_STATUS_OK) {
        return false;
    }

    major = version[0];
    minor = version[1];
    patch = version[2];

    return true;
}

bool ZeusPlatform::getBatteryStatus() const {
    if (!initialized_) {
        return false;
    }

    BOOLEAN status = FALSE;
    S7_Result result = S7LITE_Battery_GetStatus(&status);

    if (result != S7DLL_STATUS_OK) {
        return false;
    }

    return (status == TRUE);
}

uint16_t ZeusPlatform::getBatteryVoltage() const {
    if (!initialized_) {
        return 0;
    }

    USHORT voltage = 0;
    S7_Result result = S7LITE_Battery_GetVoltage(&voltage);

    if (result != S7DLL_STATUS_OK) {
        return 0;
    }

    return voltage;
}

bool ZeusPlatform::setBacklightBrightness(uint32_t brightness) {
    if (!initialized_) {
        return false;
    }

    // Clamp to valid range (0-1023)
    if (brightness > 1023) {
        brightness = 1023;
    }

    S7_Result result = S7LITE_RemoteLCD_SetBacklightPWM(brightness);

    return (result == S7DLL_STATUS_OK);
}

bool ZeusPlatform::initializeWatchdog() {
    // Enable watchdog
    S7_Result result = S7LITE_Watchdog_Enable();
    if (result != S7DLL_STATUS_OK) {
        return false;
    }

    // Set timeout
    result = S7LITE_Watchdog_SetTimeout(watchdogTimeout_);
    if (result != S7DLL_STATUS_OK) {
        return false;
    }

    return true;
}

bool ZeusPlatform::querySRAMSize() {
    UINT size = 0;
    S7_Result result = S7LITE_SRAM_Size(&size);

    if (result != S7DLL_STATUS_OK) {
        sramSize_ = 0;
        return false;
    }

    sramSize_ = size;
    return true;
}

} // namespace megamic
