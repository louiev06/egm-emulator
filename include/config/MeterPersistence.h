#ifndef CONFIG_METERPERSISTENCE_H
#define CONFIG_METERPERSISTENCE_H

#include "simulator/Machine.h"
#include <string>
#include <cstdint>

namespace config {

/**
 * MeterPersistence - Saves and loads meter values to/from persistent storage
 *
 * Stores meters in /sdboot/meters.json for persistence across reboots
 * Minimizes disk writes by:
 * - Loading meters once at startup
 * - Keeping meters in RAM during operation
 * - Saving only on explicit save() call (shutdown, reboot button, etc.)
 */
class MeterPersistence {
public:
    /**
     * Load meters from persistent storage into machine
     * Tries /sdboot/meters.json first, falls back to local meters.json
     * @param machine Machine to load meters into
     * @return true if loaded successfully
     */
    static bool loadMeters(simulator::Machine* machine);

    /**
     * Save meters from machine to persistent storage
     * Saves to /sdboot/meters.json (or local if /sdboot not available)
     * @param machine Machine to save meters from
     * @return true if saved successfully
     */
    static bool saveMeters(simulator::Machine* machine);

    /**
     * Get the path where meters are persisted
     * @return Path to meters.json file
     */
    static std::string getMetersPath();

private:
    /**
     * Check if /sdboot is available for persistent storage
     * @return true if /sdboot exists and is writable
     */
    static bool isSdbootAvailable();

    /**
     * Get current timestamp in ISO 8601 format
     * @return Timestamp string (e.g., "2025-11-29T10:00:00Z")
     */
    static std::string getCurrentTimestamp();
};

} // namespace config

#endif // CONFIG_METERPERSISTENCE_H
