#ifndef SAS_COMMANDS_DATETIMECOMMANDS_H
#define SAS_COMMANDS_DATETIMECOMMANDS_H

#include "sas/SASCommands.h"
#include "simulator/Machine.h"


namespace sas {
namespace commands {

/**
 * DateTimeCommands - Handler for SAS date/time commands
 *
 * Implements:
 * - 0x1B: Send Date and Time
 * - 0x20: Set Date and Time (not typically implemented on game)
 */
class DateTimeCommands {
public:
    /**
     * Handle "Send Date and Time" (0x1B)
     * Returns current machine date/time in BCD format
     * @param machine Machine instance
     * @return Response with date/time data
     */
    static Message handleSendDateTime(simulator::Machine* machine);

    /**
     * Handle "Set Date and Time" (0x20)
     * Updates machine clock (if supported)
     * @param machine Machine instance
     * @param data Date/time data from host
     * @return ACK response
     */
    static Message handleSetDateTime(simulator::Machine* machine,
                                    const std::vector<uint8_t>& data);

private:
    /**
     * Get current system time in SAS BCD format
     * Format: MMDDYYYY HHMMSS (6 bytes date + 3 bytes time)
     * @return Vector of BCD-encoded date/time bytes
     */
    static std::vector<uint8_t> getCurrentDateTimeBCD();
};

} // namespace commands
} // namespace sas


#endif // SAS_COMMANDS_DATETIMECOMMANDS_H
