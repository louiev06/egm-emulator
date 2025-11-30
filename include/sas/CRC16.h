#ifndef SAS_CRC16_H
#define SAS_CRC16_H

#include <cstdint>
#include <cstddef>


namespace sas {

/**
 * CRC16 - CRC-16 calculation for SAS protocol
 *
 * Based on SAS Protocol 6.01 Section 5 - Cyclical Redundancy Check
 * Uses nibble-based algorithm with CCITT polynomial x^16+x^12+x^5+1
 *
 * SAS protocol uses CRC-16 with the following parameters:
 * - Polynomial: 0x1081 (derived from x^16+x^12+x^5+1)
 * - Initial value: 0x0000
 * - Algorithm: Nibble-based (processes 4 bits at a time, LSB first)
 * - Transmission: LSB first in SAS messages
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
};

} // namespace sas


#endif // SAS_CRC16_H
