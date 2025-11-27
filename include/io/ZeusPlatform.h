#ifndef ZEUSPLATFORM_H
#define ZEUSPLATFORM_H

#include "ICardPlatform.h"
#include <string>



/**
 * ZeusPlatform - Hardware platform implementation for Zeus OS / Axiomtek S7 Lite
 *
 * This platform provides integration with the Axiomtek S7 Lite gaming platform
 * running Zeus OS. It wraps the S7LITE API for:
 * - SAS serial communication (hardware UART)
 * - LED control (status indicators)
 * - Watchdog timer
 * - SRAM access (non-volatile storage)
 * - RTC (real-time clock)
 * - Battery monitoring
 *
 * The Axiomtek S7 Lite hardware includes:
 * - Pre-configured SAS serial port (19200 baud, 9-bit)
 * - On-board SRAM for game state persistence
 * - Hardware watchdog for system reliability
 * - LED indicators for status
 * - Battery backup for SRAM
 */
class ZeusPlatform : public ICardPlatform {
public:
    /**
     * Constructor
     * @param enableWatchdog Enable hardware watchdog timer (default: true)
     * @param watchdogTimeout Watchdog timeout in seconds (default: 30)
     */
    explicit ZeusPlatform(bool enableWatchdog = true, uint32_t watchdogTimeout = 30);

    /**
     * Destructor - ensures proper cleanup
     */
    ~ZeusPlatform() override;

    // ICardPlatform interface implementation
    std::shared_ptr<io::CommChannel> createSASPort() override;
    void setLED(int ledId, bool state) override;
    std::string getPlatformInfo() const override;

    /**
     * Initialize the Zeus platform
     * Must be called before using any other methods
     * @return true if initialization successful
     */
    bool initialize();

    /**
     * Shutdown the Zeus platform
     */
    void shutdown();

    /**
     * Kick the watchdog timer to prevent system reset
     * Should be called periodically (< watchdog timeout)
     */
    void kickWatchdog();

    /**
     * Get SRAM size available for game state storage
     * @return SRAM size in bytes
     */
    uint32_t getSRAMSize() const;

    /**
     * Read data from SRAM (non-volatile storage)
     * @param offset Offset in SRAM (in words, 16-bit units)
     * @param buffer Buffer to read into
     * @param length Number of words to read
     * @return true if successful
     */
    bool readSRAM(uint32_t offset, uint8_t* buffer, uint32_t length);

    /**
     * Write data to SRAM (non-volatile storage)
     * @param offset Offset in SRAM (in words, 16-bit units)
     * @param buffer Buffer to write from
     * @param length Number of words to write
     * @return true if successful
     */
    bool writeSRAM(uint32_t offset, const uint8_t* buffer, uint32_t length);

    /**
     * Get firmware version string
     * @return Firmware version (e.g., "PRODUCT.MAJOR.MINOR.BUILD")
     */
    std::string getFirmwareVersion() const;

    /**
     * Get DLL library version
     * @param major Major version number
     * @param minor Minor version number
     * @param patch Patch version number
     * @return true if successful
     */
    bool getLibraryVersion(uint8_t& major, uint8_t& minor, uint8_t& patch) const;

    /**
     * Get battery status
     * @return true if battery is good, false if battery is low/bad
     */
    bool getBatteryStatus() const;

    /**
     * Get battery voltage in millivolts
     * @return Battery voltage in mV (0 if error)
     */
    uint16_t getBatteryVoltage() const;

    /**
     * Set LCD backlight brightness
     * @param brightness Brightness level (0-1023)
     * @return true if successful
     */
    bool setBacklightBrightness(uint32_t brightness);

private:
    bool initialized_;
    bool watchdogEnabled_;
    uint32_t watchdogTimeout_;
    uint32_t sramSize_;
    std::shared_ptr<io::CommChannel> sasPort_;  // Cached SAS port instance

    // LED state cache (for optimization)
    int currentLEDState_[4];  // Assume up to 4 LEDs

    /**
     * Initialize watchdog timer
     */
    bool initializeWatchdog();

    /**
     * Query SRAM size from hardware
     */
    bool querySRAMSize();
};



#endif // ZEUSPLATFORM_H
