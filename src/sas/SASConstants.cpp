#include "sas/SASConstants.h"
#include <cmath>


namespace sas {

SASConstants::Denominations::Denominations() {
    // Common SAS denomination codes (simplified)
    // Code 0 = multi-denom
    codeToValue_[0] = 0.0;

    // Standard denominations
    codeToValue_[1] = 0.01;   // 1 cent
    codeToValue_[2] = 0.02;   // 2 cents
    codeToValue_[3] = 0.05;   // 5 cents (nickel)
    codeToValue_[4] = 0.10;   // 10 cents (dime)
    codeToValue_[5] = 0.25;   // 25 cents (quarter)
    codeToValue_[6] = 0.50;   // 50 cents
    codeToValue_[7] = 1.00;   // 1 dollar
    codeToValue_[8] = 2.00;   // 2 dollars
    codeToValue_[9] = 5.00;   // 5 dollars
    codeToValue_[10] = 10.00; // 10 dollars
    codeToValue_[11] = 20.00; // 20 dollars
    codeToValue_[12] = 25.00; // 25 dollars
    codeToValue_[13] = 50.00; // 50 dollars
    codeToValue_[14] = 100.00; // 100 dollars
    codeToValue_[15] = 250.00; // 250 dollars
    codeToValue_[16] = 500.00; // 500 dollars
    codeToValue_[17] = 1000.00; // 1000 dollars

    // Build reverse mapping
    for (const auto& pair : codeToValue_) {
        valueToCode_[pair.second] = pair.first;
    }
}

double SASConstants::Denominations::getDenomination(int denomCode) const {
    auto it = codeToValue_.find(denomCode);
    if (it != codeToValue_.end()) {
        return it->second;
    }
    return 0.01; // Default to 1 cent
}

int SASConstants::Denominations::getDenomCodeByDenomination(double denomination) const {
    // Round to 2 decimal places for comparison
    double rounded = std::round(denomination * 100.0) / 100.0;

    auto it = valueToCode_.find(rounded);
    if (it != valueToCode_.end()) {
        return it->second;
    }
    return -1; // Not found
}

// Static instance
const SASConstants::Denominations SASConstants::DENOMINATIONS;

// C++11 requires definitions for ODR-used constexpr static members
constexpr int SASConstants::METER_COIN_IN;
constexpr int SASConstants::METER_COIN_OUT;
constexpr int SASConstants::METER_JACKPOT;
constexpr int SASConstants::METER_HANDPAID_CANCELLED_CRD;
constexpr int SASConstants::METER_CANCELLED_CRD;
constexpr int SASConstants::METER_GAMES_PLAYED;
constexpr int SASConstants::METER_GAMES_WON;
constexpr int SASConstants::METER_GAMES_LOST;
constexpr int SASConstants::METER_CRD_FR_COIN_ACCEPTOR;
constexpr int SASConstants::METER_CRD_PAID_FR_HOPPER;
constexpr int SASConstants::METER_CRD_FR_COIN_TO_DROP;
constexpr int SASConstants::METER_CRD_FR_BILL_ACCEPTOR;
constexpr int SASConstants::METER_CURRENT_CRD;
constexpr int SASConstants::METER_TOT_TKT_IN;
constexpr int SASConstants::METER_TOT_TKT_OUT;
constexpr int SASConstants::METER_TOT_DROP;
constexpr int SASConstants::METER_REST_TKT_OUT;
constexpr int SASConstants::METER_TOT_TKT_IN_QTY;
constexpr int SASConstants::METER_TOT_TKT_OUT_QTY;
constexpr int SASConstants::METER_REST_TKT_IN_QTY;
constexpr int SASConstants::METER_REG_CASHABLE_TKT_IN;
constexpr int SASConstants::METER_REST_PROMO_TKT_IN;
constexpr int SASConstants::METER_TOTAL_NONREST_PLAYED;

// Bill acceptor aliases
constexpr int SASConstants::METER_1_BILLS_ACCEPTED;
constexpr int SASConstants::METER_2_BILLS_ACCEPTED;
constexpr int SASConstants::METER_5_BILLS_ACCEPTED;
constexpr int SASConstants::METER_10_BILLS_ACCEPTED;
constexpr int SASConstants::METER_20_BILLS_ACCEPTED;
constexpr int SASConstants::METER_50_BILLS_ACCEPTED;
constexpr int SASConstants::METER_100_BILLS_ACCEPTED;
constexpr int SASConstants::METER_200_BILLS_ACCEPTED;
constexpr int SASConstants::METER_500_BILLS_ACCEPTED;
constexpr int SASConstants::METER_1000_BILLS_ACCEPTED;

// Extended METER_* codes (0x100+)
constexpr int SASConstants::METER_COIN_DROP;
constexpr int SASConstants::METER_SLOT_DOOR;
constexpr int SASConstants::METER_DROP_DOOR;
constexpr int SASConstants::METER_LOGIC_DOOR;
constexpr int SASConstants::METER_CASH_DOOR;
constexpr int SASConstants::METER_AUX_FILL_DOOR;
constexpr int SASConstants::METER_ACTUAL_SLOT_DOOR;
constexpr int SASConstants::METER_CHASSIS_DOOR;
constexpr int SASConstants::METER_TRUE_COIN_IN;
constexpr int SASConstants::METER_TRUE_COIN_OUT;
constexpr int SASConstants::METER_ACTUAL_COIN_DROP;
constexpr int SASConstants::METER_PHYS_COIN_IN_DOLLAR_VALUE;
constexpr int SASConstants::METER_PHYS_COIN_OUT_DOLLAR_VALUE;
constexpr int SASConstants::METER_VOUCHER_TICKET_DROP;
constexpr int SASConstants::METER_NCEP_CREDITS;
constexpr int SASConstants::METER_MAX_COIN_BET;
constexpr int SASConstants::METER_BONUS_WON;
constexpr int SASConstants::METER_PROGRESSIVE_COIN_IN;

} // namespace sas

