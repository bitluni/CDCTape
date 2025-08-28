#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Calculate CRC16 (IBM/Modbus variant, poly = 0xA001, init = 0xFFFF)
uint16_t crc16(const uint8_t *data, size_t length, uint16_t crc = 0xFFFF) 
{
	for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc = crc >> 1;
            }
        }
    }
    return crc;
}

// Validate a buffer that ends with its CRC16 (little-endian, low byte first)
bool crc16_validate(const uint8_t *data, size_t length) {
    if (length < 2) return false; // need at least CRC16

    // calculate CRC over data excluding last two bytes
    uint16_t calc_crc = crc16(data, length - 2);

    // extract stored CRC (assuming little-endian, Modbus style)
    uint16_t stored_crc = data[length - 2] | (data[length - 1] << 8);

    return calc_crc == stored_crc;
}