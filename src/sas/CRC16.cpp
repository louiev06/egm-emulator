#include "sas/CRC16.h"
#include <cstring>


namespace sas {

/**
 * SAS CRC-16 calculation using nibble-based algorithm
 *
 * Based on SAS Protocol 6.01 Section 5 - Cyclical Redundancy Check
 * The CRC follows the CCITT convention starting with the most significant byte,
 * least significant bit and applying the CRC polynomial x^16+x^12+x^5+1
 *
 * The magic number 010201 octal (0x1081 hex) is derived from the CRC polynomial
 * x^16+x^12+x^5+1
 */

uint16_t CRC16::calculate(const uint8_t* data, size_t length) {
    if (data == nullptr || length == 0) {
        return 0;
    }

    uint16_t crcval = 0;

    // Process each byte
    for (size_t i = 0; i < length; i++) {
        uint8_t byte_val = data[i];

        // Process low nibble (bits 0-3)
        uint16_t q = (crcval ^ byte_val) & 0x0F;
        crcval = (crcval >> 4) ^ (q * 0x1081);

        // Process high nibble (bits 4-7)
        q = (crcval ^ (byte_val >> 4)) & 0x0F;
        crcval = (crcval >> 4) ^ (q * 0x1081);
    }

    return crcval & 0xFFFF;
}

bool CRC16::verify(const uint8_t* data, size_t length) {
    if (data == nullptr || length < 3) {  // Minimum: 1 byte data + 2 bytes CRC
        return false;
    }

    // Calculate CRC of data (excluding last 2 bytes which are the CRC)
    uint16_t calculatedCrc = calculate(data, length - 2);

    // Extract received CRC (LSB first)
    uint16_t receivedCrc = extract(data, length);

    return (calculatedCrc == receivedCrc);
}

size_t CRC16::append(const uint8_t* data, size_t dataLength, uint8_t* buffer) {
    if (data == nullptr || buffer == nullptr) {
        return 0;
    }

    // Copy data to buffer
    memcpy(buffer, data, dataLength);

    // Calculate CRC
    uint16_t crc = calculate(data, dataLength);

    // Append CRC in LSB first format (SAS protocol requirement)
    buffer[dataLength] = static_cast<uint8_t>(crc & 0xFF);        // LSB
    buffer[dataLength + 1] = static_cast<uint8_t>(crc >> 8);      // MSB

    return dataLength + 2;
}

uint16_t CRC16::extract(const uint8_t* data, size_t length) {
    if (data == nullptr || length < 2) {
        return 0;
    }

    // Extract CRC from last 2 bytes (LSB first)
    uint16_t crc = static_cast<uint16_t>(data[length - 2]);        // LSB
    crc |= static_cast<uint16_t>(data[length - 1]) << 8;           // MSB

    return crc;
}

} // namespace sas

