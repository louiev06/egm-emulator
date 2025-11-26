#ifndef MEGAMIC_SAS_CRC16_H
#define MEGAMIC_SAS_CRC16_H

#include <cstdint>
#include <cstddef>

namespace megamic {
namespace sas {

/**
 * CRC16 - CRC-16 calculation for SAS protocol
 *
 * SAS protocol uses CRC-16 with the following parameters:
 * - Polynomial: 0x8005 (x^16 + x^15 + x^2 + 1)
 * - Initial value: 0x0000
 * - Final XOR: 0x0000
 * - Reflect input: false
 * - Reflect output: false
 *
 * CRC is transmitted LSB first in SAS messages.
 */
class CRC16 {
public:
    /**
     * Calculate CRC-16 for the given data
     * @param data Pointer to data buffer
     * @param length Number of bytes in buffer
     * @return Calculated CRC-16 value
     */
    static uint16_t calculate(const uint8_t* data, size_t length);

    /**
     * Verify CRC-16 of received message
     * @param data Pointer to data buffer (including CRC bytes)
     * @param length Total length including CRC (2 bytes)
     * @return true if CRC is valid
     */
    static bool verify(const uint8_t* data, size_t length);

    /**
     * Append CRC-16 to message buffer
     * @param data Pointer to buffer with message data
     * @param dataLength Length of message data (excluding CRC)
     * @param buffer Pointer to output buffer (must have space for dataLength + 2)
     * @return Total length (dataLength + 2)
     */
    static size_t append(const uint8_t* data, size_t dataLength, uint8_t* buffer);

    /**
     * Extract CRC from message (last 2 bytes)
     * @param data Pointer to message buffer
     * @param length Total message length
     * @return CRC value (LSB first format)
     */
    static uint16_t extract(const uint8_t* data, size_t length);

private:
    static constexpr uint16_t POLYNOMIAL = 0x8005;
    static constexpr uint16_t INITIAL_VALUE = 0x0000;

    /**
     * Lookup table for faster CRC calculation
     * Generated using polynomial 0x8005
     */
    static const uint16_t CRC_TABLE[256];

    /**
     * Initialize CRC lookup table
     */
    static void initTable();
};

} // namespace sas
} // namespace megamic

#endif // MEGAMIC_SAS_CRC16_H
