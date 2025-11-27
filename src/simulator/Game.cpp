#include "simulator/Game.h"
#include "sas/SASConstants.h"


namespace simulator {

Game::Game(int gameNumber, int denomCode, int maxBet,
           const std::string& gameName, const std::string& paytable)
    : gameNumber_(gameNumber),
      denomCode_(denomCode),
      maxBet_(maxBet),
      gameName_(gameName),
      paytable_(paytable),
      coinInMeter_(0.0) {
}

double Game::getDenom() const {
    return sas::SASConstants::DENOMINATIONS.getDenomination(denomCode_);
}

double Game::bet(int credits) {
    double betAmount = getDenom() * credits;
    coinInMeter_ += betAmount;
    return betAmount;
}

void Game::resetCoinInMeter() {
    coinInMeter_ = 0.0;
}

} // namespace simulator

