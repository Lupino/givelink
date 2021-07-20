#include "givelink_raw.h"
#include <string.h>

uint16_t givelink_raw_get_header_length(const uint8_t * payload, const uint16_t size) {
    if (size < MINI_PACKET_LENGTH) {
        return 0;
    }
    uint16_t headerLen = PACKET_MAGIC_LENGTH;
    const uint16_t keyLen = (const uint16_t)payload[headerLen];
    headerLen = headerLen + 1 + keyLen;
    if (size <= headerLen) {
        return 0;
    }
    const uint16_t tokenLen = (const uint16_t)payload[headerLen];
    headerLen = headerLen + 1 + tokenLen + PACKET_ID_LENGTH + PACKET_LENGTH_LENGTH + PACKET_CRC_LENGTH;
    if (size < headerLen) {
        return 0;
    }
    return headerLen;
}

uint16_t givelink_raw_get_data_length0(const uint8_t * payload, const uint16_t headerLen) {
    const uint16_t pos = headerLen - PACKET_CRC_LENGTH - PACKET_LENGTH_LENGTH;
    return givelink_touint16(payload[pos], payload[pos+1]);
}

bool givelink_raw_check_crc16(uint8_t * payload, const uint16_t length, const uint16_t headerLen) {
    const uint16_t pos = headerLen - PACKET_CRC_LENGTH;
    const uint8_t crch = payload[pos];
    const uint8_t crcl = payload[pos+1];
    const uint16_t crc0 = givelink_touint16(crch, crcl);

    payload[pos] = 0x00;
    payload[pos+1] = 0x00;

    const uint16_t crc = givelink_crc16(payload, length);

    payload[pos] = crch;
    payload[pos+1] = crcl;

    return crc == crc0;
}

bool givelink_raw_recv(uint8_t * payload, uint16_t * length, const uint8_t c, bool *crc) {
    uint16_t size = *length;
    bool recved = false;
    payload[size] = c;
    size = size + 1;
    if (givelink_discover_magic(payload, size)) {
        uint16_t headerLen = givelink_raw_get_header_length(payload, size);
        if (headerLen > 0) {
            uint16_t dataLen = givelink_raw_get_data_length0(payload, headerLen);
            if (size >= headerLen + dataLen) {
                *crc = givelink_raw_check_crc16(payload, size, headerLen);
                recved = true;
            }
        }
    } else {
        if (size >= PACKET_MAGIC_LENGTH) {
            givelink_shift_data(payload, size);
            size -= 1;
        }
    }

    *length = size;
    return recved;
}

void givelink_raw_set_key(uint8_t * payload, const uint8_t * key, const uint16_t key_len) {
    payload[PACKET_MAGIC_LENGTH] = (const uint8_t)key_len;
    memcpy(payload + PACKET_MAGIC_LENGTH + 1, key, key_len);
}

void givelink_raw_set_token(uint8_t * payload, const uint8_t * token, const uint16_t token_len) {
    uint16_t headerLen = PACKET_MAGIC_LENGTH;
    headerLen = headerLen + 1 + (const uint16_t)payload[headerLen];
    payload[headerLen] = (const uint8_t)token_len;
    if (token_len > 0) {
        memcpy(payload + headerLen + 1, token, token_len);
    }
}

void givelink_raw_set_id(uint8_t * payload, const uint16_t id) {
    uint16_t headerLen = PACKET_MAGIC_LENGTH;
    headerLen = headerLen + 1 + (const uint16_t)payload[headerLen]; // + keyLen
    headerLen = headerLen + 1 + (const uint16_t)payload[headerLen]; // + tokenLen

    uint8_t h;
    uint8_t l;

    givelink_fromuint16(id, &h, &l);

    payload[headerLen]     = h;
    payload[headerLen + 1] = l;
}

void givelink_raw_set_type(uint8_t * payload, const uint8_t type) {
    uint16_t headerLen = PACKET_MAGIC_LENGTH;
    headerLen = headerLen + 1 + (const uint16_t)payload[headerLen]; // + keyLen
    headerLen = headerLen + 1 + (const uint16_t)payload[headerLen]; // + tokenLen
    headerLen = headerLen + PACKET_ID_LENGTH + PACKET_LENGTH_LENGTH + PACKET_CRC_LENGTH;
    payload[headerLen] = type;
}

void givelink_raw_set_data(uint8_t * payload, const uint8_t * data, const uint16_t length) {
    uint16_t headerLen = PACKET_MAGIC_LENGTH;
    headerLen = headerLen + 1 + (const uint16_t)payload[headerLen]; // + keyLen
    headerLen = headerLen + 1 + (const uint16_t)payload[headerLen]; // + tokenLen
    headerLen = headerLen + PACKET_ID_LENGTH;

    uint8_t h;
    uint8_t l;

    givelink_fromuint16(length + 1, &h, &l);

    payload[headerLen]     = h;
    payload[headerLen + 1] = l;

    headerLen = headerLen + PACKET_LENGTH_LENGTH + PACKET_CRC_LENGTH + PACKET_TYPE_LENGTH;

    if (length > 0) {
        memcpy(payload + headerLen, data, length);
    }
}

uint16_t givelink_raw_get_length(const uint8_t * payload) {
    uint16_t headerLen = PACKET_MAGIC_LENGTH;
    headerLen = headerLen + 1 + (const uint16_t)payload[headerLen]; // + keyLen
    headerLen = headerLen + 1 + (const uint16_t)payload[headerLen]; // + tokenLen
    headerLen = headerLen + PACKET_ID_LENGTH; // + idLen
    headerLen = headerLen + givelink_touint16(payload[headerLen], payload[headerLen+1]); // dataLen
    headerLen = headerLen + PACKET_LENGTH_LENGTH + PACKET_CRC_LENGTH;
    return headerLen;
}

void givelink_raw_set_crc16(uint8_t * payload) {
    const uint16_t size = givelink_raw_get_length(payload);
    const uint16_t headerLen = givelink_raw_get_header_length(payload, size);
    const uint16_t pos = headerLen - PACKET_CRC_LENGTH;
    payload[pos]     = 0;
    payload[pos + 1] = 0;

    const uint16_t crc = givelink_crc16(payload, size);

    uint8_t h;
    uint8_t l;

    givelink_fromuint16(crc, &h, &l);

    payload[pos]     = h;
    payload[pos + 1] = l;
}

uint8_t givelink_raw_get_type(const uint8_t * payload) {
    uint16_t headerLen = PACKET_MAGIC_LENGTH;
    headerLen = headerLen + 1 + (const uint16_t)payload[headerLen]; // + keyLen
    headerLen = headerLen + 1 + (const uint16_t)payload[headerLen]; // + tokenLen
    headerLen = headerLen + PACKET_ID_LENGTH + PACKET_LENGTH_LENGTH + PACKET_CRC_LENGTH;
    return payload[headerLen];
}

void givelink_raw_get_data(const uint8_t * payload, uint8_t * data, uint16_t *dataLen) {
    uint16_t headerLen = PACKET_MAGIC_LENGTH;
    headerLen = headerLen + 1 + (const uint16_t)payload[headerLen]; // + keyLen
    headerLen = headerLen + 1 + (const uint16_t)payload[headerLen]; // + tokenLen
    headerLen = headerLen + PACKET_ID_LENGTH;

    *dataLen = givelink_touint16(payload[headerLen], payload[headerLen+1]) - 1;

    headerLen = headerLen + PACKET_LENGTH_LENGTH + PACKET_CRC_LENGTH + PACKET_TYPE_LENGTH;

    if (*dataLen > 0) {
        memcpy(data, payload + headerLen, *dataLen);
    }
}

uint16_t givelink_raw_get_id(const uint8_t * payload) {
    uint16_t headerLen = PACKET_MAGIC_LENGTH;
    headerLen = headerLen + 1 + (const uint16_t)payload[headerLen]; // + keyLen
    headerLen = headerLen + 1 + (const uint16_t)payload[headerLen]; // + tokenLen
    return givelink_touint16(payload[headerLen], payload[headerLen+1]);
}


void givelink_raw_get_key(const uint8_t * payload, uint8_t * key, uint16_t * key_len) {
    uint16_t headerLen = PACKET_MAGIC_LENGTH;
    *key_len = (uint16_t)payload[headerLen];
    memcpy(key, payload + headerLen, *key_len);
}

void givelink_raw_set_magic(uint8_t * payload) {
    memcpy(payload, (uint8_t *)GLP0, PACKET_MAGIC_LENGTH);
}
