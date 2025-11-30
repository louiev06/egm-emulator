#ifndef CONFIG_EGMCONFIG_H
#define CONFIG_EGMCONFIG_H

#include <rapidjson/document.h>
#include <string>
#include <cstdint>

namespace config {

/**
 * EGMConfig - Loads and provides access to EGM configuration from JSON file
 * Uses RapidJSON for parsing
 */
class EGMConfig {
public:
    /**
     * Load configuration from JSON file
     * Tries /sdboot/egm-config.json first, falls back to local egm-config.json
     * @param configPath Path to egm-config.json (optional, will use default paths)
     * @return true if loaded successfully
     */
    static bool load(const std::string& configPath = "");

    /**
     * Get the root JSON document
     * @return Pointer to document or nullptr if not loaded
     */
    static const rapidjson::Document* getDocument();

    /**
     * Get string value from config using dot notation
     * @param key Key path (e.g., "machineInfo.serialNumber")
     * @param defaultValue Default if key not found
     * @return Value from config or default
     */
    static std::string getString(const std::string& key, const std::string& defaultValue = "");

    /**
     * Get integer value from config using dot notation
     * @param key Key path (e.g., "aft.maxBufferIndex")
     * @param defaultValue Default if key not found
     * @return Value from config or default
     */
    static int64_t getInt(const std::string& key, int64_t defaultValue = 0);

    /**
     * Get double value from config using dot notation
     * @param key Key path (e.g., "machineInfo.denomination")
     * @param defaultValue Default if key not found
     * @return Value from config or default
     */
    static double getDouble(const std::string& key, double defaultValue = 0.0);

    /**
     * Get boolean value from config using dot notation
     * @param key Key path (e.g., "aft.enabled")
     * @param defaultValue Default if key not found
     * @return Value from config or default
     */
    static bool getBool(const std::string& key, bool defaultValue = false);

    /**
     * Get nested object using dot notation
     * @param key Key path (e.g., "aft")
     * @return Pointer to nested object or nullptr if not found
     */
    static const rapidjson::Value* getObject(const std::string& key);

private:
    static rapidjson::Document document_;
    static bool loaded_;

    /**
     * Navigate to nested value using dot notation
     * @param key Key path with dots
     * @return Pointer to value or nullptr if not found
     */
    static const rapidjson::Value* navigateToValue(const std::string& key);
};

} // namespace config

#endif // CONFIG_EGMCONFIG_H
