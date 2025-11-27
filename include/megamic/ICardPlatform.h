#ifndef MEGAMIC_ICARDPLATFORM_H
#define MEGAMIC_ICARDPLATFORM_H

#include <memory>
#include <string>

namespace megamic {

// Forward declarations
namespace io {
class CommChannel;
}

/**
 * Platform abstraction layer for hardware-specific functionality
 * This is the C++ port of ICardPlatform.java
 */
class ICardPlatform {
public:
    virtual ~ICardPlatform() = default;

    /**
     * Create a serial port for SAS communication
     * @return CommChannel for SAS protocol
     */
    virtual std::shared_ptr<io::CommChannel> createSASPort() = 0;

    /**
     * Set LED state
     * @param ledId LED identifier
     * @param state true = on, false = off
     */
    virtual void setLED(int ledId, bool state) = 0;

    /**
     * Get platform name/version
     */
    virtual std::string getPlatformInfo() const = 0;
};

/**
 * Simulated platform for testing/development
 */
class SimulatedPlatform : public ICardPlatform {
public:
    SimulatedPlatform();
    ~SimulatedPlatform() override = default;

    std::shared_ptr<io::CommChannel> createSASPort() override;
    void setLED(int ledId, bool state) override;
    std::string getPlatformInfo() const override;

private:
    int portCounter_;
};

} // namespace megamic

#endif // MEGAMIC_ICARDPLATFORM_H
