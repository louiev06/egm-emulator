#include "megamic/sas/SASConstants.h"
#include <cmath>

namespace megamic {
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

} // namespace sas
} // namespace megamic
