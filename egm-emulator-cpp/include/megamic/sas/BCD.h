#ifndef MEGAMIC_SAS_BCD_H
#define MEGAMIC_SAS_BCD_H

#include <cstdint>
#include <cstddef>
#include <vector>

namespace megamic {
namespace sas {

/**
 * BCD - Binary-Coded Decimal encoding/decoding for SAS protocol
 *
 * SAS protocol uses BCD encoding for meter values and other numeric data.
 * Each byte contains two decimal digits (0-9), with the high nibble being
 * the more significant digit.
 *
 * Example: Decimal 1234 → BCD: 0x12 0x34
 *
 * BCD Format in SAS:
 * - Each nibble (4 bits) represents one decimal digit (0-9)
 * - High nibble = tens digit, low nibble = ones digit
 * - Multi-byte values are big-endian (most significant byte first)
 * - Maximum value for N bytes: 10^(2N) - 1
 */
class BCD {
public:
    /**
     * Encode a binary value to BCD format
     * @param value Binary value to encode
     * @param numBytes Number of BCD bytes to generate
     * @return Vector of BCD bytes (big-endian)
     */
    static std::vector<uint8_t> encode(uint64_t value, size_t numBytes);

    /**
     * Encode to BCD and write to buffer
     * @param value Binary value to encode
     * @param buffer Output buffer
     * @param numBytes Number of BCD bytes to write
     * @return true if successful
     */
    static bool encodeTo(uint64_t value, uint8_t* buffer, size_t numBytes);

    /**
     * Decode BCD format to binary value
     * @param bcdData Pointer to BCD data
     * @param numBytes Number of BCD bytes
     * @return Decoded binary value
     */
    static uint64_t decode(const uint8_t* bcdData, size_t numBytes);

    /**
     * Validate BCD data (check all nibbles are 0-9)
     * @param bcdData Pointer to BCD data
     * @param numBytes Number of BCD bytes
     * @return true if valid BCD
     */
    static bool isValid(const uint8_t* bcdData, size_t numBytes);

    /**
     * Get maximum value for given number of BCD bytes
     * @param numBytes Number of BCD bytes
     * @return Maximum value (10^(2*numBytes) - 1)
     */
    static uint64_t maxValue(size_t numBytes);

    /**
     * Calculate minimum number of BCD bytes needed for value
     * @param value Binary value
     * @return Minimum number of BCD bytes required
     */
    static size_t minBytes(uint64_t value);

    /**
     * Convert single byte to BCD
     * @param value Value (0-99)
     * @return BCD byte
     */
    static uint8_t toBCD(uint8_t value);

    /**
     * Convert single BCD byte to binary
     * @param bcd BCD byte
     * @return Binary value (0-99)
     */
    static uint8_t fromBCD(uint8_t bcd);

private:
    /**
     * Check if a single nibble is valid BCD (0-9)
     */
    static bool isValidNibble(uint8_t nibble);
};

} // namespace sas
} // namespace megamic

#endif // MEGAMIC_SAS_BCD_H
