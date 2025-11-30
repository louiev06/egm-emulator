#ifndef SAS_SASCONSTANTS_H
#define SAS_SASCONSTANTS_H

#include <cstdint>
#include <map>


namespace sas {

/**
 * SAS Protocol Constants
 * This is a partial C++ port of SASConstants.java
 */
class SASConstants {
public:
    // SAS Meter codes from Table C-7 (SAS Protocol Version 6.03)
    // Code range 0000-007F: Standard meters
    static constexpr int METER_COIN_IN = 0x00;                      // Total coin in credits
    static constexpr int METER_COIN_OUT = 0x01;                     // Total coin out credits
    static constexpr int METER_JACKPOT = 0x02;                      // Total jackpot credits
    static constexpr int METER_HANDPAID_CANCELLED_CRD = 0x03;       // Total hand paid cancelled credits
    static constexpr int METER_CANCELLED_CRD = 0x04;                // Total cancelled credits
    static constexpr int METER_GAMES_PLAYED = 0x05;                 // Games played
    static constexpr int METER_GAMES_WON = 0x06;                    // Games won
    static constexpr int METER_GAMES_LOST = 0x07;                   // Games lost
    static constexpr int METER_CRD_FR_COIN_ACCEPTOR = 0x08;         // Total credits from coin acceptor
    static constexpr int METER_CRD_PAID_FR_HOPPER = 0x09;           // Total credits paid from hopper
    static constexpr int METER_CRD_FR_COIN_TO_DROP = 0x0A;          // Total credits from coins to drop
    static constexpr int METER_CRD_FR_BILL_ACCEPTOR = 0x0B;         // Total credits from bills accepted
    static constexpr int METER_CURRENT_CRD = 0x0C;                  // Current credits
    static constexpr int METER_TOT_TKT_IN = 0x0D;                   // Total SAS cashable ticket in, including nonrestricted tickets (cents) [same as meters 0080 + 0084]
    static constexpr int METER_TOT_TKT_OUT = 0x0E;                  // Total SAS cashable ticket out, including debit tickets (cents) [same as meters 0086 + 008A]
    static constexpr int METER_TOT_DROP = 0x0F;                     // Total SAS restricted ticket in (cents) [same as meter 0082]
    static constexpr int METER_REST_TKT_OUT = 0x10;                 // Total SAS restricted ticket out (cents) [same as meter 0088]
    static constexpr int METER_TOT_TKT_IN_QTY = 0x11;               // Total SAS cashable ticket in, including nonrestricted tickets (quantity) [same as meters 0081 + 0085]
    static constexpr int METER_TOT_TKT_OUT_QTY = 0x12;              // Total SAS cashable ticket out, including debit tickets (quantity) [same as meters 0087 + 008B]
    static constexpr int METER_REST_TKT_IN_QTY = 0x13;              // Total SAS restricted ticket in (quantity) [same as meter 0083]
    static constexpr int METER_REST_TKT_OUT_QTY_0014 = 0x14;        // Total SAS restricted ticket out (quantity) [same as meter 0089]
    static constexpr int METER_TOT_TKT_IN_INCL_CASHABLE_NONREST = 0x15; // Total ticket in, including cashable, nonrestricted and restricted tickets (credits)
    static constexpr int METER_TOT_TKT_OUT_INCL_CASHABLE_NONREST = 0x16; // Total ticket out, including cashable, nonrestricted, restricted and debit tickets (credits)
    static constexpr int METER_ELEC_XFER_TO_GAME = 0x17;            // Total electronic transfers to gaming machine, including cashable, nonrestricted, restricted and debit, whether transfer is to credit meter or to ticket (credits)
    static constexpr int METER_ELEC_XFER_TO_HOST = 0x18;            // Total electronic transfers to host, including cashable, nonrestricted, restricted and debit amounts (credits)
    static constexpr int METER_TOTAL_REST_PLAYED = 0x19;            // Total restricted amount played (credits)
    static constexpr int METER_TOTAL_NONREST_PLAYED = 0x1A;         // Total nonrestricted amount played (credits)
    static constexpr int METER_CURRENT_REST_CRD = 0x1B;             // Current restricted credits
    static constexpr int METER_MACH_PAID_PAYTABLE = 0x1C;           // Total machine paid paytable win, not including progressive or external bonus amounts (credits)
    static constexpr int METER_MACH_PAID_PROG = 0x1D;               // Total machine paid progressive win (credits)
    static constexpr int METER_MACH_PAID_EXT_BONUS = 0x1E;          // Total machine paid external bonus win (credits)
    static constexpr int METER_ATT_PAID_PAYTABLE = 0x1F;            // Total attendant paid paytable win, not including progressive or external bonus amounts (credits)
    static constexpr int METER_ATT_PAID_PROG = 0x20;                // Total attendant paid progressive win (credits)
    static constexpr int METER_ATT_PAID_EXT_BONUS = 0x21;           // Total attendant paid external bonus win (credits)
    static constexpr int METER_TOTAL_WON = 0x22;                    // Total won credits (sum of total coin out and total jackpot)
    static constexpr int METER_TOTAL_HANDPAID = 0x23;               // Total hand paid credits (sum of total hand paid cancelled credits and total jackpot)
    static constexpr int METER_TOTAL_DROP_INCL_BILLS = 0x24;        // Total drop, including but not limited to coins to drop, bills to drop, tickets to drop, and electronic drop (credits)
    static constexpr int METER_GAMES_SINCE_POWER_RESET = 0x25;      // Games since last power reset
    static constexpr int METER_GAMES_SINCE_DOOR_CLOSE = 0x26;       // Games since slot door closure
    static constexpr int METER_TOTAL_EXT_COIN_ACCEPTOR = 0x27;      // Total credits from external coin acceptor
    static constexpr int METER_CASHABLE_TKT_IN = 0x28;              // Total cashable ticket in, including nonrestricted promotional tickets (credits)
    static constexpr int METER_REG_CASHABLE_TKT_IN = 0x29;          // Total regular cashable ticket in (credits)
    static constexpr int METER_REST_PROMO_TKT_IN = 0x2A;            // Total restricted promotional ticket in (credits)
    static constexpr int METER_NONREST_PROMO_TKT_IN = 0x2B;         // Total nonrestricted promotional ticket in (credits)
    static constexpr int METER_CASHABLE_TKT_OUT = 0x2C;             // Total cashable ticket out, including debit tickets (credits)
    static constexpr int METER_REST_PROMO_TKT_OUT = 0x2D;           // Total restricted promotional ticket out (credits)
    static constexpr int METER_ELEC_REG_CASHABLE_TO_GAME = 0x2E;    // Electronic regular cashable transfers to gaming machine, not including external bonus awards (credits)
    static constexpr int METER_ELEC_REST_PROMO_TO_GAME = 0x2F;      // Electronic restricted promotional transfers to gaming machine, not including external bonus awards (credits)
    static constexpr int METER_ELEC_NONREST_PROMO_TO_GAME = 0x30;   // Electronic nonrestricted promotional transfers to gaming machine, not including external bonus awards (credits)
    static constexpr int METER_ELEC_DEBIT_TO_GAME = 0x31;           // Electronic debit transfers to gaming machine (credits)
    static constexpr int METER_ELEC_REG_CASHABLE_TO_HOST = 0x32;    // Electronic regular cashable transfers to host (credits)
    static constexpr int METER_ELEC_REST_PROMO_TO_HOST = 0x33;      // Electronic restricted promotional transfers to host (credits)
    static constexpr int METER_ELEC_NONREST_PROMO_TO_HOST = 0x34;   // Electronic nonrestricted promotional transfers to host (credits)
    static constexpr int METER_REG_CASHABLE_TKT_IN_QTY = 0x35;      // Total regular cashable ticket in (quantity)
    static constexpr int METER_REST_PROMO_TKT_IN_QTY = 0x36;        // Total restricted promotional ticket in (quantity)
    static constexpr int METER_NONREST_PROMO_TKT_IN_QTY = 0x37;     // Total nonrestricted promotional ticket in (quantity)
    static constexpr int METER_CASHABLE_TKT_OUT_QTY = 0x38;         // Total cashable ticket out, including debit tickets (quantity)
    static constexpr int METER_REST_PROMO_TKT_OUT_QTY = 0x39;       // Total restricted promotional ticket out (quantity)
    // 0x3A-0x3D: Reserved for future use
    static constexpr int METER_NUM_BILLS_IN_STACKER = 0x3E;         // Number of bills currently in the stacker (Issue exception 7B when this meter is reset)
    static constexpr int METER_TOTAL_VALUE_BILLS_IN_STACKER = 0x3F; // Total value of bills currently in the stacker (credits) (Issue exception 7B when this meter is reset)

    // Code range 0x40-0x57: Bill denomination acceptance meters
    static constexpr int METER_1_BILLS_ACCEPTED = 0x40;             // Total number of $1.00 bills accepted
    static constexpr int METER_2_BILLS_ACCEPTED = 0x41;             // Total number of $2.00 bills accepted
    static constexpr int METER_5_BILLS_ACCEPTED = 0x42;             // Total number of $5.00 bills accepted
    static constexpr int METER_10_BILLS_ACCEPTED = 0x43;            // Total number of $10.00 bills accepted
    static constexpr int METER_20_BILLS_ACCEPTED = 0x44;            // Total number of $20.00 bills accepted
    static constexpr int METER_25_BILLS_ACCEPTED = 0x45;            // Total number of $25.00 bills accepted
    static constexpr int METER_50_BILLS_ACCEPTED = 0x46;            // Total number of $50.00 bills accepted
    static constexpr int METER_100_BILLS_ACCEPTED = 0x47;           // Total number of $100.00 bills accepted
    static constexpr int METER_200_BILLS_ACCEPTED = 0x48;           // Total number of $200.00 bills accepted
    static constexpr int METER_250_BILLS_ACCEPTED = 0x49;           // Total number of $250.00 bills accepted
    static constexpr int METER_500_BILLS_ACCEPTED = 0x4A;           // Total number of $500.00 bills accepted
    static constexpr int METER_1000_BILLS_ACCEPTED = 0x4B;          // Total number of $1,000.00 bills accepted
    static constexpr int METER_2000_BILLS_ACCEPTED = 0x4C;          // Total number of $2,000.00 bills accepted
    static constexpr int METER_2500_BILLS_ACCEPTED = 0x4D;          // Total number of $2,500.00 bills accepted
    static constexpr int METER_5000_BILLS_ACCEPTED = 0x4E;          // Total number of $5,000.00 bills accepted
    static constexpr int METER_10000_BILLS_ACCEPTED = 0x4F;         // Total number of $10,000.00 bills accepted
    static constexpr int METER_20000_BILLS_ACCEPTED = 0x50;         // Total number of $20,000.00 bills accepted
    static constexpr int METER_25000_BILLS_ACCEPTED = 0x51;         // Total number of $25,000.00 bills accepted
    static constexpr int METER_50000_BILLS_ACCEPTED = 0x52;         // Total number of $50,000.00 bills accepted
    static constexpr int METER_100000_BILLS_ACCEPTED = 0x53;        // Total number of $100,000.00 bills accepted
    static constexpr int METER_200000_BILLS_ACCEPTED = 0x54;        // Total number of $200,000.00 bills accepted
    static constexpr int METER_250000_BILLS_ACCEPTED = 0x55;        // Total number of $250,000.00 bills accepted
    static constexpr int METER_500000_BILLS_ACCEPTED = 0x56;        // Total number of $500,000.00 bills accepted
    static constexpr int METER_1000000_BILLS_ACCEPTED = 0x57;       // Total number of $1,000,000.00 bills accepted

    // Code range 0x58-0x7F: Additional bill and specialized meters
    static constexpr int METER_CREDITS_FROM_BILLS_TO_DROP = 0x58;   // Total credits from bills to drop
    static constexpr int METER_1_BILLS_TO_DROP = 0x59;              // Total number of $1.00 bills to drop
    static constexpr int METER_2_BILLS_TO_DROP = 0x5A;              // Total number of $2.00 bills to drop
    static constexpr int METER_5_BILLS_TO_DROP = 0x5B;              // Total number of $5.00 bills to drop
    static constexpr int METER_10_BILLS_TO_DROP = 0x5C;             // Total number of $10.00 bills to drop
    static constexpr int METER_20_BILLS_TO_DROP = 0x5D;             // Total number of $20.00 bills to drop
    static constexpr int METER_50_BILLS_TO_DROP = 0x5E;             // Total number of $50.00 bills to drop
    static constexpr int METER_100_BILLS_TO_DROP = 0x5F;            // Total number of $100.00 bills to drop
    static constexpr int METER_200_BILLS_TO_DROP = 0x60;            // Total number of $200.00 bills to drop
    static constexpr int METER_500_BILLS_TO_DROP = 0x61;            // Total number of $500.00 bills to drop
    static constexpr int METER_1000_BILLS_TO_DROP = 0x62;           // Total number of $1000.00 bills to drop
    static constexpr int METER_CREDITS_FROM_BILLS_TO_HOPPER = 0x63; // Total credits from bills diverted to hopper
    static constexpr int METER_1_BILLS_TO_HOPPER = 0x64;            // Total number of $1.00 bills diverted to hopper
    static constexpr int METER_2_BILLS_TO_HOPPER = 0x65;            // Total number of $2.00 bills diverted to hopper
    static constexpr int METER_5_BILLS_TO_HOPPER = 0x66;            // Total number of $5.00 bills diverted to hopper
    static constexpr int METER_10_BILLS_TO_HOPPER = 0x67;           // Total number of $10.00 bills diverted to hopper
    static constexpr int METER_20_BILLS_TO_HOPPER = 0x68;           // Total number of $20.00 bills diverted to hopper
    static constexpr int METER_50_BILLS_TO_HOPPER = 0x69;           // Total number of $50.00 bills diverted to hopper
    static constexpr int METER_100_BILLS_TO_HOPPER = 0x6A;          // Total number of $100.00 bills diverted to hopper
    static constexpr int METER_200_BILLS_TO_HOPPER = 0x6B;          // Total number of $200.00 bills diverted to hopper
    static constexpr int METER_500_BILLS_TO_HOPPER = 0x6C;          // Total number of $500.00 bills diverted to hopper
    static constexpr int METER_1000_BILLS_TO_HOPPER = 0x6D;         // Total number of $1000.00 bills diverted to hopper
    static constexpr int METER_CREDITS_FROM_BILLS_DISPENSED = 0x6E; // Total credits from bills dispensed from hopper
    static constexpr int METER_1_BILLS_DISPENSED_FROM_HOPPER = 0x6F; // Total number of $1.00 bills dispensed from hopper
    static constexpr int METER_2_BILLS_DISPENSED_FROM_HOPPER = 0x70; // Total number of $2.00 bills dispensed from hopper
    static constexpr int METER_5_BILLS_DISPENSED_FROM_HOPPER = 0x71; // Total number of $5.00 bills dispensed from hopper
    static constexpr int METER_10_BILLS_DISPENSED_FROM_HOPPER = 0x72; // Total number of $10.00 bills dispensed from hopper
    static constexpr int METER_20_BILLS_DISPENSED_FROM_HOPPER = 0x73; // Total number of $20.00 bills dispensed from hopper
    static constexpr int METER_50_BILLS_DISPENSED_FROM_HOPPER = 0x74; // Total number of $50.00 bills dispensed from hopper
    static constexpr int METER_100_BILLS_DISPENSED_FROM_HOPPER = 0x75; // Total number of $100.00 bills dispensed from hopper
    static constexpr int METER_200_BILLS_DISPENSED_FROM_HOPPER = 0x76; // Total number of $200.00 bills dispensed from hopper
    static constexpr int METER_500_BILLS_DISPENSED_FROM_HOPPER = 0x77; // Total number of $500.00 bills dispensed from hopper
    static constexpr int METER_1000_BILLS_DISPENSED_FROM_HOPPER = 0x78; // Total number of $1000.00 bills dispensed from hopper
    static constexpr int METER_SESSIONS_PLAYED = 0x79;              // Sessions played (a session is a set of individual games purchased as a group and played in a series)
    static constexpr int METER_TIP_MONEY = 0x7A;                    // Tip money (credits)
    static constexpr int METER_FOREIGN_BILL_AMOUNT = 0x7B;          // Total foreign bill converted amount (cents)
    static constexpr int METER_FOREIGN_BILL_COUNT = 0x7C;           // Total foreign bill count (quantity)
    // 0x7D-0x7E: Reserved for future use
    static constexpr int METER_WTPP = 0x7F;                         // Weighted average theoretical payback percentage in hundredths of a percent (read below)

    // Code range 0x80-0x9F: SAS Validation-Specific meters
    static constexpr int METER_REG_CASHABLE_TKT_IN_CENTS = 0x80;    // Regular cashable ticket in (cents)
    static constexpr int METER_REG_CASHABLE_TKT_IN_QTY_0081 = 0x81; // Regular cashable ticket in (quantity)
    static constexpr int METER_REST_TKT_IN_CENTS = 0x82;            // Restricted ticket in (cents)
    static constexpr int METER_REST_TKT_IN_QTY_0083 = 0x83;         // Restricted ticket in (quantity)
    static constexpr int METER_NONREST_TKT_IN_CENTS = 0x84;         // Nonrestricted ticket in (cents)
    static constexpr int METER_NONREST_TKT_IN_QTY_0085 = 0x85;      // Nonrestricted ticket in (quantity)
    static constexpr int METER_REG_CASHABLE_TKT_OUT_CENTS = 0x86;   // Regular cashable ticket out (cents)
    static constexpr int METER_REG_CASHABLE_TKT_OUT_QTY_0087 = 0x87; // Regular cashable ticket out (quantity)
    static constexpr int METER_REST_TKT_OUT_CENTS = 0x88;           // Restricted ticket out (cents)
    static constexpr int METER_REST_TKT_OUT_QTY_0089 = 0x89;        // Restricted ticket out (quantity)
    static constexpr int METER_DEBIT_TKT_OUT_CENTS = 0x8A;          // Debit ticket out (cents)
    static constexpr int METER_DEBIT_TKT_OUT_QTY = 0x8B;            // Debit ticket out (quantity)
    static constexpr int METER_VALIDATED_CANCELLED_CREDIT_HANDPAY_RECEIPT_CENTS = 0x8C; // Validated cancelled credit handpay, receipt printed (cents)
    static constexpr int METER_VALIDATED_CANCELLED_CREDIT_HANDPAY_RECEIPT_QTY = 0x8D; // Validated cancelled credit handpay, receipt printed (quantity)
    static constexpr int METER_VALIDATED_JACKPOT_HANDPAY_RECEIPT_CENTS = 0x8E; // Validated jackpot handpay, receipt printed (cents)
    static constexpr int METER_VALIDATED_JACKPOT_HANDPAY_RECEIPT_QTY = 0x8F; // Validated jackpot handpay, receipt printed (quantity)
    static constexpr int METER_VALIDATED_CANCELLED_CREDIT_HANDPAY_NO_RECEIPT_CENTS = 0x90; // Validated cancelled credit handpay, no receipt (cents)
    static constexpr int METER_VALIDATED_CANCELLED_CREDIT_HANDPAY_NO_RECEIPT_QTY = 0x91; // Validated cancelled credit handpay, no receipt (quantity)
    static constexpr int METER_VALIDATED_JACKPOT_HANDPAY_NO_RECEIPT_CENTS = 0x92; // Validated jackpot handpay, no receipt (cents)
    static constexpr int METER_VALIDATED_JACKPOT_HANDPAY_NO_RECEIPT_QTY = 0x93; // Validated jackpot handpay, no receipt (quantity)
    // 0x94-0x9F: Reserved for future use

    // Code range 0xA0-0xBD: SAS AFT-Specific meters
    static constexpr int METER_IN_HOUSE_CASHABLE_TO_GAME_CENTS = 0xA0; // In-house cashable transfers to gaming machine (cents)
    static constexpr int METER_IN_HOUSE_CASHABLE_TO_GAME_QTY = 0xA1; // In-house transfers to gaming machine that included cashable amounts (quantity)
    static constexpr int METER_IN_HOUSE_REST_TO_GAME_CENTS = 0xA2;  // In-house restricted transfers to gaming machine (cents)
    static constexpr int METER_IN_HOUSE_REST_TO_GAME_QTY = 0xA3;    // In-house transfers to gaming machine that included restricted amounts (quantity)
    static constexpr int METER_IN_HOUSE_NONREST_TO_GAME_CENTS = 0xA4; // In-house nonrestricted transfers to gaming machine (cents)
    static constexpr int METER_IN_HOUSE_NONREST_TO_GAME_QTY = 0xA5; // In-house transfers to gaming machine that included nonrestricted amounts (quantity)
    static constexpr int METER_DEBIT_TO_GAME_CENTS = 0xA6;          // Debit transfers to gaming machine (cents)
    static constexpr int METER_DEBIT_TO_GAME_QTY = 0xA7;            // Debit transfers to gaming machine (quantity)
    static constexpr int METER_IN_HOUSE_CASHABLE_TO_TICKET_CENTS = 0xA8; // In-house cashable transfers to ticket (cents)
    static constexpr int METER_IN_HOUSE_CASHABLE_TO_TICKET_QTY = 0xA9; // In-house cashable transfers to ticket (quantity)
    static constexpr int METER_IN_HOUSE_REST_TO_TICKET_CENTS = 0xAA; // In-house restricted transfers to ticket (cents)
    static constexpr int METER_IN_HOUSE_REST_TO_TICKET_QTY = 0xAB;  // In-house restricted transfers to ticket (quantity)
    static constexpr int METER_DEBIT_TO_TICKET_CENTS = 0xAC;        // Debit transfers to ticket (cents)
    static constexpr int METER_DEBIT_TO_TICKET_QTY = 0xAD;          // Debit transfers to ticket (quantity)
    static constexpr int METER_BONUS_CASHABLE_TO_GAME_CENTS = 0xAE; // Bonus cashable transfers to gaming machine (cents)
    static constexpr int METER_BONUS_CASHABLE_TO_GAME_QTY = 0xAF;   // Bonus transfers to gaming machine that included cashable amounts (quantity)
    static constexpr int METER_BONUS_NONREST_TO_GAME_CENTS = 0xB0;  // Bonus nonrestricted transfers to gaming machine (cents)
    static constexpr int METER_BONUS_NONREST_TO_GAME_QTY = 0xB1;    // Bonus transfers to gaming machine that included nonrestricted amounts (quantity)
    static constexpr int METER_IN_HOUSE_CASHABLE_TO_HOST_CENTS = 0xB8; // In-house cashable transfers to host (cents)
    static constexpr int METER_IN_HOUSE_CASHABLE_TO_HOST_QTY = 0xB9; // In-house transfers to host that included cashable amounts (quantity)
    static constexpr int METER_IN_HOUSE_REST_TO_HOST_CENTS = 0xBA;  // In-house restricted transfers to host (cents)
    static constexpr int METER_IN_HOUSE_REST_TO_HOST_QTY = 0xBB;    // In-house transfers to host that included restricted amounts (quantity)
    static constexpr int METER_IN_HOUSE_NONREST_TO_HOST_CENTS = 0xBC; // In-house nonrestricted transfers to host (cents)
    static constexpr int METER_IN_HOUSE_NONREST_TO_HOST_QTY = 0xBD; // In-house transfers to host that included nonrestricted amounts (quantity)
    // 0xBE-0xF9: Reserved for future use

    // Code range 0xFA-0xFF: Keyed-on/off funds meters
    static constexpr int METER_REG_CASHABLE_KEYED_ON = 0xFA;        // Regular cashable keyed-on funds (credits)
    static constexpr int METER_REST_PROMO_KEYED_ON = 0xFB;          // Restricted promotional keyed-on funds (credits)
    static constexpr int METER_NONREST_PROMO_KEYED_ON = 0xFC;       // Nonrestricted promotional keyed-on funds (credits)
    static constexpr int METER_REG_CASHABLE_KEYED_OFF = 0xFD;       // Regular cashable keyed-off funds (credits)
    static constexpr int METER_REST_PROMO_KEYED_OFF = 0xFE;         // Restricted promotional keyed-off funds (credits)
    static constexpr int METER_NONREST_PROMO_KEYED_OFF = 0xFF;      // Nonrestricted promotional keyed-off funds (credits)

    // Legacy aliases for backward compatibility
    static constexpr int METER_TICKET_OUT = METER_TOT_TKT_OUT;      // Alias for total ticket out

    // Extended METER_* codes (0x100+) for nCompass persistence-only meters
    // These codes have no SAS protocol equivalent but are used for persistence
    // They are placed in the 0x100+ range to avoid collision with SAS codes (0x00-0xFF)
    static constexpr int METER_COIN_DROP = 0x100;              // Coin drop meter (mCD)
    static constexpr int METER_SLOT_DOOR = 0x101;              // Slot door access (mSDr)
    static constexpr int METER_DROP_DOOR = 0x102;              // Drop door access (mDDr)
    static constexpr int METER_LOGIC_DOOR = 0x103;             // Logic door access (mLDr)
    static constexpr int METER_CASH_DOOR = 0x104;              // Cash door access (mCDr)
    static constexpr int METER_AUX_FILL_DOOR = 0x105;          // Aux fill door access (mFDr)
    static constexpr int METER_ACTUAL_SLOT_DOOR = 0x106;       // Actual slot door (mActualSlotDr)
    static constexpr int METER_CHASSIS_DOOR = 0x107;           // Chassis door access (mChassisDoor)
    static constexpr int METER_TRUE_COIN_IN = 0x108;           // True coin in (mTCi)
    static constexpr int METER_TRUE_COIN_OUT = 0x109;          // True coin out (mTCo)
    static constexpr int METER_ACTUAL_COIN_DROP = 0x10A;       // Actual coin drop (mActualCD)
    static constexpr int METER_PHYS_COIN_IN_DOLLAR_VALUE = 0x10B;  // Physical coin in dollar value (mTValueCI)
    static constexpr int METER_PHYS_COIN_OUT_DOLLAR_VALUE = 0x10C; // Physical coin out dollar value (mTValueCO)
    static constexpr int METER_VOUCHER_TICKET_DROP = 0x10D;    // Voucher/ticket drop (mVchrTktDrop)
    static constexpr int METER_NCEP_CREDITS = 0x10E;           // NCEP credits (mNcepCredits)
    static constexpr int METER_MAX_COIN_BET = 0x10F;           // Max coin bet (gMC)
    static constexpr int METER_BONUS_WON = 0x110;              // Bonus won (gBW)
    static constexpr int METER_PROGRESSIVE_COIN_IN = 0x111;    // Progressive coin in (gPI)

    // NOTE: Legacy mD*/gCI* persistence codes (mCD, mSDr, gCI, etc.) have been removed.
    // These codes are now ONLY used in MeterPersistence for JSON serialization.
    // Runtime code must use ONLY METER_* codes (0x00-0xFF for SAS, 0x100+ for extended).

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


#endif // SAS_SASCONSTANTS_H
