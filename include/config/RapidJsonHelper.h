#ifndef CONFIG_RAPIDJSONHELPER_H
#define CONFIG_RAPIDJSONHELPER_H

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>
#include <string>
#include <cstdint>

namespace config {

// Type aliases matching nCompass pattern
typedef rapidjson::StringBuffer JsonWriteStreamString;
typedef rapidjson::Writer<JsonWriteStreamString> JsonWriter;
typedef rapidjson::PrettyWriter<JsonWriteStreamString> JsonPrettyWriter;

/**
 * RapidJsonHelper - Helper functions for working with RapidJSON
 * Provides type-safe getters with default values
 */
class RapidJsonHelper {
public:
    /**
     * Get string value from JSON object
     * @param value JSON object to read from
     * @param key Key name
     * @param defaultValue Default if key not found or wrong type
     * @return String value or default
     */
    static std::string GetString(const rapidjson::Value& value, const char* key, const std::string& defaultValue = "");

    /**
     * Get 64-bit integer value from JSON object
     * @param value JSON object to read from
     * @param key Key name
     * @param defaultValue Default if key not found or wrong type
     * @return int64_t value or default
     */
    static int64_t GetInt64(const rapidjson::Value& value, const char* key, int64_t defaultValue = 0);

    /**
     * Get integer value from JSON object
     * @param value JSON object to read from
     * @param key Key name
     * @param defaultValue Default if key not found or wrong type
     * @return int value or default
     */
    static int GetInt(const rapidjson::Value& value, const char* key, int defaultValue = 0);

    /**
     * Get unsigned 64-bit integer value from JSON object
     * @param value JSON object to read from
     * @param key Key name
     * @param defaultValue Default if key not found or wrong type
     * @return uint64_t value or default
     */
    static uint64_t GetUint64(const rapidjson::Value& value, const char* key, uint64_t defaultValue = 0);

    /**
     * Get double value from JSON object
     * @param value JSON object to read from
     * @param key Key name
     * @param defaultValue Default if key not found or wrong type
     * @return double value or default
     */
    static double GetDouble(const rapidjson::Value& value, const char* key, double defaultValue = 0.0);

    /**
     * Get boolean value from JSON object
     * @param value JSON object to read from
     * @param key Key name
     * @param defaultValue Default if key not found or wrong type
     * @return bool value or default
     */
    static bool GetBool(const rapidjson::Value& value, const char* key, bool defaultValue = false);

    /**
     * Check if key exists in JSON object
     * @param value JSON object to check
     * @param key Key name
     * @return true if key exists
     */
    static bool HasMember(const rapidjson::Value& value, const char* key);

    /**
     * Get nested object from JSON object
     * @param value JSON object to read from
     * @param key Key name
     * @return Pointer to nested object or nullptr if not found
     */
    static const rapidjson::Value* GetObject(const rapidjson::Value& value, const char* key);
};

} // namespace config

#endif // CONFIG_RAPIDJSONHELPER_H
