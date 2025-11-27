#include "ICardPlatform.h"
#include "io/CommChannel.h"
#include <sstream>



SimulatedPlatform::SimulatedPlatform()
    : portCounter_(0) {
}

std::shared_ptr<io::CommChannel> SimulatedPlatform::createSASPort() {
    std::ostringstream oss;
    oss << "SAS_PORT_" << portCounter_++;
    return std::make_shared<io::PipedCommChannel>(oss.str());
}

void SimulatedPlatform::setLED(int ledId, bool state) {
    // Simulated - no-op
    (void)ledId;
    (void)state;
}

std::string SimulatedPlatform::getPlatformInfo() const {
    return "Simulated Platform v1.0 (C++)";
}


