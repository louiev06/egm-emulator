#ifndef SIMULATOR_GAME_H
#define SIMULATOR_GAME_H

#include <string>
#include <cstdint>


namespace simulator {

/**
 * Represents a single game configuration within a multi-game cabinet.
 * This is the C++ port of Game.java
 */
class Game {
public:
    /**
     * Constructor
     * @param gameNumber The game number/ID
     * @param denomCode The denomination code (per SAS spec)
     * @param maxBet Maximum bet allowed for this game
     * @param gameName Name of the game
     * @param paytable Paytable identifier
     */
    Game(int gameNumber, int denomCode, int maxBet,
         const std::string& gameName, const std::string& paytable);

    // Getters
    int getGameNumber() const { return gameNumber_; }
    int getDenomCode() const { return denomCode_; }
    double getDenom() const;  // Returns denomination in dollars
    int getMaxBet() const { return maxBet_; }
    std::string getGameName() const { return gameName_; }
    std::string getPaytable() const { return paytable_; }
    double getCoinInMeter() const { return coinInMeter_; }

    // Setters
    void setMaxBet(int maxBet) { maxBet_ = maxBet; }
    void setGameName(const std::string& name) { gameName_ = name; }
    void setPaytable(const std::string& paytable) { paytable_ = paytable; }

    /**
     * Place a bet and update coin-in meter
     * @param credits Number of credits to bet
     * @return The dollar amount of the bet
     */
    double bet(int credits);

    /**
     * Reset the coin-in meter
     */
    void resetCoinInMeter();

private:
    int gameNumber_;
    int denomCode_;
    int maxBet_;
    std::string gameName_;
    std::string paytable_;
    double coinInMeter_;  // Total wagered on this game in dollars
};

} // namespace simulator


#endif // SIMULATOR_GAME_H
