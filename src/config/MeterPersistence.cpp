#include "config/MeterPersistence.h"
#include "config/RapidJsonHelper.h"
#include "utils/Logger.h"
#include "sas/SASConstants.h"
#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/prettywriter.h>
#include <fstream>
#include <cstdio>
#include <ctime>
#include <sys/stat.h>

namespace config {

bool MeterPersistence::isSdbootAvailable() {
    struct stat info;
    if (stat("/sdboot", &info) != 0) {
        return false;  // /sdboot doesn't exist
    }
    return (info.st_mode & S_IFDIR) != 0;  // Check if it's a directory
}

std::string MeterPersistence::getMetersPath() {
    if (isSdbootAvailable()) {
        return "/sdboot/meters.json";
    }
    return "meters.json";
}

std::string MeterPersistence::getCurrentTimestamp() {
    std::time_t now = std::time(nullptr);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&now));
    return std::string(buf);
}

bool MeterPersistence::loadMeters(simulator::Machine* machine) {
    if (!machine) {
        utils::Logger::log("[Meters] ERROR: Machine pointer is null");
        return false;
    }

    std::string metersPath = getMetersPath();
    utils::Logger::log("[Meters] Loading meters from: " + metersPath);

    FILE* fp = fopen(metersPath.c_str(), "rb");
    if (!fp) {
        utils::Logger::log("[Meters] No existing meters file found (this is normal for first boot)");
        return false;
    }

    // Read and parse JSON
    char readBuffer[65536];
    rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));
    rapidjson::Document doc;
    doc.ParseStream(is);
    fclose(fp);

    if (doc.HasParseError()) {
        utils::Logger::log("[Meters] ERROR: JSON parse error in meters file");
        return false;
    }

    if (!doc.IsObject()) {
        utils::Logger::log("[Meters] ERROR: Meters file root is not an object");
        return false;
    }

    // Load main machine meters
    if (doc.HasMember("mainMeters") && doc["mainMeters"].IsObject()) {
        const rapidjson::Value& mainMeters = doc["mainMeters"];

        // Load each meter using METER_* codes directly (no more persistence code mapping!)
        auto loadMeter = [&](const char* key, int meterCode) {
            if (mainMeters.HasMember(key) && mainMeters[key].IsInt64()) {
                int64_t value = mainMeters[key].GetInt64();
                machine->setMeter(meterCode, value);
                utils::Logger::log("[Meters]   " + std::string(key) + ": " + std::to_string(value) +
                                   " (METER_* code: 0x" + std::to_string(meterCode) + ")");
            }
        };

        utils::Logger::log("[Meters] Loading main meters:");

        // Machine meters - Doors (use extended METER_* codes 0x100+)
        loadMeter("coinDrop", sas::SASConstants::METER_COIN_DROP);
        loadMeter("slotDoor", sas::SASConstants::METER_SLOT_DOOR);
        loadMeter("dropDoor", sas::SASConstants::METER_DROP_DOOR);
        loadMeter("logicDoor", sas::SASConstants::METER_LOGIC_DOOR);
        loadMeter("cashDoor", sas::SASConstants::METER_CASH_DOOR);
        loadMeter("auxFillDoor", sas::SASConstants::METER_AUX_FILL_DOOR);
        loadMeter("actualSlotDoor", sas::SASConstants::METER_ACTUAL_SLOT_DOOR);
        loadMeter("chassisDoor", sas::SASConstants::METER_CHASSIS_DOOR);

        // Machine meters - Bill denoms (use SAS METER_* codes)
        loadMeter("billsIn1", sas::SASConstants::METER_1_BILLS_ACCEPTED);
        loadMeter("billsIn2", sas::SASConstants::METER_2_BILLS_ACCEPTED);
        loadMeter("billsIn5", sas::SASConstants::METER_5_BILLS_ACCEPTED);
        loadMeter("billsIn10", sas::SASConstants::METER_10_BILLS_ACCEPTED);
        loadMeter("billsIn20", sas::SASConstants::METER_20_BILLS_ACCEPTED);
        loadMeter("billsIn50", sas::SASConstants::METER_50_BILLS_ACCEPTED);
        loadMeter("billsIn100", sas::SASConstants::METER_100_BILLS_ACCEPTED);
        loadMeter("billsIn200", sas::SASConstants::METER_200_BILLS_ACCEPTED);
        loadMeter("billsIn500", sas::SASConstants::METER_500_BILLS_ACCEPTED);
        loadMeter("billsIn1000", sas::SASConstants::METER_1000_BILLS_ACCEPTED);

        // Machine meters - Credits and coins (use SAS or extended METER_* codes)
        loadMeter("credits", sas::SASConstants::METER_CURRENT_CRD);
        loadMeter("trueCoinIn", sas::SASConstants::METER_TRUE_COIN_IN);
        loadMeter("trueCoinOut", sas::SASConstants::METER_TRUE_COIN_OUT);
        loadMeter("billDrop", sas::SASConstants::METER_CRD_FR_BILL_ACCEPTOR);
        loadMeter("totalHandPay", sas::SASConstants::METER_HANDPAID_CANCELLED_CRD);
        loadMeter("actualCoinDrop", sas::SASConstants::METER_ACTUAL_COIN_DROP);
        loadMeter("handPaidCancelledCredits", sas::SASConstants::METER_HANDPAID_CANCELLED_CRD);
        loadMeter("physicalCoinInValue", sas::SASConstants::METER_PHYS_COIN_IN_DOLLAR_VALUE);
        loadMeter("physicalCoinOutValue", sas::SASConstants::METER_PHYS_COIN_OUT_DOLLAR_VALUE);
        loadMeter("totalDrop", sas::SASConstants::METER_TOT_DROP);
        loadMeter("voucherTicketDrop", sas::SASConstants::METER_VOUCHER_TICKET_DROP);
        loadMeter("ncepCredits", sas::SASConstants::METER_NCEP_CREDITS);

        // Machine meters - AFT (use SAS METER_* codes)
        loadMeter("aftCashableToGame", sas::SASConstants::METER_AFT_CASHABLE_IN);
        loadMeter("aftRestrictedToGame", sas::SASConstants::METER_AFT_REST_IN);
        loadMeter("aftNonRestrictedToGame", sas::SASConstants::METER_AFT_IN);
        loadMeter("aftCashableToHost", sas::SASConstants::METER_AFT_CASHABLE_OUT);
        loadMeter("aftRestrictedToHost", sas::SASConstants::METER_AFT_REST_OUT);
        loadMeter("aftNonRestrictedToHost", sas::SASConstants::METER_AFT_OUT);
        loadMeter("aftDebitToGame", sas::SASConstants::METER_AFT_DEBIT_XFER_TO_GAME_VALUE);

        // Machine meters - Bonus and Progressive (use SAS METER_* codes)
        loadMeter("bonusMachinePayout", sas::SASConstants::METER_MACH_PAID_EXT_BONUS);
        loadMeter("bonusAttendantPayout", sas::SASConstants::METER_ATT_PAID_EXT_BONUS);
        loadMeter("progressiveAttendantPayout", sas::SASConstants::METER_ATT_PAID_PROG);
        loadMeter("progressiveMachinePayout", sas::SASConstants::METER_MACH_PAID_PROG);

        // Machine meters - Special (use SAS or extended METER_* codes)
        loadMeter("restrictedPlayed", sas::SASConstants::METER_TOTAL_REST_PLAYED);
        loadMeter("unrestrictedPlayed", sas::SASConstants::METER_TOTAL_NONREST_PLAYED);
        loadMeter("gameWeightedTheoretical", sas::SASConstants::METER_WTPP);

        // Game meters (base game - use SAS or extended METER_* codes)
        loadMeter("coinIn", sas::SASConstants::METER_COIN_IN);
        loadMeter("coinOut", sas::SASConstants::METER_COIN_OUT);
        loadMeter("gamesPlayed", sas::SASConstants::METER_GAMES_PLAYED);
        loadMeter("gamesWon", sas::SASConstants::METER_GAMES_WON);
        loadMeter("maxCoinBet", sas::SASConstants::METER_MAX_COIN_BET);
        loadMeter("cancelledCredits", sas::SASConstants::METER_CANCELLED_CRD);
        loadMeter("bonusWon", sas::SASConstants::METER_BONUS_WON);
        loadMeter("jackpot", sas::SASConstants::METER_JACKPOT);
        loadMeter("progressiveCoinIn", sas::SASConstants::METER_PROGRESSIVE_COIN_IN);
    }

    // Load game-specific meters
    if (doc.HasMember("games") && doc["games"].IsArray()) {
        const rapidjson::Value& gamesArray = doc["games"];
        utils::Logger::log("[Meters] Loading game meters for " + std::to_string(gamesArray.Size()) + " games");

        for (rapidjson::SizeType i = 0; i < gamesArray.Size(); i++) {
            const rapidjson::Value& gameData = gamesArray[i];
            if (!gameData.IsObject()) continue;

            int gameNumber = RapidJsonHelper::GetInt(gameData, "gameNumber", -1);
            if (gameNumber < 0) continue;

            utils::Logger::log("[Meters]   Game " + std::to_string(gameNumber) + ":");

            // Get game meters if they exist
            if (gameData.HasMember("meters") && gameData["meters"].IsObject()) {
                const rapidjson::Value& gameMeters = gameData["meters"];

                // Load game-specific meters
                // TODO: Implement game-specific meter loading when Game class supports it
                // For now, just log that we found them
                utils::Logger::log("[Meters]     Found " + std::to_string(gameMeters.MemberCount()) + " meters");
            }
        }
    }

    // Log last saved timestamp
    if (doc.HasMember("lastSaved") && doc["lastSaved"].IsString()) {
        utils::Logger::log("[Meters] Last saved: " + std::string(doc["lastSaved"].GetString()));
    }

    utils::Logger::log("[Meters] Meters loaded successfully");
    return true;
}

bool MeterPersistence::saveMeters(simulator::Machine* machine) {
    if (!machine) {
        utils::Logger::log("[Meters] ERROR: Machine pointer is null");
        return false;
    }

    std::string metersPath = getMetersPath();
    utils::Logger::log("[Meters] Saving meters to: " + metersPath);

    FILE* fp = fopen(metersPath.c_str(), "wb");
    if (!fp) {
        utils::Logger::log("[Meters] ERROR: Could not open meters file for writing: " + metersPath);
        return false;
    }

    // Create JSON document using RapidJSON Writer
    char writeBuffer[65536];
    rapidjson::FileWriteStream os(fp, writeBuffer, sizeof(writeBuffer));
    rapidjson::PrettyWriter<rapidjson::FileWriteStream> writer(os);

    writer.StartObject();

    // Write main machine meters
    writer.Key("mainMeters");
    writer.StartObject();

    // Save meters using METER_* codes directly (no more persistence code mapping!)
    auto writeMeter = [&](const char* key, int meterCode) {
        writer.Key(key);
        writer.Int64(machine->getMeter(meterCode));
    };

    // Machine meters - Doors (use extended METER_* codes 0x100+)
    writeMeter("coinDrop", sas::SASConstants::METER_COIN_DROP);
    writeMeter("slotDoor", sas::SASConstants::METER_SLOT_DOOR);
    writeMeter("dropDoor", sas::SASConstants::METER_DROP_DOOR);
    writeMeter("logicDoor", sas::SASConstants::METER_LOGIC_DOOR);
    writeMeter("cashDoor", sas::SASConstants::METER_CASH_DOOR);
    writeMeter("auxFillDoor", sas::SASConstants::METER_AUX_FILL_DOOR);
    writeMeter("actualSlotDoor", sas::SASConstants::METER_ACTUAL_SLOT_DOOR);
    writeMeter("chassisDoor", sas::SASConstants::METER_CHASSIS_DOOR);

    // Machine meters - Bill denoms (use SAS METER_* codes)
    writeMeter("billsIn1", sas::SASConstants::METER_1_BILLS_ACCEPTED);
    writeMeter("billsIn2", sas::SASConstants::METER_2_BILLS_ACCEPTED);
    writeMeter("billsIn5", sas::SASConstants::METER_5_BILLS_ACCEPTED);
    writeMeter("billsIn10", sas::SASConstants::METER_10_BILLS_ACCEPTED);
    writeMeter("billsIn20", sas::SASConstants::METER_20_BILLS_ACCEPTED);
    writeMeter("billsIn50", sas::SASConstants::METER_50_BILLS_ACCEPTED);
    writeMeter("billsIn100", sas::SASConstants::METER_100_BILLS_ACCEPTED);
    writeMeter("billsIn200", sas::SASConstants::METER_200_BILLS_ACCEPTED);
    writeMeter("billsIn500", sas::SASConstants::METER_500_BILLS_ACCEPTED);
    writeMeter("billsIn1000", sas::SASConstants::METER_1000_BILLS_ACCEPTED);

    // Machine meters - Credits and coins (use SAS or extended METER_* codes)
    writeMeter("credits", sas::SASConstants::METER_CURRENT_CRD);
    writeMeter("trueCoinIn", sas::SASConstants::METER_TRUE_COIN_IN);
    writeMeter("trueCoinOut", sas::SASConstants::METER_TRUE_COIN_OUT);
    writeMeter("billDrop", sas::SASConstants::METER_CRD_FR_BILL_ACCEPTOR);
    writeMeter("totalHandPay", sas::SASConstants::METER_HANDPAID_CANCELLED_CRD);
    writeMeter("actualCoinDrop", sas::SASConstants::METER_ACTUAL_COIN_DROP);
    writeMeter("handPaidCancelledCredits", sas::SASConstants::METER_HANDPAID_CANCELLED_CRD);
    writeMeter("physicalCoinInValue", sas::SASConstants::METER_PHYS_COIN_IN_DOLLAR_VALUE);
    writeMeter("physicalCoinOutValue", sas::SASConstants::METER_PHYS_COIN_OUT_DOLLAR_VALUE);
    writeMeter("totalDrop", sas::SASConstants::METER_TOT_DROP);
    writeMeter("voucherTicketDrop", sas::SASConstants::METER_VOUCHER_TICKET_DROP);
    writeMeter("ncepCredits", sas::SASConstants::METER_NCEP_CREDITS);

    // Machine meters - AFT (use SAS METER_* codes)
    writeMeter("aftCashableToGame", sas::SASConstants::METER_AFT_CASHABLE_IN);
    writeMeter("aftRestrictedToGame", sas::SASConstants::METER_AFT_REST_IN);
    writeMeter("aftNonRestrictedToGame", sas::SASConstants::METER_AFT_IN);
    writeMeter("aftCashableToHost", sas::SASConstants::METER_AFT_CASHABLE_OUT);
    writeMeter("aftRestrictedToHost", sas::SASConstants::METER_AFT_REST_OUT);
    writeMeter("aftNonRestrictedToHost", sas::SASConstants::METER_AFT_OUT);
    writeMeter("aftDebitToGame", sas::SASConstants::METER_AFT_DEBIT_XFER_TO_GAME_VALUE);

    // Machine meters - Bonus and Progressive (use SAS METER_* codes)
    writeMeter("bonusMachinePayout", sas::SASConstants::METER_MACH_PAID_EXT_BONUS);
    writeMeter("bonusAttendantPayout", sas::SASConstants::METER_ATT_PAID_EXT_BONUS);
    writeMeter("progressiveAttendantPayout", sas::SASConstants::METER_ATT_PAID_PROG);
    writeMeter("progressiveMachinePayout", sas::SASConstants::METER_MACH_PAID_PROG);

    // Machine meters - Special (use SAS or extended METER_* codes)
    writeMeter("restrictedPlayed", sas::SASConstants::METER_TOTAL_REST_PLAYED);
    writeMeter("unrestrictedPlayed", sas::SASConstants::METER_TOTAL_NONREST_PLAYED);
    writeMeter("gameWeightedTheoretical", sas::SASConstants::METER_WTPP);

    // Game meters (base game - use SAS or extended METER_* codes)
    writeMeter("coinIn", sas::SASConstants::METER_COIN_IN);
    writeMeter("coinOut", sas::SASConstants::METER_COIN_OUT);
    writeMeter("gamesPlayed", sas::SASConstants::METER_GAMES_PLAYED);
    writeMeter("gamesWon", sas::SASConstants::METER_GAMES_WON);
    writeMeter("maxCoinBet", sas::SASConstants::METER_MAX_COIN_BET);
    writeMeter("cancelledCredits", sas::SASConstants::METER_CANCELLED_CRD);
    writeMeter("bonusWon", sas::SASConstants::METER_BONUS_WON);
    writeMeter("jackpot", sas::SASConstants::METER_JACKPOT);
    writeMeter("progressiveCoinIn", sas::SASConstants::METER_PROGRESSIVE_COIN_IN);

    writer.EndObject();

    // Write game-specific meters
    writer.Key("games");
    writer.StartArray();

    auto games = machine->getGames();
    for (const auto& game : games) {
        writer.StartObject();

        writer.Key("gameNumber");
        writer.Int(game->getGameNumber());

        writer.Key("meters");
        writer.StartObject();
        // TODO: Add game-specific meters when Game class supports them
        writer.EndObject();

        writer.EndObject();
    }

    writer.EndArray();

    // Write timestamp
    writer.Key("lastSaved");
    writer.String(getCurrentTimestamp().c_str());

    writer.EndObject();

    fclose(fp);

    utils::Logger::log("[Meters] Meters saved successfully");
    utils::Logger::log("[Meters]   Coin In: " + std::to_string(machine->getMeter(sas::SASConstants::METER_COIN_IN)));
    utils::Logger::log("[Meters]   Coin Out: " + std::to_string(machine->getMeter(sas::SASConstants::METER_COIN_OUT)));
    utils::Logger::log("[Meters]   Credits: " + std::to_string(machine->getMeter(sas::SASConstants::METER_CURRENT_CRD)));

    return true;
}

// NOTE: liveToPersistence() and persistenceToLive() mapping functions have been removed.
// We now use METER_* codes directly everywhere (runtime and persistence).
// No more mD*/gCI* persistence codes!

} // namespace config
