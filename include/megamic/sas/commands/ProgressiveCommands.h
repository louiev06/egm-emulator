#ifndef MEGAMIC_SAS_COMMANDS_PROGRESSIVECOMMANDS_H
#define MEGAMIC_SAS_COMMANDS_PROGRESSIVECOMMANDS_H

#include "megamic/sas/SASCommands.h"
#include "megamic/simulator/Machine.h"
#include <vector>
#include <cstdint>

namespace megamic {
namespace sas {
namespace commands {

/**
 * ProgressiveCommands - Handler for Progressive Jackpot commands
 *
 * Implements progressive jackpot reporting and management:
 * - 0x51: Send Current Progressive Amount
 * - 0x52: Send Progressive Win Amount
 * - 0x53: Send Progressive Levels (multi-level progressives)
 * - 0x5A: Send Progressive Broadcast Values
 */
class ProgressiveCommands {
public:
    /**
     * Progressive level definition
     */
    struct ProgressiveLevel {
        uint8_t levelId;           // Level ID (1-32)
        uint64_t currentAmount;    // Current amount in cents
        uint64_t resetAmount;      // Reset/starting amount
        uint64_t incrementRate;    // Increment rate (per bet)
        bool hasWin;               // True if level has been hit
    };

    /**
     * Handle "Send Current Progressive Amount" (0x51)
     * Returns the current amount for a specific progressive level
     * @param machine Machine instance
     * @param data Level group ID
     * @return Response with progressive amount
     */
    static Message handleSendProgressiveAmount(simulator::Machine* machine,
                                               const std::vector<uint8_t>& data);

    /**
     * Handle "Send Progressive Win Amount" (0x52)
     * Reports when a progressive jackpot has been won
     * @param machine Machine instance
     * @param data Level group ID
     * @return Response with win information
     */
    static Message handleSendProgressiveWin(simulator::Machine* machine,
                                           const std::vector<uint8_t>& data);

    /**
     * Handle "Send Progressive Levels" (0x53)
     * Returns all progressive levels and their current values
     * @param machine Machine instance
     * @return Response with all progressive level data
     */
    static Message handleSendProgressiveLevels(simulator::Machine* machine);

    /**
     * Handle "Send Progressive Broadcast Values" (0x5A)
     * Sends multiple progressive values for display
     * @param machine Machine instance
     * @return Response with broadcast progressive data
     */
    static Message handleSendProgressiveBroadcast(simulator::Machine* machine);

    /**
     * Initialize progressive levels for machine
     * Sets up default progressive configuration
     * @param machine Machine instance
     */
    static void initializeProgressives(simulator::Machine* machine);

    /**
     * Increment progressive levels based on bet
     * Called after each game play
     * @param machine Machine instance
     * @param betAmount Bet amount in cents
     */
    static void incrementProgressives(simulator::Machine* machine, uint64_t betAmount);

    /**
     * Award progressive win
     * Resets progressive to base amount and marks as won
     * @param machine Machine instance
     * @param levelId Level ID (1-32)
     * @return Win amount
     */
    static uint64_t awardProgressiveWin(simulator::Machine* machine, uint8_t levelId);

    // Progressive Group IDs
    enum ProgressiveGroup {
        GROUP_1 = 0x01,     // Primary progressive
        GROUP_2 = 0x02,     // Secondary progressive
        GROUP_3 = 0x03,     // Tertiary progressive
        GROUP_WIDE_AREA = 0x80  // Wide-area progressive
    };

private:
    /**
     * Get progressive level by ID
     * @param levelId Level ID (1-32)
     * @return Progressive level, or nullptr if not found
     */
    static ProgressiveLevel* getProgressiveLevel(uint8_t levelId);

    /**
     * Build progressive response
     * @param address SAS address
     * @param command Command code
     * @param levelId Level ID
     * @param amount Progressive amount
     * @return Response message
     */
    static Message buildProgressiveResponse(uint8_t address,
                                            uint8_t command,
                                            uint8_t levelId,
                                            uint64_t amount);
};

} // namespace commands
} // namespace sas
} // namespace megamic

#endif // MEGAMIC_SAS_COMMANDS_PROGRESSIVECOMMANDS_H
