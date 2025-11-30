#include "config/EGMConfig.h"
#include "config/RapidJsonHelper.h"
#include "utils/Logger.h"
#include <rapidjson/filereadstream.h>
#include <fstream>
#include <sstream>
#include <cstdio>

namespace config {

// Static member initialization
rapidjson::Document EGMConfig::document_;
bool EGMConfig::loaded_ = false;

bool EGMConfig::load(const std::string& configPath) {
    std::string pathToTry;

    // If no path provided, try /sdboot first, then local
    if (configPath.empty()) {
        // Try persistent storage first
        pathToTry = "/sdboot/egm-config.json";
        std::ifstream testFile(pathToTry);
        if (!testFile.good()) {
            // Fall back to local
            pathToTry = "egm-config.json";
        }
    } else {
        pathToTry = configPath;
    }

    utils::Logger::log("[Config] Attempting to load: " + pathToTry);

    FILE* fp = fopen(pathToTry.c_str(), "rb");
    if (!fp) {
        utils::Logger::log("[Config] ERROR: Could not open config file: " + pathToTry);
        return false;
    }

    // Read file using RapidJSON FileReadStream
    char readBuffer[65536];
    rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));

    // Parse JSON
    document_.ParseStream(is);
    fclose(fp);

    if (document_.HasParseError()) {
        utils::Logger::log("[Config] ERROR: JSON parse error at offset " +
                          std::to_string(document_.GetErrorOffset()) +
                          ": " + std::to_string(document_.GetParseError()));
        return false;
    }

    if (!document_.IsObject()) {
        utils::Logger::log("[Config] ERROR: Root element is not an object");
        return false;
    }

    loaded_ = true;
    utils::Logger::log("[Config] Successfully loaded configuration from: " + pathToTry);
    return true;
}

const rapidjson::Document* EGMConfig::getDocument() {
    return loaded_ ? &document_ : nullptr;
}

const rapidjson::Value* EGMConfig::navigateToValue(const std::string& key) {
    if (!loaded_) {
        return nullptr;
    }

    // Split key by dots (e.g., "aft.transferLimit" -> ["aft", "transferLimit"])
    std::vector<std::string> parts;
    std::stringstream ss(key);
    std::string part;
    while (std::getline(ss, part, '.')) {
        parts.push_back(part);
    }

    if (parts.empty()) {
        return nullptr;
    }

    // Navigate through nested objects
    const rapidjson::Value* current = &document_;
    for (const auto& p : parts) {
        if (!current->IsObject() || !current->HasMember(p.c_str())) {
            return nullptr;
        }
        current = &(*current)[p.c_str()];
    }

    return current;
}

std::string EGMConfig::getString(const std::string& key, const std::string& defaultValue) {
    const rapidjson::Value* value = navigateToValue(key);
    if (!value || !value->IsString()) {
        return defaultValue;
    }
    return value->GetString();
}

int64_t EGMConfig::getInt(const std::string& key, int64_t defaultValue) {
    const rapidjson::Value* value = navigateToValue(key);
    if (!value) {
        return defaultValue;
    }

    if (value->IsInt64()) {
        return value->GetInt64();
    } else if (value->IsInt()) {
        return value->GetInt();
    } else if (value->IsUint64()) {
        return static_cast<int64_t>(value->GetUint64());
    } else if (value->IsDouble()) {
        return static_cast<int64_t>(value->GetDouble());
    }

    return defaultValue;
}

double EGMConfig::getDouble(const std::string& key, double defaultValue) {
    const rapidjson::Value* value = navigateToValue(key);
    if (!value) {
        return defaultValue;
    }

    if (value->IsDouble()) {
        return value->GetDouble();
    } else if (value->IsInt()) {
        return static_cast<double>(value->GetInt());
    } else if (value->IsUint()) {
        return static_cast<double>(value->GetUint());
    }

    return defaultValue;
}

bool EGMConfig::getBool(const std::string& key, bool defaultValue) {
    const rapidjson::Value* value = navigateToValue(key);
    if (!value || !value->IsBool()) {
        return defaultValue;
    }
    return value->GetBool();
}

const rapidjson::Value* EGMConfig::getObject(const std::string& key) {
    const rapidjson::Value* value = navigateToValue(key);
    if (!value || !value->IsObject()) {
        return nullptr;
    }
    return value;
}

} // namespace config
