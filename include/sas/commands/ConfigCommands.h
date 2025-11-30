#ifndef SAS_COMMANDS_CONFIGCOMMANDS_H
#define SAS_COMMANDS_CONFIGCOMMANDS_H

#include "sas/SASCommands.h"
#include "simulator/Machine.h"


namespace sas {
namespace commands {

/**
 * Configuration and identification command handlers
 */
class ConfigCommands {
public:
    /**
     * Handle Send Gaming Machine ID and Serial Number (0x54)
     *
     * Response format:
     * [Address][0x54][Length][Version(3)][Serial Number(0-40)][CRC]
     *
     * Length byte = number of bytes following (excluding CRC)
     * Version = 3 bytes (manufacturer-specific version)
     * Serial Number = 0-40 ASCII characters
     *
     * @param machine Pointer to machine simulator
     * @return Response message
     */
    static Message handleSendMachineID(simulator::Machine* machine);

    /**
     * Handle Send Number of Games Implemented (0x51)
     *
     * Response format:
     * [Address][0x51][Number of Games (2 BCD)][CRC]
     * Total: 6 bytes
     *
     * @param machine Pointer to machine simulator
     * @return Response message
     */
    static Message handleSendNumberOfGames(simulator::Machine* machine);

    /**
     * Handle Send Selected Game Number (0x55)
     *
     * Response format:
     * [Address][0x55][Selected Game Number (2 BCD)][CRC]
     * Total: 6 bytes
     *
     * @param machine Pointer to machine simulator
     * @return Response message
     */
    static Message handleSendSelectedGameNumber(simulator::Machine* machine);

    /**
     * Handle Send Game N Configuration (0x53)
     *
     * Input: [Addr][0x53][Game Number (2 BCD)][CRC]
     * Response format (28 bytes total):
     * [Address][0x53][Length][Game Number(2)][Game ID(2)][Additional ID(3)]
     * [Denomination(1)][Max Bet(1)][Progressive Group(1)][Game Options(2)]
     * [Pay Table(6)][Base Percent(4)][CRC(2)]
     *
     * @param machine Pointer to machine simulator
     * @param data Input data containing game number (2 BCD bytes)
     * @return Response message with game configuration
     */
    static Message handleSendGameNConfiguration(simulator::Machine* machine, const std::vector<uint8_t>& data);

    /**
     * Handle Send Enabled Game Numbers (0x56)
     *
     * Response format (variable length):
     * [Address][0x56][Length][Number of Games(1)][Game1(2 BCD)][Game2(2 BCD)]...[CRC(2)]
     *
     * Length byte = Number of bytes following (excluding CRC)
     * Number of Games = count of enabled games
     * Each game number is 2 bytes BCD
     *
     * @param machine Pointer to machine simulator
     * @return Response message with list of enabled game numbers
     */
    static Message handleSendEnabledGameNumbers(simulator::Machine* machine);

    /**
     * Handle Enable/Disable Game N (0xA0)
     *
     * Input: [Addr][0xA0][Game Number (2 BCD)][CRC]
     * Response format (12 bytes total):
     * [Address][0xA0][Game Number(2 BCD)][Gaming Machine Capabilities Flags1][Flags2][Flags3][CRC(2)]
     *
     * Flags1 (byte 4):
     *   Bit 0: Jackpot Multiplier
     *   Bit 1: AFT Bonus Awards
     *   Bit 2: Legacy Bonus Awards
     *   Bit 3: Tournament
     *   Bit 4: Validation Extensions
     *   Bits 5-6: Validation Style (0=None, 1=Secure Enhanced, 2=System)
     *   Bit 7: Ticket Redemption
     *
     * Flags2 (byte 5):
     *   Bits 0-1: Meter Model (0=SAS3, 1=SAS4, 2=SAS5, 3=SAS6)
     *   Bit 2: Tickets to Total Drop
     *   Bit 3: Extended Meters
     *   Bit 4: Component Authentication
     *   Bit 5: Reserved
     *   Bit 6: AFT
     *   Bit 7: Multi-Denom Extensions
     *
     * Flags3 (byte 6):
     *   Bit 0: Maximum Polling Rate (1=40ms)
     *   Bit 1: Multiple SAS Progressive Win Reporting
     *   Bits 2-7: Reserved
     *
     * @param machine Pointer to machine simulator
     * @param data Input data containing game number (2 BCD bytes)
     * @return Response message with gaming machine capabilities
     */
    static Message handleEnableDisableGameN(simulator::Machine* machine, const std::vector<uint8_t>& data);
};

} // namespace commands
} // namespace sas


#endif // SAS_COMMANDS_CONFIGCOMMANDS_H
