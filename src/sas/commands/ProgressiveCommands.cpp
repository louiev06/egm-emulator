#include "sas/commands/ProgressiveCommands.h"
#include "sas/BCD.h"
#include "sas/SASConstants.h"
#include <map>
#include <algorithm>


namespace sas {
namespace commands {

// Static storage for progressive levels
static std::map<uint8_t, ProgressiveCommands::ProgressiveLevel> progressiveLevels;
static bool progressivesInitialized = false;

Message ProgressiveCommands::handleSendProgressiveAmount(simulator::Machine* machine,
                                                         const std::vector<uint8_t>& data) {
    if (!machine) {
        return Message();
    }

    // Initialize progressives if needed
    if (!progressivesInitialized) {
        initializeProgressives(machine);
    }

    Message response;
    response.address = 1;
    response.command = LongPoll::SEND_PROGRESSIVE_AMOUNT;

    // Extract level group ID (first byte of data)
    uint8_t groupId = (data.size() > 0) ? data[0] : GROUP_1;

    // For simplicity, map group to level ID
    uint8_t levelId = groupId;

    // Get progressive level
    ProgressiveLevel* level = getProgressiveLevel(levelId);
    uint64_t amount = (level != nullptr) ? level->currentAmount : 0;

    // Response format:
    // Byte 0: Group ID
    // Bytes 1-4: Progressive amount (4 bytes BCD)
    response.data.push_back(groupId);

    std::vector<uint8_t> amountBCD = BCD::encode(amount, 4);
    response.data.insert(response.data.end(), amountBCD.begin(), amountBCD.end());

    return response;
}

Message ProgressiveCommands::handleSendProgressiveWin(simulator::Machine* machine,
                                                      const std::vector<uint8_t>& data) {
    if (!machine) {
        return Message();
    }

    // Initialize progressives if needed
    if (!progressivesInitialized) {
        initializeProgressives(machine);
    }

    Message response;
    response.address = 1;
    response.command = LongPoll::SEND_PROGRESSIVE_WIN;

    // Extract level group ID
    uint8_t groupId = (data.size() > 0) ? data[0] : GROUP_1;
    uint8_t levelId = groupId;

    // Get progressive level
    ProgressiveLevel* level = getProgressiveLevel(levelId);

    if (level != nullptr && level->hasWin) {
        // Response includes win amount
        response.data.push_back(groupId);

        // Win amount (5 bytes BCD for larger jackpots)
        std::vector<uint8_t> amountBCD = BCD::encode(level->currentAmount, 5);
        response.data.insert(response.data.end(), amountBCD.begin(), amountBCD.end());

        // Clear win flag (will be reset when acknowledged)
        level->hasWin = false;

        // Reset progressive to base amount
        level->currentAmount = level->resetAmount;
    } else {
        // No win - return zeros
        response.data.push_back(groupId);
        std::vector<uint8_t> zeroBCD = BCD::encode(0, 5);
        response.data.insert(response.data.end(), zeroBCD.begin(), zeroBCD.end());
    }

    return response;
}

Message ProgressiveCommands::handleSendProgressiveLevels(simulator::Machine* machine) {
    if (!machine) {
        return Message();
    }

    // Initialize progressives if needed
    if (!progressivesInitialized) {
        initializeProgressives(machine);
    }

    Message response;
    response.address = 1;
    response.command = LongPoll::SEND_PROGRESSIVE_LEVELS;

    // Response format:
    // Byte 0: Number of levels
    // For each level:
    //   Byte N: Level ID
    //   Bytes N+1 to N+4: Current amount (4 bytes BCD)

    // Count active levels
    uint8_t levelCount = static_cast<uint8_t>(progressiveLevels.size());
    response.data.push_back(levelCount);

    // Add each level
    for (const auto& entry : progressiveLevels) {
        const ProgressiveLevel& level = entry.second;

        // Level ID
        response.data.push_back(level.levelId);

        // Current amount
        std::vector<uint8_t> amountBCD = BCD::encode(level.currentAmount, 4);
        response.data.insert(response.data.end(), amountBCD.begin(), amountBCD.end());
    }

    return response;
}

Message ProgressiveCommands::handleSendProgressiveBroadcast(simulator::Machine* machine) {
    if (!machine) {
        return Message();
    }

    // Initialize progressives if needed
    if (!progressivesInitialized) {
        initializeProgressives(machine);
    }

    Message response;
    response.address = 1;
    response.command = LongPoll::SEND_PROGRESSIVE_BROADCAST;

    // Broadcast format - similar to levels but optimized for display
    // Typically includes top 4 progressive levels

    // Number of progressives to broadcast (max 4)
    uint8_t broadcastCount = std::min(static_cast<uint8_t>(4),
                                     static_cast<uint8_t>(progressiveLevels.size()));
    response.data.push_back(broadcastCount);

    // Add top levels (sorted by amount, descending)
    std::vector<ProgressiveLevel> sortedLevels;
    for (const auto& entry : progressiveLevels) {
        sortedLevels.push_back(entry.second);
    }

    std::sort(sortedLevels.begin(), sortedLevels.end(),
              [](const ProgressiveLevel& a, const ProgressiveLevel& b) {
                  return a.currentAmount > b.currentAmount;
              });

    // Add top levels to response
    for (uint8_t i = 0; i < broadcastCount && i < sortedLevels.size(); i++) {
        const ProgressiveLevel& level = sortedLevels[i];

        response.data.push_back(level.levelId);

        std::vector<uint8_t> amountBCD = BCD::encode(level.currentAmount, 4);
        response.data.insert(response.data.end(), amountBCD.begin(), amountBCD.end());
    }

    return response;
}

void ProgressiveCommands::initializeProgressives(simulator::Machine* machine) {
    if (progressivesInitialized) {
        return;
    }

    // Create default progressive levels
    // Level 1: Mini progressive (starts at $10)
    ProgressiveLevel mini;
    mini.levelId = 1;
    mini.currentAmount = 1000;      // $10.00
    mini.resetAmount = 1000;
    mini.incrementRate = 1;         // 1 cent per bet
    mini.hasWin = false;
    progressiveLevels[1] = mini;

    // Level 2: Minor progressive (starts at $100)
    ProgressiveLevel minor;
    minor.levelId = 2;
    minor.currentAmount = 10000;    // $100.00
    minor.resetAmount = 10000;
    minor.incrementRate = 5;        // 5 cents per bet
    minor.hasWin = false;
    progressiveLevels[2] = minor;

    // Level 3: Major progressive (starts at $1,000)
    ProgressiveLevel major;
    major.levelId = 3;
    major.currentAmount = 100000;   // $1,000.00
    major.resetAmount = 100000;
    major.incrementRate = 10;       // 10 cents per bet
    major.hasWin = false;
    progressiveLevels[3] = major;

    // Level 4: Grand progressive (starts at $10,000)
    ProgressiveLevel grand;
    grand.levelId = 4;
    grand.currentAmount = 1000000;  // $10,000.00
    grand.resetAmount = 1000000;
    grand.incrementRate = 25;       // 25 cents per bet
    grand.hasWin = false;
    progressiveLevels[4] = grand;

    progressivesInitialized = true;
}

void ProgressiveCommands::incrementProgressives(simulator::Machine* machine, uint64_t betAmount) {
    if (!machine) {
        return;
    }

    // Initialize if needed
    if (!progressivesInitialized) {
        initializeProgressives(machine);
    }

    // Increment all progressive levels based on their increment rates
    for (auto& entry : progressiveLevels) {
        ProgressiveLevel& level = entry.second;
        level.currentAmount += level.incrementRate;
    }
}

uint64_t ProgressiveCommands::awardProgressiveWin(simulator::Machine* machine, uint8_t levelId) {
    if (!machine) {
        return 0;
    }

    // Initialize if needed
    if (!progressivesInitialized) {
        initializeProgressives(machine);
    }

    ProgressiveLevel* level = getProgressiveLevel(levelId);
    if (level == nullptr) {
        return 0;
    }

    // Get win amount
    uint64_t winAmount = level->currentAmount;

    // Mark as won
    level->hasWin = true;

    // Add win to machine credits
    machine->addCredits(static_cast<int64_t>(winAmount));

    // Increment jackpot meter
    machine->incrementMeter(SASConstants::METER_JACKPOT, winAmount);

    // Progressive will be reset when handleSendProgressiveWin is called

    return winAmount;
}

ProgressiveCommands::ProgressiveLevel* ProgressiveCommands::getProgressiveLevel(uint8_t levelId) {
    auto it = progressiveLevels.find(levelId);
    if (it != progressiveLevels.end()) {
        return &(it->second);
    }
    return nullptr;
}

Message ProgressiveCommands::buildProgressiveResponse(uint8_t address,
                                                      uint8_t command,
                                                      uint8_t levelId,
                                                      uint64_t amount) {
    Message response;
    response.address = address;
    response.command = command;

    // Level ID
    response.data.push_back(levelId);

    // Amount (5 bytes BCD)
    std::vector<uint8_t> amountBCD = BCD::encode(amount, 5);
    response.data.insert(response.data.end(), amountBCD.begin(), amountBCD.end());

    return response;
}

} // namespace commands
} // namespace sas

