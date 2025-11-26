#include "megamic/sas/commands/DateTimeCommands.h"
#include "megamic/sas/BCD.h"
#include <ctime>

namespace megamic {
namespace sas {
namespace commands {

Message DateTimeCommands::handleSendDateTime(simulator::Machine* machine) {
    if (!machine) {
        return Message();
    }

    Message response;
    response.address = 1;
    response.command = LongPoll::SEND_DATE_TIME;

    // Get current date/time in BCD format
    std::vector<uint8_t> dateTimeBCD = getCurrentDateTimeBCD();
    response.data = dateTimeBCD;

    return response;
}

Message DateTimeCommands::handleSetDateTime(simulator::Machine* machine,
                                           const std::vector<uint8_t>& data) {
    if (!machine) {
        return Message();
    }

    // Most gaming machines don't allow setting date/time via SAS
    // They sync with system clock or RTC
    // Return ACK anyway for compatibility

    Message response;
    response.address = 1;
    response.command = LongPoll::SET_DATE_TIME;
    // No data - just ACK

    return response;
}

std::vector<uint8_t> DateTimeCommands::getCurrentDateTimeBCD() {
    std::vector<uint8_t> result;

    // Get current system time
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);

    if (!timeinfo) {
        // Error - return zeros
        return std::vector<uint8_t>(9, 0);
    }

    // SAS Date/Time format (9 bytes total):
    // Byte 0-1: Month (01-12) BCD
    // Byte 2-3: Day (01-31) BCD
    // Byte 4-7: Year (YYYY) BCD
    // Byte 8: Hour (00-23) BCD
    // Byte 9: Minute (00-59) BCD
    // Byte 10: Second (00-59) BCD

    // Month (01-12)
    uint8_t month = timeinfo->tm_mon + 1;  // tm_mon is 0-11
    result.push_back(BCD::toBCD(month));

    // Day (01-31)
    uint8_t day = timeinfo->tm_mday;
    result.push_back(BCD::toBCD(day));

    // Year (YYYY) - 2 bytes BCD
    int year = timeinfo->tm_year + 1900;  // tm_year is years since 1900
    std::vector<uint8_t> yearBCD = BCD::encode(year, 2);
    result.insert(result.end(), yearBCD.begin(), yearBCD.end());

    // Hour (00-23)
    uint8_t hour = timeinfo->tm_hour;
    result.push_back(BCD::toBCD(hour));

    // Minute (00-59)
    uint8_t minute = timeinfo->tm_min;
    result.push_back(BCD::toBCD(minute));

    // Second (00-59)
    uint8_t second = timeinfo->tm_sec;
    result.push_back(BCD::toBCD(second));

    return result;
}

} // namespace commands
} // namespace sas
} // namespace megamic
