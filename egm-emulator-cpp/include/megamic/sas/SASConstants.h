#ifndef MEGAMIC_SAS_SASCONSTANTS_H
#define MEGAMIC_SAS_SASCONSTANTS_H

#include <cstdint>
#include <map>

namespace megamic {
namespace sas {

/**
 * SAS Protocol Constants
 * This is a partial C++ port of SASConstants.java
 */
class SASConstants {
public:
    // Meter codes (from Machine.java usage)
    static constexpr int MTR_COIN_IN = 0x00;
    static constexpr int MTR_COIN_OUT = 0x01;
    static constexpr int MTR_JACKPOT = 0x02;
    static constexpr int MTR_HANDPAID_CANCELLED_CRD = 0x03;
    static constexpr int MTR_CANCELLED_CRD = 0x04;
    static constexpr int MTR_GAMES_PLAYED = 0x05;
    static constexpr int MTR_GAMES_WON = 0x06;
    static constexpr int MTR_GAMES_LOST = 0x07;
    static constexpr int MTR_CRD_FR_COIN_ACCEPTOR = 0x08;
    static constexpr int MTR_CRD_PAID_FR_HOPPER = 0x09;
    static constexpr int MTR_CRD_FR_COIN_TO_DROP = 0x0A;
    static constexpr int MTR_CRD_FR_BILL_ACCEPTOR = 0x0B;
    static constexpr int MTR_CURRENT_CRD = 0x0C;
    static constexpr int MTR_TOT_TKT_IN = 0x0D;
    static constexpr int MTR_TOT_TKT_OUT = 0x0E;
    static constexpr int MTR_TOT_DROP = 0x0F;
    static constexpr int MTR_REG_CASH_TKT_IN = 0x10;
    static constexpr int MTR_REST_PROMO_TKT_IN = 0x11;
    static constexpr int MTR_1_BILLS_ACCEPTED = 0x12;
    static constexpr int MTR_5_BILLS_ACCEPTED = 0x13;
    static constexpr int MTR_10_BILLS_ACCEPTED = 0x14;
    static constexpr int MTR_20_BILLS_ACCEPTED = 0x15;
    static constexpr int MTR_50_BILLS_ACCEPTED = 0x16;
    static constexpr int MTR_100_BILLS_ACCEPTED = 0x17;
    static constexpr int MTR_CURRENT_REST_CRD = 0x18;
    static constexpr int MTR_CURRENT_NON_REST_CRD = 0x19;
    static constexpr int MTR_AFT_IN = 0x1A;
    static constexpr int MTR_AFT_OUT = 0x1B;
    static constexpr int METER_TICKET_OUT = 0x0E;  // Alias for ticket out meter
    static constexpr int METER_AFT_IN = 0x1A;      // AFT credits in
    static constexpr int METER_AFT_OUT = 0x1B;     // AFT credits out

    /**
     * Denomination mapping helper
     */
    class Denominations {
    public:
        Denominations();

        /**
         * Get denomination value from code
         * @param denomCode SAS denomination code
         * @return Denomination in dollars
         */
        double getDenomination(int denomCode) const;

        /**
         * Get denomination code from value
         * @param denomination Denomination in dollars
         * @return SAS denomination code, or -1 if not found
         */
        int getDenomCodeByDenomination(double denomination) const;

    private:
        std::map<int, double> codeToValue_;
        std::map<double, int> valueToCode_;
    };

    static const Denominations DENOMINATIONS;
};

} // namespace sas
} // namespace megamic

#endif // MEGAMIC_SAS_SASCONSTANTS_H
