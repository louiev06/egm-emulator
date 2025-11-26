#ifndef MEGAMIC_SIMULATOR_MACHINEEVENTS_H
#define MEGAMIC_SIMULATOR_MACHINEEVENTS_H

#include <cstdint>
#include "megamic/simulator/Game.h"
#include "megamic/simulator/Machine.h"

namespace megamic {
namespace simulator {

/**
 * Base class for all machine events
 */
struct MachineEvent {
    virtual ~MachineEvent() = default;
};

/**
 * Event published when a progressive level value changes
 */
struct LevelValueChangedEvent : public MachineEvent {
    LevelValue value;

    explicit LevelValueChangedEvent(const LevelValue& val) : value(val) {}
};

/**
 * Event published when a bonus is awarded
 */
struct BonusAwardedEvent : public MachineEvent {
    double amount;
    bool aft;

    BonusAwardedEvent(double amt, bool isAft) : amount(amt), aft(isAft) {}
};

/**
 * Event published for AFT lock requests
 */
struct AftLockEvent : public MachineEvent {
    bool lock;

    explicit AftLockEvent(bool lockState) : lock(lockState) {}
};

/**
 * Event published when AFT lock is confirmed
 */
struct AftLockedEvent : public MachineEvent {
    bool lock;

    explicit AftLockedEvent(bool lockState) : lock(lockState) {}
};

/**
 * Event published for AFT transfers
 */
struct AftTransferEvent : public MachineEvent {
    double cashableAmount;
    double restrictedAmount;
    double nonRestrictedAmount;

    AftTransferEvent(double cashable, double restricted, double nonRestricted)
        : cashableAmount(cashable),
          restrictedAmount(restricted),
          nonRestrictedAmount(nonRestricted) {}
};

/**
 * Event published when legacy bonus is credited
 */
struct LegacyBonusCreditedEvent : public MachineEvent {
    double amount;

    explicit LegacyBonusCreditedEvent(double amt) : amount(amt) {}
};

/**
 * Event published when AFT transfer is credited
 */
struct AftTransferCreditedEvent : public MachineEvent {
};

/**
 * Event published for EFT transfers
 */
struct EftTransferEvent : public MachineEvent {
};

/**
 * Event published when the current game changes
 */
struct GameChangedEvent : public MachineEvent {
};

/**
 * Event published when a game is played
 */
struct GamePlayedEvent : public MachineEvent {
    std::shared_ptr<Game> game;
    double wager;

    GamePlayedEvent(std::shared_ptr<Game> g, double w)
        : game(g), wager(w) {}
};

/**
 * Event published when a progressive is hit
 */
struct ProgressiveHitEvent : public MachineEvent {
    int levelId;
    double win;

    ProgressiveHitEvent(int level, double winAmount)
        : levelId(level), win(winAmount) {}
};

/**
 * Event published for game delays
 */
struct GameDelayEvent : public MachineEvent {
    int64_t delayMillis;

    explicit GameDelayEvent(int64_t delay) : delayMillis(delay) {}

    int64_t getDelayMillis() const { return delayMillis; }
};

} // namespace simulator
} // namespace megamic

#endif // MEGAMIC_SIMULATOR_MACHINEEVENTS_H
