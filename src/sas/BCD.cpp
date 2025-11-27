#include "sas/BCD.h"
#include <cstring>


namespace sas {

std::vector<uint8_t> BCD::encode(uint64_t value, size_t numBytes) {
    std::vector<uint8_t> result(numBytes, 0);
    encodeTo(value, result.data(), numBytes);
    return result;
}

bool BCD::encodeTo(uint64_t value, uint8_t* buffer, size_t numBytes) {
    if (buffer == nullptr || numBytes == 0) {
        return false;
    }

    // Check if value fits in requested number of bytes
    if (value > maxValue(numBytes)) {
        // Value too large, truncate or return false
        return false;
    }

    // Convert from least significant byte to most significant
    for (size_t i = 0; i < numBytes; i++) {
        size_t bufferIndex = numBytes - 1 - i;  // Fill from right to left

        // Get last two decimal digits
        uint8_t ones = value % 10;
        value /= 10;
        uint8_t tens = value % 10;
        value /= 10;

        // Pack into BCD byte (tens in high nibble, ones in low nibble)
        buffer[bufferIndex] = (tens << 4) | ones;
    }

    return true;
}

uint64_t BCD::decode(const uint8_t* bcdData, size_t numBytes) {
    if (bcdData == nullptr || numBytes == 0) {
        return 0;
    }

    uint64_t result = 0;

    // Process from most significant byte to least significant
    for (size_t i = 0; i < numBytes; i++) {
        uint8_t bcdByte = bcdData[i];

        // Extract high nibble (tens) and low nibble (ones)
        uint8_t tens = (bcdByte >> 4) & 0x0F;
        uint8_t ones = bcdByte & 0x0F;

        // Validate nibbles
        if (tens > 9 || ones > 9) {
            // Invalid BCD, treat as 0
            tens = 0;
            ones = 0;
        }

        // Accumulate result
        result = result * 100 + (tens * 10) + ones;
    }

    return result;
}

bool BCD::isValid(const uint8_t* bcdData, size_t numBytes) {
    if (bcdData == nullptr || numBytes == 0) {
        return false;
    }

    for (size_t i = 0; i < numBytes; i++) {
        uint8_t highNibble = (bcdData[i] >> 4) & 0x0F;
        uint8_t lowNibble = bcdData[i] & 0x0F;

        if (!isValidNibble(highNibble) || !isValidNibble(lowNibble)) {
            return false;
        }
    }

    return true;
}

uint64_t BCD::maxValue(size_t numBytes) {
    if (numBytes == 0) {
        return 0;
    }

    // Each byte holds 2 decimal digits (0-99)
    // Maximum value = 10^(2*numBytes) - 1

    uint64_t result = 1;
    for (size_t i = 0; i < numBytes * 2; i++) {
        result *= 10;
    }
    return result - 1;
}

size_t BCD::minBytes(uint64_t value) {
    if (value == 0) {
        return 1;  // At least 1 byte
    }

    // Count decimal digits
    size_t digits = 0;
    uint64_t temp = value;
    while (temp > 0) {
        digits++;
        temp /= 10;
    }

    // Round up to even number of digits (2 digits per byte)
    return (digits + 1) / 2;
}

uint8_t BCD::toBCD(uint8_t value) {
    if (value > 99) {
        value = 99;  // Clamp to valid range
    }

    uint8_t tens = value / 10;
    uint8_t ones = value % 10;

    return (tens << 4) | ones;
}

uint8_t BCD::fromBCD(uint8_t bcd) {
    uint8_t tens = (bcd >> 4) & 0x0F;
    uint8_t ones = bcd & 0x0F;

    // Validate
    if (tens > 9) tens = 9;
    if (ones > 9) ones = 9;

    return tens * 10 + ones;
}

bool BCD::isValidNibble(uint8_t nibble) {
    return nibble <= 9;
}

} // namespace sas

