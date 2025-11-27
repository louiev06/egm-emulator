#ifndef MEGAMIC_SAS_COMMANDS_METERCOMMANDS_H
#define MEGAMIC_SAS_COMMANDS_METERCOMMANDS_H

#include "megamic/sas/SASCommands.h"
#include "megamic/simulator/Machine.h"
#include <vector>

namespace megamic {
namespace sas {
namespace commands {

/**
 * MeterCommands - Handler for SAS meter query commands
 *
 * Implements meter-related SAS commands:
 * - 0x10-0x1F: Send meters (groups of 10)
 * - 0x19: Send selected meters
 * - 0x1F: Send game configuration and meters
 */
class MeterCommands {
public:
    /**
     * Handle "Send Meters" command (0x10-0x1F)
     * Each command returns 10 meters in BCD format
     *
     * @param machine Machine instance
     * @param command Command code (0x10-0x1F)
     * @return Response message with meter data
     */
    static Message handleSendMeters(simulator::Machine* machine, uint8_t command);

    /**
     * Handle "Send Total Coin In" (0x11)
     * @param machine Machine instance
     * @return Response with 4-byte BCD meter value
     */
    static Message handleSendTotalCoinIn(simulator::Machine* machine);

    /**
     * Handle "Send Total Coin Out" (0x12)
     * @param machine Machine instance
     * @return Response with 4-byte BCD meter value
     */
    static Message handleSendTotalCoinOut(simulator::Machine* machine);

    /**
     * Handle "Send Total Drop" (0x13)
     * @param machine Machine instance
     * @return Response with 4-byte BCD meter value
     */
    static Message handleSendTotalDrop(simulator::Machine* machine);

    /**
     * Handle "Send Total Jackpot" (0x14)
     * @param machine Machine instance
     * @return Response with 4-byte BCD meter value
     */
    static Message handleSendTotalJackpot(simulator::Machine* machine);

    /**
     * Handle "Send Games Played" (0x15)
     * @param machine Machine instance
     * @return Response with 4-byte BCD meter value
     */
    static Message handleSendGamesPlayed(simulator::Machine* machine);

    /**
     * Handle "Send Games Won" (0x16)
     * @param machine Machine instance
     * @return Response with 4-byte BCD meter value
     */
    static Message handleSendGamesWon(simulator::Machine* machine);

    /**
     * Handle "Send Games Lost" (0x17)
     * @param machine Machine instance
     * @return Response with 4-byte BCD meter value
     */
    static Message handleSendGamesLost(simulator::Machine* machine);

    /**
     * Handle "Send Selected Meters" (0x19)
     * @param machine Machine instance
     * @param meterCodes List of meter codes to retrieve
     * @return Response with requested meter data
     */
    static Message handleSendSelectedMeters(simulator::Machine* machine,
                                           const std::vector<uint8_t>& meterCodes);

    /**
     * Handle "Send Game Configuration" (0x1F)
     * Includes denomination and meter information
     * @param machine Machine instance
     * @return Response with game config and meters
     */
    static Message handleSendGameConfiguration(simulator::Machine* machine);

private:
    /**
     * Build meter response message
     * @param address SAS address
     * @param command Command code
     * @param meterValue Meter value (binary)
     * @return Complete message with BCD-encoded meter
     */
    static Message buildMeterResponse(uint8_t address, uint8_t command,
                                      uint64_t meterValue);

    /**
     * Build multi-meter response
     * @param address SAS address
     * @param command Command code
     * @param meterValues List of meter values
     * @return Complete message with all meters
     */
    static Message buildMultiMeterResponse(uint8_t address, uint8_t command,
                                          const std::vector<uint64_t>& meterValues);
};

} // namespace commands
} // namespace sas
} // namespace megamic

#endif // MEGAMIC_SAS_COMMANDS_METERCOMMANDS_H
