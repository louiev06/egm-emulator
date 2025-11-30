#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <vector>
#include <cstdint>

namespace utils {

/**
 * Logger utility for consistent timestamped logging across the application
 */
class Logger {
public:
    /**
     * Get timestamp string in milliseconds since epoch
     */
    static std::string getTimestamp() {
        auto now = std::chrono::steady_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();
        return "[" + std::to_string(ms) + "] ";
    }

    /**
     * Log a message with timestamp
     */
    static void log(const std::string& message) {
        std::cout << getTimestamp() << message << std::endl;
        std::cout << std::flush;
    }

    /**
     * Log without timestamp or newline (for building multi-part messages)
     */
    static void logPart(const std::string& message) {
        std::cout << message << std::endl;
        std::cout << std::flush;
    }

    /**
     * Log hex data with timestamp
     */
    static void logHex(const std::string& prefix, const uint8_t* data, size_t length, size_t bytesPerLine = 16) {
        std::string ts = getTimestamp();
        std::cout << ts << prefix;

        for (size_t i = 0; i < length; i++) {
            printf("%02X ", data[i]);
            if ((i + 1) % bytesPerLine == 0 && (i + 1) < length) {
                std::cout << std::endl << ts << std::string(prefix.length(), ' ');
            }
        }
        std::cout << std::endl;
        std::cout << std::flush;
    }

    /**
     * Log hex data from vector with timestamp
     */
    static void logHexVector(const std::string& prefix, const std::vector<uint8_t>& data, size_t bytesPerLine = 16) {
        if (!data.empty()) {
            logHex(prefix, data.data(), data.size(), bytesPerLine);
        }
    }
};

} // namespace utils

#endif // LOGGER_H
