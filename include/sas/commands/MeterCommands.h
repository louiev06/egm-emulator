#ifndef SAS_COMMANDS_METERCOMMANDS_H
#define SAS_COMMANDS_METERCOMMANDS_H

#include "sas/SASCommands.h"
#include "simulator/Machine.h"
#include <vector>


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
     * Handle "Send Total Coin In and Associated Meters" (0x19)
     * Returns 5 meters: CoinIn, CoinOut, TotalDrop, Jackpot, GamesPlayed
     * @param machine Machine instance
     * @return Response with 24 bytes total: [Addr][Cmd][20 bytes meter data][CRC]
     */
    static Message handleSendTotalCoinInAndMeters(simulator::Machine* machine);

    /**
     * Handle "Send Total Bills" (0x20)
     * Returns bill drop meter (total dollar bills accepted)
     * @param machine Machine instance
     * @return Response with 8 bytes total: [Addr][Cmd][4 bytes BCD][CRC]
     */
    static Message handleSendTotalBills(simulator::Machine* machine);

    /**
     * Handle "Send Selected Meters" (0x19 - DEPRECATED, use 0x19 for fixed format)
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

    // Phase 1 - Basic Meters (Simple 4-byte BCD responses)

    /**
     * Handle "Send Cancelled Credits Meter" (0x10)
     * @param machine Machine instance
     * @return Response with 8 bytes total: [Addr][Cmd][4 bytes BCD][CRC]
     */
    static Message handleSendCancelledCredits(simulator::Machine* machine);

    /**
     * Handle "Send Current Credits" (0x1A)
     * @param machine Machine instance
     * @return Response with 8 bytes total: [Addr][Cmd][4 bytes BCD][CRC]
     */
    static Message handleSendCurrentCredits(simulator::Machine* machine);

    /**
     * Handle "Send True Coin In" (0x2A)
     * @param machine Machine instance
     * @return Response with 8 bytes total: [Addr][Cmd][4 bytes BCD][CRC]
     */
    static Message handleSendTrueCoinIn(simulator::Machine* machine);

    /**
     * Handle "Send True Coin Out" (0x2B)
     * @param machine Machine instance
     * @return Response with 8 bytes total: [Addr][Cmd][4 bytes BCD][CRC]
     */
    static Message handleSendTrueCoinOut(simulator::Machine* machine);

    /**
     * Handle "Send Bills Accepted Credits" (0x46)
     * @param machine Machine instance
     * @return Response with 8 bytes total: [Addr][Cmd][4 bytes BCD][CRC]
     */
    static Message handleSendBillsAcceptedCredits(simulator::Machine* machine);

    // Bill Denomination Meters

    /**
     * Handle "Send $1 Bills Meter" (0x31)
     */
    static Message handleSend$1Bills(simulator::Machine* machine);

    /**
     * Handle "Send $2 Bills Meter" (0x32)
     */
    static Message handleSend$2Bills(simulator::Machine* machine);

    /**
     * Handle "Send $5 Bills Meter" (0x33)
     */
    static Message handleSend$5Bills(simulator::Machine* machine);

    /**
     * Handle "Send $10 Bills Meter" (0x34)
     */
    static Message handleSend$10Bills(simulator::Machine* machine);

    /**
     * Handle "Send $20 Bills Meter" (0x35)
     */
    static Message handleSend$20Bills(simulator::Machine* machine);

    /**
     * Handle "Send $50 Bills Meter" (0x36)
     */
    static Message handleSend$50Bills(simulator::Machine* machine);

    /**
     * Handle "Send $100 Bills Meter" (0x37)
     */
    static Message handleSend$100Bills(simulator::Machine* machine);

    /**
     * Handle "Send $500 Bills Meter" (0x38)
     */
    static Message handleSend$500Bills(simulator::Machine* machine);

    /**
     * Handle "Send $1000 Bills Meter" (0x39)
     */
    static Message handleSend$1000Bills(simulator::Machine* machine);

    /**
     * Handle "Send $200 Bills Meter" (0x3A)
     */
    static Message handleSend$200Bills(simulator::Machine* machine);

    /**
     * Handle "Send Bill Meters" (0x1E)
     * Returns all 6 common bill denominations: $1, $5, $10, $20, $50, $100
     * @param machine Machine instance
     * @return Response with 28 bytes total: [Addr][Cmd][24 bytes BCD (6x4)][CRC]
     */
    static Message handleSendBillMeters(simulator::Machine* machine);

    /**
     * Handle "Send Gaming Machine Meters 1-8" (0x1C)
     * Returns 8 meters: CoinIn, CoinOut, TotalDrop, Jackpot, GamesPlayed, GamesWon, SlotDoor, PowerReset
     * @param machine Machine instance
     * @return Response with 36 bytes total: [Addr][Cmd][32 bytes BCD (8x4)][CRC]
     */
    static Message handleSendGamingMachineMeters(simulator::Machine* machine);

    /**
     * Handle "Send Selected Game Meters" (0x52)
     * Send: [Addr][0x52][Game Number (2 BCD)][CRC]
     * Response: [Addr][0x52][Game Number (2)][Coin In (4)][Coin Out (4)][Jackpot (4)][Games Played (4)][CRC]
     * Total: 20 bytes
     *
     * Game 0 = EGM (main game), uses gCI, gCO, gJP, gGS
     * Game 1+ = Sub-games, uses SUBGAME_METER_* constants
     *
     * @param machine Machine instance
     * @param data Input data containing game number (2 BCD bytes)
     * @return Response with game-specific meters
     */
    static Message handleSendSelectedGameMeters(simulator::Machine* machine, const std::vector<uint8_t>& data);

    /**
     * Handle "Send Selected Meters for Game N" (0x2F)
     * Send: [Addr][0x2F][Game Number (2 BCD)][Meter Code 1]...[Meter Code N][CRC]
     * Response: [Addr][0x2F][Length][Game Number (2)][Code1][Value1(4 or 5)]...[CRC]
     *
     * Variable length response with requested meter codes and values
     * Most meters = 4 bytes BCD, some TITO meters = 5 bytes BCD
     *
     * @param machine Machine instance
     * @param data Input data containing game number (2 BCD) and meter codes
     * @return Response with requested meters for specified game
     */
    static Message handleSendSelectedMetersForGameN(simulator::Machine* machine, const std::vector<uint8_t>& data);

    /**
     * Handle "Send Selected Game Number and Handpay Cancelled Credits" (0x2D)
     * Send: [Addr][0x2D][Game Number (2 BCD)][CRC]
     * Response: [Addr][0x2D][Cancelled Credits (4 BCD)][CRC]
     * Total: 10 bytes
     *
     * @param machine Machine instance
     * @param data Input data containing game number (2 BCD bytes)
     * @return Response with handpay cancelled credits
     */
    static Message handleSendHandpayCancelledCredits(simulator::Machine* machine, const std::vector<uint8_t>& data);

    /**
     * Handle "Send Selected Meters for Game N" (0x6F/0xAF)
     * Send: [Addr][0x6F/AF][Length][Game Number (2 BCD)][Meter Code 1 (2)]...[CRC]
     * Response: [Addr][0x6F/AF][Length][Game Number (2 BCD)][Code1(2)][Size1(1)][Value1(4-5 BCD)]...[CRC]
     *
     * Variable length response with meter codes, sizes, and values
     * Supports up to 12 meters per request
     * Alternates between 0x6F and 0xAF to detect stale responses
     *
     * @param machine Machine instance
     * @param command Command code (0x6F or 0xAF)
     * @param data Input data containing length, game number (2 BCD), and meter codes (2 bytes each)
     * @return Response with requested meters and their sizes
     */
    static Message handleSendSelectedMetersForGameNExtended(simulator::Machine* machine,
                                                            uint8_t command,
                                                            const std::vector<uint8_t>& data);

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


#endif // SAS_COMMANDS_METERCOMMANDS_H
