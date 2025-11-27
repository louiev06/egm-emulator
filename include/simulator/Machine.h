#ifndef SIMULATOR_MACHINE_H
#define SIMULATOR_MACHINE_H

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <queue>
#include <atomic>
#include <mutex>
#include <thread>
#include <functional>
#include "Game.h"
#include "event/EventService.h"



// Forward declarations from parent namespace
class ICardPlatform;

namespace io {
class MachineCommPort;
}

namespace simulator {

/**
 * LevelValue represents a progressive jackpot level
 */
struct LevelValue {
    int levelId;
    double value;  // Using double for BigDecimal equivalent (in dollars)

    LevelValue() : levelId(0), value(0.0) {}
    LevelValue(int id, double val) : levelId(id), value(val) {}
};

/**
 * CreditVoucher represents a ticket/voucher
 */
struct CreditVoucher {
    uint64_t validationNumber;
    double amount;
    int voucherType;  // 0 = cashable, 1 = restricted, etc.
};

/**
 * Simulates a game cabinet with one or more configured games.
 * This is the C++ port of Machine.java
 */
class Machine {
public:
    // Constants
    static constexpr double CENTS_IN_DOLLAR = 100.0;
    static constexpr double DEFAULT_HANDPAY_LIMIT = 400.00;

    // Constructor/Destructor
    explicit Machine(std::shared_ptr<event::EventService> eventService,
                    std::shared_ptr<ICardPlatform> platform);
    ~Machine();

    // Port management
    std::shared_ptr<io::MachineCommPort> addSASPort();
    bool isConfigured() const;

    // Game management
    void setCurrentGame(std::shared_ptr<Game> game);
    void setCurrentGame(int gameNumber, double denomAmount);
    std::shared_ptr<Game> getGame(int gameNumber, double denomAmount);
    std::shared_ptr<Game> addGame(int gameNumber, int denomCode, int maxBet,
                                   const std::string& gameName, const std::string& paytable);
    std::shared_ptr<Game> addGame(int gameNumber, double denom, int maxBet,
                                   const std::string& gameName, const std::string& paytable);
    std::shared_ptr<Game> getCurrentGame() const { return currentGame_; }
    const std::vector<std::shared_ptr<Game>>& getGames() const { return games_; }
    int getCurrentGameIndex() const;

    // Meter management
    bool hasMeter(int meterCode) const;
    int64_t getMeter(int meterCode) const;
    void setMeter(int meterCode, int64_t value);
    void incrementMeter(int meterCode, int64_t amount);
    int64_t getGamesPlayed() const;

    const std::map<int, int64_t>& getMachineMeters() const { return machineMeters_; }

    // Progressive management
    void addProgressive(int levelId);
    void setProgressive(int levelId, float amount);
    void setProgressiveValue(int levelId, double amount, bool updateTime = true);
    double getProgressive(int levelId) const;
    std::vector<int> getProgressiveLevelIds() const;
    void progressiveHit(int levelId);
    LevelValue getOldestHit();
    bool isProgressiveLinkUp() const;
    void clearProgressiveValues();

    // Credits management
    int64_t getCredits() const;
    double getCashableAmount() const;
    void addCredits(int64_t credits);
    void addCredits(double dollarAmount);

    int64_t getRestrictedCredits() const;
    double getRestrictedAmount() const;
    void addRestrictedCredits(int64_t credits);
    void addRestrictedCredits(double dollarAmount);

    int64_t getNonRestrictedCredits() const;
    double getNonRestrictedAmount() const;
    void addNonRestrictedCredits(int credits);
    void addNonRestrictedCredits(double dollarAmount);

    // Jackpot/Award management
    void addJackpot(double award);
    void addCoinOut(double coinOut);
    void awardBonus(int64_t bonusUnits, bool aft);

    // Game play
    int64_t playGameCredit();
    void gameStart(int credits);
    void pokerGameStart(int credits, const std::string& dealtHand);
    void gameEnd();
    void pokerGameEnd(const std::string& finalHand);
    void GameWon();
    void GameLost();
    void bet(int credits);
    void betMax();
    void secondaryWager(int credits);

    int64_t getCreditsByGameDenom() const;
    int64_t getRestrictedCreditsByGameDenom() const;
    int64_t getNonRestrictedCreditsByGameDenom() const;

    // Machine state
    void start();
    void stop();
    bool isStarted() const { return started_.load(); }
    bool isEnabled() const { return enabled_.load(); }
    void setEnabled(bool enabled);
    bool isPlayable() const;
    void checkPlayable();

    // Door/Light/Hopper
    void setDoorOpen(bool open);
    bool isDoorOpen() const { return doorOpen_; }
    void setLightOn(bool on);
    bool isLightOn() const { return lightOn_; }
    void setHopper(bool isLow);
    bool isHopperLow() const { return hopperLow_; }

    // Handpay
    double getHandpayLimit() const { return handpayLimit_; }
    void setHandpayLimit(double limit) { handpayLimit_ = limit; }
    bool isHandpayPending() const;
    void handpayReset();
    void cashoutButtonTriggerHandpay();
    void cashoutButton();
    void setIgnoreHandpay(bool flag) { ignoreHandpay_ = flag; }
    bool getIgnoreHandpay() const { return ignoreHandpay_; }

    // AFT/EFT
    void setAftLocked(bool locked);
    bool isAftLocked() const { return aftLocked_.load(); }
    bool isEftTransferFromEnabled() const { return eftTransferFromEnabled_; }
    void setEftTransferFromEnabled(bool enabled) { eftTransferFromEnabled_ = enabled; }
    bool isEfttransferToEnabled() const { return efttransferToEnabled_; }
    void setEfttransferToEnabled(bool enabled) { efttransferToEnabled_ = enabled; }

    void publishAftTransfer(int64_t cashableAmount, int64_t restrictedAmount,
                           int64_t nonRestrictedAmount);
    void publishAftLock(bool lock);
    void publishEftTransfer();
    void publishGameDelay(int64_t delayMillis);

    // Denomination
    int getAccountingDenomCode() const { return accountingDenomCode_; }
    void setAccountingDenomCode(int code) { accountingDenomCode_ = code; }
    double getAccountingDenom() const;
    int64_t toAccountingDenom(double amount) const;
    double fromAccountingDenom(int64_t amount) const;

    std::vector<int> getEnabledDenomCodes() const;
    std::vector<int> getEnabledGames(int denominationCode) const;
    std::vector<int> getEnabledGames() const;

    // Base percentage (RTP)
    std::string getBasePercentage() const { return basePercentage_; }
    void setBasePercentage(const std::string& percentage) { basePercentage_ = percentage; }
    void setBasePercentage(int themeId, const std::string& percentage);
    std::string getBasePercentage(int themeId) const;

    // Meters
    double getCoinInMeter() const;
    double getCoinInMeter(int denomCode) const;
    int64_t getCoinOutMeter() const;
    double getCoinOutMeterAsCurrency() const;
    int64_t getDropMeter() const;
    double getDropMeterAsCurrency() const;
    int64_t getJackpotMeter() const;
    double getJackpotMeterAsCurrency() const;
    int64_t getDenomMeter(int denominationCode) const;

    // Progressive group
    int getProgressiveGroup() const { return progressiveGroup_; }
    void setProgressiveGroup(int group) { progressiveGroup_ = group; }
    int getReportedProgressiveGroup() const { return reportedProgressiveGroup_; }
    void setReportedProgressiveGroup(int group) { reportedProgressiveGroup_ = group; }

    // Misc
    int64_t getAssetNumber() const { return assetNumber_; }
    void setAssetNumber(int64_t number) { assetNumber_ = number; }
    bool isRoundProgressiveJPToGameDenom() const { return roundProgressiveJPToGameDenom_; }
    void setRoundProgressiveJPToGameDenom(bool round) { roundProgressiveJPToGameDenom_ = round; }
    bool isPlaySecondaryWager() const { return playSecondaryWager_; }
    void setPlaySecondaryWager(bool play) { playSecondaryWager_ = play; }
    bool isFastPolling() const { return fastPolling_; }
    void setFastPolling(bool fast) { fastPolling_ = fast; }
    bool isNackBonusAward() const { return nackBonusAward_; }
    void setNackBonusAward(bool nack) { nackBonusAward_ = nack; }
    bool isConnected() const;
    void setConnected(bool connected);
    bool isMissingProgressiveUpdates() const { return missingProgressiveUpdates_; }
    void setMissingProgressiveUpdates(bool missing) { missingProgressiveUpdates_ = missing; }
    bool isAutoProcessEvents() const { return autoProcessEvents_; }
    void autoProcessEvents();

    // RAM clear
    void doRamClear();
    void ramClear();

    // Options
    void optionsChanged();

    // Poker hand
    std::string getPokerHand() const { return pokerHand_; }
    bool isPokerHandFinal() const { return pokerHandFinal_; }

    // Game delay
    void setDelayMillis(int64_t delayMillis);
    int64_t getDelayMillis() const { return delayMillis_; }
    void subtractDelayMillis(int64_t amount);
    bool isGameDelayed() const { return delayMillis_ > 0; }

    // Voucher
    bool isWaitingToPrintCashoutVoucher() const { return waitingToPrintCashoutVoucher_; }
    void setWaitingToPrintCashoutVoucher(bool waiting) { waitingToPrintCashoutVoucher_ = waiting; }
    bool printVoucher(const CreditVoucher& voucher);

    // Event service
    std::shared_ptr<event::EventService> getEventService() const { return eventService_; }

    // Port access
    const std::vector<std::shared_ptr<io::MachineCommPort>>& getPorts() const { return ports_; }
    std::shared_ptr<io::MachineCommPort> getPrimarySASPort();
    bool hasSAS() const;

    int getMaxMaxBet() const;
    std::string getPaytable() const;
    std::shared_ptr<Game> getGameByGameNumber(int gameNumber) const;

private:
    // Private helper methods
    void initializeMeters();
    int getDenomCode(double denom) const;
    double convertDenomToBigDecimal(double denom) const;
    int64_t playRestrictedGameCredit();
    int64_t playNonRestrictedGameCredit();
    void addPendingHandpay(double amount, int levelId);
    void gameStateException(const std::string& msg);
    void progressiveWatchdogTask();

    // Member variables
    std::shared_ptr<event::EventService> eventService_;
    std::shared_ptr<ICardPlatform> platform_;

    std::vector<std::shared_ptr<io::MachineCommPort>> ports_;
    std::vector<std::shared_ptr<Game>> games_;
    std::shared_ptr<Game> currentGame_;

    std::map<int, int64_t> machineMeters_;
    std::vector<LevelValue> progressives_;
    std::queue<LevelValue> progressiveHits_;
    std::queue<int64_t> pendingHandpayReset_;
    std::map<int, std::string> basePercentageByTheme_;

    int reportedProgressiveGroup_;
    int accountingDenomCode_;
    int progressiveGroup_;
    int64_t assetNumber_;
    int64_t lastProgressiveSetTime_;
    int64_t delayMillis_;
    double handpayLimit_;
    std::string basePercentage_;
    std::string pokerHand_;

    std::atomic<bool> started_;
    std::atomic<bool> enabled_;
    std::atomic<bool> aftLocked_;
    bool doorOpen_;
    bool lightOn_;
    bool hopperLow_;
    bool nackBonusAward_;
    bool missingProgressiveUpdates_;
    bool roundProgressiveJPToGameDenom_;
    bool playSecondaryWager_;
    bool waitingToPrintCashoutVoucher_;
    bool pokerHandFinal_;
    bool fastPolling_;
    bool eftTransferFromEnabled_;
    bool efttransferToEnabled_;
    bool ignoreHandpay_;
    bool playable_;
    bool pendingLock_;
    bool autoProcessEvents_;

    // Threading
    mutable std::recursive_mutex mutex_;
    std::unique_ptr<std::thread> watchdogThread_;
    std::atomic<bool> stopWatchdog_;
};

} // namespace simulator


#endif // SIMULATOR_MACHINE_H
