#include "simulator/Machine.h"
#include "simulator/MachineEvents.h"
#include "sas/SASConstants.h"
#include "sas/SASCommPort.h"
#include "ICardPlatform.h"
#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <cmath>
#include <chrono>
#include <memory>
#include <set>


namespace simulator {

// MachineCommPort base class implementation
class MachineCommPort {
public:
    explicit MachineCommPort(Machine* machine) : machine_(machine) {}
    virtual ~MachineCommPort() = default;

    virtual void gameSelected() = 0;
    virtual void setMultigame(bool multigame) = 0;
    virtual void progressiveHit(int levelId) = 0;
    virtual void progressivePaid() = 0;
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void gameStarted(int credits, double denom) = 0;
    virtual void gameEnded() = 0;
    virtual void doorOpen() = 0;
    virtual void doorClose() = 0;
    virtual void cashoutButton() = 0;
    virtual void lightOn() = 0;
    virtual void lightOff() = 0;
    virtual void hopperLow() = 0;
    virtual void setConnected(bool connected) = 0;
    virtual bool isConnected() = 0;
    virtual void handpayPending(int levelId, double amount) = 0;
    virtual void resetOldestHandpay() = 0;
    virtual void ramClear() = 0;
    virtual void optionsChanged() = 0;
    virtual std::string getPortType() const = 0;

protected:
    Machine* machine_;
};

// Machine implementation
Machine::Machine(std::shared_ptr<event::EventService> eventService,
                 std::shared_ptr<ICardPlatform> platform)
    : eventService_(eventService),
      platform_(platform),
      reportedProgressiveGroup_(1),
      accountingDenomCode_(1),
      progressiveGroup_(1),
      assetNumber_(0),
      lastProgressiveSetTime_(-1),
      delayMillis_(0),
      handpayLimit_(DEFAULT_HANDPAY_LIMIT),
      basePercentage_("0000"),
      started_(false),
      enabled_(true),
      aftLocked_(false),
      doorOpen_(false),
      lightOn_(false),
      hopperLow_(false),
      nackBonusAward_(false),
      missingProgressiveUpdates_(false),
      roundProgressiveJPToGameDenom_(false),
      playSecondaryWager_(false),
      waitingToPrintCashoutVoucher_(false),
      pokerHandFinal_(false),
      fastPolling_(true),
      eftTransferFromEnabled_(false),
      efttransferToEnabled_(false),
      ignoreHandpay_(false),
      playable_(true),
      pendingLock_(false),
      autoProcessEvents_(false),
      stopWatchdog_(false) {

    initializeMeters();

    // Start progressive watchdog thread (C++11 compatible - reset instead of make_unique)
    watchdogThread_.reset(new std::thread([this]() {
        progressiveWatchdogTask();
    }));
}

Machine::~Machine() {
    stopWatchdog_ = true;
    if (watchdogThread_ && watchdogThread_->joinable()) {
        watchdogThread_->join();
    }
}

void Machine::initializeMeters() {
    using namespace sas;
    machineMeters_[SASConstants::METER_COIN_IN] = 0;
    machineMeters_[SASConstants::METER_COIN_OUT] = 0;
    machineMeters_[SASConstants::METER_JACKPOT] = 0;
    machineMeters_[SASConstants::METER_HANDPAID_CANCELLED_CRD] = 0;
    machineMeters_[SASConstants::METER_CANCELLED_CRD] = 0;
    machineMeters_[SASConstants::METER_GAMES_PLAYED] = 0;
    machineMeters_[SASConstants::METER_GAMES_WON] = 0;
    machineMeters_[SASConstants::METER_GAMES_LOST] = 0;
    machineMeters_[SASConstants::METER_CRD_FR_COIN_ACCEPTOR] = 0;
    machineMeters_[SASConstants::METER_CRD_PAID_FR_HOPPER] = 0;
    machineMeters_[SASConstants::METER_CRD_FR_COIN_TO_DROP] = 0;
    machineMeters_[SASConstants::METER_CRD_FR_BILL_ACCEPTOR] = 0;
    machineMeters_[SASConstants::METER_CURRENT_CRD] = 0;
    machineMeters_[SASConstants::METER_TOT_TKT_IN] = 0;
    machineMeters_[SASConstants::METER_TOT_TKT_OUT] = 0;
    machineMeters_[SASConstants::METER_TOT_DROP] = 0;
    machineMeters_[SASConstants::METER_REG_CASHABLE_TKT_IN] = 0;
    machineMeters_[SASConstants::METER_REST_PROMO_TKT_IN] = 0;
    machineMeters_[SASConstants::METER_1_BILLS_ACCEPTED] = 0;
    machineMeters_[SASConstants::METER_5_BILLS_ACCEPTED] = 0;
    machineMeters_[SASConstants::METER_10_BILLS_ACCEPTED] = 0;
    machineMeters_[SASConstants::METER_20_BILLS_ACCEPTED] = 0;
    machineMeters_[SASConstants::METER_50_BILLS_ACCEPTED] = 0;
    machineMeters_[SASConstants::METER_100_BILLS_ACCEPTED] = 0;
}

void Machine::progressiveWatchdogTask() {
    while (!stopWatchdog_) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        if (!progressives_.empty()) {
            auto now = std::chrono::system_clock::now();
            auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()).count();

            if (lastProgressiveSetTime_ > 0 && (nowMs - lastProgressiveSetTime_) > 5000) {
                // Progressive link down
                clearProgressiveValues();
            }
        }
    }
}

void Machine::clearProgressiveValues() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    for (auto& progressive : progressives_) {
        setProgressiveValue(progressive.levelId, 0.0, false);
    }
}

bool Machine::hasMeter(int meterCode) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return machineMeters_.find(meterCode) != machineMeters_.end();
}

int64_t Machine::getMeter(int meterCode) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto it = machineMeters_.find(meterCode);
    if (it != machineMeters_.end()) {
        return it->second;
    }
    return 0;
}

void Machine::setMeter(int meterCode, int64_t value) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    machineMeters_[meterCode] = value;
}

void Machine::incrementMeter(int meterCode, int64_t amount) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    // TODO: Handle meter rollover
    // Now safe to call getMeter() since we're using recursive_mutex
    machineMeters_[meterCode] = getMeter(meterCode) + amount;
}

int64_t Machine::getGamesPlayed() const {
    return getMeter(sas::SASConstants::METER_GAMES_PLAYED);
}

bool Machine::isConfigured() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return !ports_.empty() || !games_.empty();
}

void Machine::setCurrentGame(std::shared_ptr<Game> game) {
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        currentGame_ = game;
    }

    eventService_->publish(GameChangedEvent());

    // TODO: Implement gameSelected() in SASCommPort to report game change via exception
    // for (auto& port : ports_) {
    //     auto sasPort = std::dynamic_pointer_cast<sas::SASCommPort>(port);
    //     if (sasPort) {
    //         sasPort->gameSelected();
    //     }
    // }
}

void Machine::setCurrentGame(int gameNumber, double denomAmount) {
    auto foundGame = getGame(gameNumber, denomAmount);
    if (foundGame) {
        setCurrentGame(foundGame);
    }
}

std::shared_ptr<Game> Machine::getGame(int gameNumber, double denomAmount) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    int denomCode = getDenomCode(convertDenomToBigDecimal(denomAmount));
    for (auto& game : games_) {
        if (game->getGameNumber() == gameNumber && game->getDenomCode() == denomCode) {
            return game;
        }
    }
    return nullptr;
}

double Machine::convertDenomToBigDecimal(double denom) const {
    // Round to cents
    int denomCents = static_cast<int>(denom * 100.0);
    return denomCents / 100.0;
}

int Machine::getDenomCode(double denom) const {
    return sas::SASConstants::DENOMINATIONS.getDenomCodeByDenomination(denom);
}

std::shared_ptr<Game> Machine::addGame(int gameNumber, int denomCode, int maxBet,
                                        const std::string& gameName,
                                        const std::string& paytable) {
    auto game = std::make_shared<Game>(gameNumber, denomCode, maxBet, gameName, paytable);

    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        games_.push_back(game);

        // TODO: Implement setMultigame() in SASCommPort when multi-game support is needed
        // if (games_.size() > 1) {
        //     for (auto& port : ports_) {
        //         auto sasPort = std::dynamic_pointer_cast<sas::SASCommPort>(port);
        //         if (sasPort) {
        //             sasPort->setMultigame(true);
        //         }
        //     }
        // }
    }

    return game;
}

std::shared_ptr<Game> Machine::addGame(int gameNumber, double denom, int maxBet,
                                        const std::string& gameName,
                                        const std::string& paytable) {
    int denomCode = getDenomCode(convertDenomToBigDecimal(denom));
    if (denomCode == -1) {
        throw std::invalid_argument("Invalid denomination: " + std::to_string(denom));
    }
    return addGame(gameNumber, denomCode, maxBet, gameName, paytable);
}

int Machine::getCurrentGameIndex() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto it = std::find(games_.begin(), games_.end(), currentGame_);
    if (it != games_.end()) {
        return static_cast<int>(std::distance(games_.begin(), it));
    }
    return -1;
}

double Machine::getAccountingDenom() const {
    return sas::SASConstants::DENOMINATIONS.getDenomination(accountingDenomCode_);
}

int64_t Machine::toAccountingDenom(double amount) const {
    return static_cast<int64_t>(amount / getAccountingDenom());
}

double Machine::fromAccountingDenom(int64_t amount) const {
    return getAccountingDenom() * amount;
}

int64_t Machine::getCredits() const {
    return getMeter(sas::SASConstants::METER_CURRENT_CRD);
}

double Machine::getCashableAmount() const {
    return getAccountingDenom() * getCredits();
}

void Machine::addCredits(int64_t credits) {
    incrementMeter(sas::SASConstants::METER_CURRENT_CRD, credits);
}

void Machine::addCredits(double dollarAmount) {
    addCredits(static_cast<int64_t>(dollarAmount / getAccountingDenom()));
}

int64_t Machine::getRestrictedCredits() const {
    return getMeter(sas::SASConstants::METER_CURRENT_REST_CRD);
}

double Machine::getRestrictedAmount() const {
    return getAccountingDenom() * getRestrictedCredits();
}

void Machine::addRestrictedCredits(int64_t credits) {
    incrementMeter(sas::SASConstants::METER_CURRENT_REST_CRD, credits);
}

void Machine::addRestrictedCredits(double dollarAmount) {
    addRestrictedCredits(static_cast<int64_t>(dollarAmount / getAccountingDenom()));
}

int64_t Machine::getNonRestrictedCredits() const {
    return getMeter(sas::SASConstants::METER_TOTAL_NONREST_PLAYED);
}

double Machine::getNonRestrictedAmount() const {
    return getAccountingDenom() * getNonRestrictedCredits();
}

void Machine::addNonRestrictedCredits(int credits) {
    incrementMeter(sas::SASConstants::METER_TOTAL_NONREST_PLAYED, credits);
}

void Machine::addNonRestrictedCredits(double dollarAmount) {
    addNonRestrictedCredits(static_cast<int>(dollarAmount / getAccountingDenom()));
}

void Machine::addProgressive(int levelId) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    // Don't add duplicates
    for (const auto& progressive : progressives_) {
        if (progressive.levelId == levelId) {
            return;
        }
    }

    LevelValue value(levelId, 0.0);
    progressives_.push_back(value);
}

void Machine::setProgressive(int levelId, float amount) {
    setProgressiveValue(levelId, static_cast<double>(amount));
}

void Machine::setProgressiveValue(int levelId, double amount, bool updateTime) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    for (auto& value : progressives_) {
        if (value.levelId == levelId) {
            if (updateTime) {
                auto now = std::chrono::system_clock::now();
                lastProgressiveSetTime_ = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now.time_since_epoch()).count();
            }

            double oldValue = value.value;
            if (std::abs(oldValue - amount) > 0.001) {  // Comparison with tolerance
                value.value = amount;
                eventService_->publish(LevelValueChangedEvent(value));
            }
            return;
        }
    }
}

double Machine::getProgressive(int levelId) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    for (const auto& value : progressives_) {
        if (value.levelId == levelId) {
            return value.value;
        }
    }
    return 0.0;
}

std::vector<int> Machine::getProgressiveLevelIds() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    std::vector<int> ids;
    for (const auto& value : progressives_) {
        ids.push_back(value.levelId);
    }
    return ids;
}

void Machine::progressiveHit(int levelId) {
    checkPlayable();

    double win = getProgressive(levelId);

    if (roundProgressiveJPToGameDenom_ && currentGame_) {
        double gameDenom = currentGame_->getDenom();
        double remainder = std::fmod(win, gameDenom);
        win = win + (gameDenom - remainder);
    }

    eventService_->publish(ProgressiveHitEvent(levelId, win));

    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        LevelValue value(levelId, win);
        progressiveHits_.push(value);
    }

    for (auto& port : ports_) {
        // TODO: Implement progressiveHit() in SASCommPort to report progressive win via exception
        // auto sasPort = std::dynamic_pointer_cast<sas::SASCommPort>(port);
        // if (sasPort) {
        //     sasPort->progressiveHit(levelId);
        // }
    }

    if (win >= handpayLimit_) {
        addPendingHandpay(win, levelId);
    } else {
        // TODO: Implement progressivePaid() in SASCommPort to report progressive payment
        // for (auto& port : ports_) {
        //     auto sasPort = std::dynamic_pointer_cast<sas::SASCommPort>(port);
        //     if (sasPort) {
        //         sasPort->progressivePaid();
        //     }
        // }
    }
}

LevelValue Machine::getOldestHit() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!progressiveHits_.empty()) {
        LevelValue value = progressiveHits_.front();
        progressiveHits_.pop();
        return value;
    }
    return LevelValue();
}

bool Machine::isProgressiveLinkUp() const {
    if (missingProgressiveUpdates_) {
        return false;
    }

    if (progressives_.empty()) {
        return true;
    }

    auto now = std::chrono::system_clock::now();
    auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();

    return (nowMs - lastProgressiveSetTime_) < 5000;
}

void Machine::addJackpot(double award) {
    int64_t awardCredits = static_cast<int64_t>(award / getAccountingDenom());
    addCredits(awardCredits);
    incrementMeter(sas::SASConstants::METER_JACKPOT, awardCredits);
}

void Machine::addCoinOut(double coinOut) {
    int64_t awardCredits = static_cast<int64_t>(coinOut / getAccountingDenom());
    addCredits(awardCredits);
    incrementMeter(sas::SASConstants::METER_COIN_OUT, awardCredits);
}

int64_t Machine::playGameCredit() {
    if (playRestrictedGameCredit() == 1) {
        return 1;
    }

    if (playNonRestrictedGameCredit() == 1) {
        return 1;
    }

    if (getCreditsByGameDenom() < 1) {
        return 0;
    }

    if (currentGame_) {
        addCredits(-1 * static_cast<int64_t>(currentGame_->getDenom() / getAccountingDenom()));
    }

    return 1;
}

int64_t Machine::playRestrictedGameCredit() {
    if (getRestrictedCreditsByGameDenom() < 1) {
        return 0;
    }

    if (currentGame_) {
        addRestrictedCredits(-1 * static_cast<int64_t>(currentGame_->getDenom() / getAccountingDenom()));
    }

    return 1;
}

int64_t Machine::playNonRestrictedGameCredit() {
    if (getNonRestrictedCreditsByGameDenom() < 1) {
        return 0;
    }

    if (currentGame_) {
        addNonRestrictedCredits(-1 * static_cast<int>(currentGame_->getDenom() / getAccountingDenom()));
    }

    return 1;
}

int64_t Machine::getCreditsByGameDenom() const {
    if (!currentGame_) {
        return 0;
    }
    return static_cast<int64_t>((getAccountingDenom() * getMeter(sas::SASConstants::METER_CURRENT_CRD)) /
                                currentGame_->getDenom());
}

int64_t Machine::getRestrictedCreditsByGameDenom() const {
    if (!currentGame_) {
        return 0;
    }
    return static_cast<int64_t>((getAccountingDenom() * getMeter(sas::SASConstants::METER_CURRENT_REST_CRD)) /
                                currentGame_->getDenom());
}

int64_t Machine::getNonRestrictedCreditsByGameDenom() const {
    if (!currentGame_) {
        return 0;
    }
    return static_cast<int64_t>((getAccountingDenom() * getMeter(sas::SASConstants::METER_TOTAL_NONREST_PLAYED)) /
                                currentGame_->getDenom());
}

void Machine::gameStart(int credits) {
    checkPlayable();
    playable_ = false;
    incrementMeter(sas::SASConstants::METER_GAMES_PLAYED, 1);

    if (currentGame_) {
        double amount = currentGame_->bet(credits);
        incrementMeter(sas::SASConstants::METER_COIN_IN, static_cast<int>(toAccountingDenom(amount)));
        eventService_->publish(GamePlayedEvent(currentGame_, amount));

        // TODO: Implement gameStarted() in SASCommPort to report game start via exception
        // for (auto& port : ports_) {
        //     auto sasPort = std::dynamic_pointer_cast<sas::SASCommPort>(port);
        //     if (sasPort) {
        //         sasPort->gameStarted(credits, currentGame_->getDenom());
        //     }
        // }
    }
}

void Machine::pokerGameStart(int credits, const std::string& dealtHand) {
    pokerHand_ = dealtHand;
    pokerHandFinal_ = false;
    gameStart(credits);
}

void Machine::gameEnd() {
    playable_ = true;
    // TODO: Implement gameEnded() in SASCommPort to report game end via exception
    // for (auto& port : ports_) {
    //     auto sasPort = std::dynamic_pointer_cast<sas::SASCommPort>(port);
    //     if (sasPort) {
    //         sasPort->gameEnded();
    //     }
    // }
}

void Machine::pokerGameEnd(const std::string& finalHand) {
    pokerHand_ = finalHand;
    pokerHandFinal_ = true;
    gameEnd();
}

void Machine::GameWon() {
    incrementMeter(sas::SASConstants::METER_GAMES_WON, 1);
}

void Machine::GameLost() {
    incrementMeter(sas::SASConstants::METER_GAMES_LOST, 1);
}

void Machine::bet(int credits) {
    gameStart(credits);
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    gameEnd();
}

void Machine::betMax() {
    if (currentGame_) {
        bet(currentGame_->getMaxBet());
    }
}

void Machine::secondaryWager(int credits) {
    if (currentGame_) {
        double amount = currentGame_->bet(credits);
        incrementMeter(sas::SASConstants::METER_COIN_IN, static_cast<int>(toAccountingDenom(amount)));
    }
}

void Machine::start() {
    started_ = true;
}

void Machine::stop() {
    for (auto& port : ports_) {
        auto sasPort = std::dynamic_pointer_cast<sas::SASCommPort>(port);
        if (sasPort) {
            sasPort->stop();
        }
    }
}

void Machine::setEnabled(bool enabled) {
    enabled_ = enabled;
}

bool Machine::isPlayable() const {
    return enabled_ && !isHandpayPending() && !doorOpen_ &&
           isProgressiveLinkUp() && !aftLocked_ && !waitingToPrintCashoutVoucher_;
}

void Machine::checkPlayable() {
    if (isHandpayPending()) {
        gameStateException("Game not playable, handpay is pending.");
    }

    if (isDoorOpen()) {
        gameStateException("Game not playable, door is open.");
    }

    if (!isProgressiveLinkUp()) {
        gameStateException("Game not playable, progressive link down.");
    }

    if (!isEnabled()) {
        gameStateException("Game disabled by SAS host.");
    }

    if (isAftLocked()) {
        gameStateException("Game not playable, locked by SAS AFT Lock Request.");
    }

    if (isWaitingToPrintCashoutVoucher()) {
        gameStateException("Game not playable, waiting for cash out voucher to print.");
    }
}

void Machine::gameStateException(const std::string& msg) {
    throw std::runtime_error(msg);
}

void Machine::setDoorOpen(bool open) {
    if (open && !doorOpen_) {
        doorOpen_ = true;
        // TODO: Implement doorOpen() in SASCommPort to report door open via exception
        // for (auto& port : ports_) {
        //     auto sasPort = std::dynamic_pointer_cast<sas::SASCommPort>(port);
        //     if (sasPort) {
        //         sasPort->doorOpen();
        //     }
        // }
    } else if (!open && doorOpen_) {
        doorOpen_ = false;
        // TODO: Implement doorClose() in SASCommPort to report door close via exception
        // for (auto& port : ports_) {
        //     auto sasPort = std::dynamic_pointer_cast<sas::SASCommPort>(port);
        //     if (sasPort) {
        //         sasPort->doorClose();
        //     }
        // }
    }
}

void Machine::setLightOn(bool on) {
    if (on && !lightOn_) {
        lightOn_ = true;
        // TODO: Implement lightOn() in SASCommPort when light control is needed
        // for (auto& port : ports_) {
        //     auto sasPort = std::dynamic_pointer_cast<sas::SASCommPort>(port);
        //     if (sasPort) {
        //         sasPort->lightOn();
        //     }
        // }
    } else if (!on && lightOn_) {
        lightOn_ = false;
        // TODO: Implement lightOff() in SASCommPort when light control is needed
        // for (auto& port : ports_) {
        //     auto sasPort = std::dynamic_pointer_cast<sas::SASCommPort>(port);
        //     if (sasPort) {
        //         sasPort->lightOff();
        //     }
        // }
    }
}

void Machine::setHopper(bool isLow) {
    if (isLow && !hopperLow_) {
        hopperLow_ = true;
        // TODO: Implement hopperLow() in SASCommPort when hopper monitoring is needed
        // for (auto& port : ports_) {
        //     auto sasPort = std::dynamic_pointer_cast<sas::SASCommPort>(port);
        //     if (sasPort) {
        //         sasPort->hopperLow();
        //     }
        // }
    } else if (!isLow && hopperLow_) {
        hopperLow_ = false;
    }
}

bool Machine::isConnected() const {
    // Check if any port is running
    for (const auto& port : ports_) {
        if (!port->isRunning()) {
            return false;
        }
    }
    return true;
}

void Machine::setConnected(bool connected) {
    // Start or stop ports based on connection state
    if (connected) {
        for (auto& port : ports_) {
            if (!port->isRunning()) {
                port->start();
            }
        }
    } else {
        for (auto& port : ports_) {
            if (port->isRunning()) {
                port->stop();
            }
        }
    }
}

void Machine::awardBonus(int64_t bonusUnits, bool aft) {
    double amount = fromAccountingDenom(bonusUnits);
    eventService_->publish(BonusAwardedEvent(amount, aft));

    if (amount >= handpayLimit_) {
        addPendingHandpay(amount, 0);
    }
}

void Machine::addPendingHandpay(double amount, int levelId) {
    auto now = std::chrono::system_clock::now();
    int64_t resetId = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();

    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        pendingHandpayReset_.push(resetId);
    }

    // TODO: Implement handpayPending() in SASCommPort to report handpay via exception
    // for (auto& port : ports_) {
    //     auto sasPort = std::dynamic_pointer_cast<sas::SASCommPort>(port);
    //     if (sasPort) {
    //         sasPort->handpayPending(levelId, amount);
    //     }
    // }
}

bool Machine::isHandpayPending() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    if (ignoreHandpay_ && !pendingHandpayReset_.empty()) {
        // Auto-reset if ignore flag is set
        const_cast<Machine*>(this)->handpayReset();
        return false;
    }

    return !pendingHandpayReset_.empty();
}

void Machine::handpayReset() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    if (pendingHandpayReset_.empty()) {
        throw std::runtime_error("No handpay pending.");
    }

    pendingHandpayReset_.pop();

    // TODO: Implement resetOldestHandpay() in SASCommPort to clear handpay exception
    // for (auto& port : ports_) {
    //     auto sasPort = std::dynamic_pointer_cast<sas::SASCommPort>(port);
    //     if (sasPort) {
    //         sasPort->resetOldestHandpay();
    //     }
    // }
}

void Machine::cashoutButtonTriggerHandpay() {
    double amount = getCashableAmount();
    addCredits(-amount);
    addPendingHandpay(amount, 0x80);
}

void Machine::cashoutButton() {
    // TODO: Implement cashoutButton() in SASCommPort to report cashout event via exception
    // for (auto& port : ports_) {
    //     auto sasPort = std::dynamic_pointer_cast<sas::SASCommPort>(port);
    //     if (sasPort) {
    //         sasPort->cashoutButton();
    //     }
    // }
}

void Machine::setAftLocked(bool locked) {
    aftLocked_ = locked;
}

void Machine::publishAftTransfer(int64_t cashableAmount, int64_t restrictedAmount,
                                 int64_t nonRestrictedAmount) {
    eventService_->publish(AftTransferEvent(
        static_cast<double>(cashableAmount) / 100.0,
        static_cast<double>(restrictedAmount) / 100.0,
        static_cast<double>(nonRestrictedAmount) / 100.0));
}

void Machine::publishAftLock(bool lock) {
    eventService_->publish(AftLockEvent(lock));
}

void Machine::publishEftTransfer() {
    eventService_->publish(EftTransferEvent());
}

void Machine::publishGameDelay(int64_t delayMillis) {
    delayMillis_ = delayMillis;
    eventService_->publish(GameDelayEvent(delayMillis));
}

void Machine::setDelayMillis(int64_t delayMillis) {
    delayMillis_ = delayMillis;
}

void Machine::subtractDelayMillis(int64_t amount) {
    if (delayMillis_ <= amount) {
        delayMillis_ = 0;
    } else {
        delayMillis_ -= amount;
    }
}

void Machine::doRamClear() {
    int64_t credits = getMeter(sas::SASConstants::METER_CURRENT_CRD);
    initializeMeters();
    addCredits(static_cast<int64_t>(credits));

    // TODO: Implement ramClear() in SASCommPort to report RAM clear via exception
    // for (auto& port : ports_) {
    //     auto sasPort = std::dynamic_pointer_cast<sas::SASCommPort>(port);
    //     if (sasPort) {
    //         sasPort->ramClear();
    //     }
    // }
}

void Machine::ramClear() {
    doRamClear();
}

void Machine::optionsChanged() {
    // TODO: Implement optionsChanged() in SASCommPort to report configuration change via exception
    // for (auto& port : ports_) {
    //     auto sasPort = std::dynamic_pointer_cast<sas::SASCommPort>(port);
    //     if (sasPort) {
    //         sasPort->optionsChanged();
    //     }
    // }
}

void Machine::setBasePercentage(int themeId, const std::string& percentage) {
    basePercentageByTheme_[themeId] = percentage;
}

std::string Machine::getBasePercentage(int themeId) const {
    auto it = basePercentageByTheme_.find(themeId);
    if (it != basePercentageByTheme_.end()) {
        return it->second;
    }
    return basePercentage_;
}

std::vector<int> Machine::getEnabledDenomCodes() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    std::set<int> denoms;
    for (const auto& game : games_) {
        denoms.insert(game->getDenomCode());
    }

    return std::vector<int>(denoms.begin(), denoms.end());
}

std::vector<int> Machine::getEnabledGames(int denominationCode) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    std::set<int> gameNumbers;
    for (const auto& game : games_) {
        if (game->getDenomCode() == denominationCode) {
            gameNumbers.insert(game->getGameNumber());
        }
    }

    return std::vector<int>(gameNumbers.begin(), gameNumbers.end());
}

std::vector<int> Machine::getEnabledGames() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    std::set<int> gameNumbers;
    for (const auto& game : games_) {
        gameNumbers.insert(game->getGameNumber());
    }

    return std::vector<int>(gameNumbers.begin(), gameNumbers.end());
}

std::shared_ptr<Game> Machine::getGameByGameNumber(int gameNumber) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    for (const auto& game : games_) {
        if (game->getGameNumber() == gameNumber) {
            return game;
        }
    }
    return nullptr;
}

int64_t Machine::getDenomMeter(int denominationCode) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    int64_t meter = 0;
    for (const auto& game : games_) {
        if (game->getDenomCode() == denominationCode) {
            meter += toAccountingDenom(game->getCoinInMeter());
        }
    }
    return meter;
}

double Machine::getCoinInMeter() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    double cim = 0.0;
    for (const auto& game : games_) {
        cim += game->getCoinInMeter();
    }
    return cim;
}

double Machine::getCoinInMeter(int denomCode) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    double cim = 0.0;
    for (const auto& game : games_) {
        if (game->getDenomCode() == denomCode) {
            cim += game->getCoinInMeter();
        }
    }
    return cim;
}

int64_t Machine::getCoinOutMeter() const {
    return getMeter(sas::SASConstants::METER_COIN_OUT);
}

double Machine::getCoinOutMeterAsCurrency() const {
    return fromAccountingDenom(getCoinOutMeter());
}

int64_t Machine::getDropMeter() const {
    return getMeter(sas::SASConstants::METER_CRD_FR_COIN_TO_DROP);
}

double Machine::getDropMeterAsCurrency() const {
    return fromAccountingDenom(getDropMeter());
}

int64_t Machine::getJackpotMeter() const {
    return getMeter(sas::SASConstants::METER_JACKPOT);
}

double Machine::getJackpotMeterAsCurrency() const {
    return fromAccountingDenom(getJackpotMeter());
}

int Machine::getMaxMaxBet() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    int maxMaxBet = 0;
    for (const auto& game : games_) {
        maxMaxBet = std::max(maxMaxBet, game->getMaxBet());
    }
    return maxMaxBet;
}

std::string Machine::getPaytable() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    if (!games_.empty()) {
        return games_[0]->getPaytable();
    }
    return "";
}

bool Machine::printVoucher(const CreditVoucher& voucher) {
    // Placeholder - would integrate with actual voucher printer
    return true;
}

void Machine::autoProcessEvents() {
    // This would set up event listeners similar to the Java implementation
    autoProcessEvents_ = true;
    ignoreHandpay_ = true;

    // Subscribe to level value changed events
    eventService_->subscribe<LevelValueChangedEvent>([](const LevelValueChangedEvent& event) {
        // Handle progressive level change
    });

    // Subscribe to bonus awarded events
    eventService_->subscribe<BonusAwardedEvent>([this](const BonusAwardedEvent& event) {
        addJackpot(event.amount);
        if (event.aft) {
            eventService_->publish(AftTransferCreditedEvent());
        } else {
            eventService_->publish(LegacyBonusCreditedEvent(event.amount));
        }
    });

    // Subscribe to AFT transfer events
    eventService_->subscribe<AftTransferEvent>([this](const AftTransferEvent& event) {
        addCredits(event.cashableAmount);
        addRestrictedCredits(event.restrictedAmount);
        addNonRestrictedCredits(static_cast<int>(event.nonRestrictedAmount));
        eventService_->publish(AftTransferCreditedEvent());
    });

    // AFT lock events would be handled here with pending lock logic
}

// Port management
std::shared_ptr<io::MachineCommPort> Machine::addSASPort() {
    if (!platform_) {
        return nullptr;
    }

    // Create SAS communication channel from platform
    auto channel = platform_->createSASPort();
    if (!channel) {
        return nullptr;
    }

    // Create SAS port with address 1 (default)
    auto sasPort = std::make_shared<sas::SASCommPort>(this, channel, 1);

    // Store in ports list
    ports_.push_back(sasPort);

    return sasPort;
}

std::shared_ptr<io::MachineCommPort> Machine::getPrimarySASPort() {
    // Find first SAS port in the list
    for (auto& port : ports_) {
        auto sasPort = std::dynamic_pointer_cast<sas::SASCommPort>(port);
        if (sasPort) {
            return port;  // Return the base MachineCommPort pointer
        }
    }

    // No SAS port found, try to create one
    return addSASPort();
}

bool Machine::hasSAS() const {
    // Check if any port is a SAS port
    for (const auto& port : ports_) {
        auto sasPort = std::dynamic_pointer_cast<const sas::SASCommPort>(port);
        if (sasPort) {
            return true;
        }
    }
    return false;
}

} // namespace simulator

