#include "crc16.h"
#include "unhex.h"
#include "lora2mqtt.h"
#include <string.h>

uint8_t *packet_header;
uint16_t PACKET_HEADER_LENGTH;

bool lora2mqtt_inited = false;


void lora2mqtt_init(const char * hex_key, const char * hex_token) {
    uint16_t KEY_LENGTH = strlen(hex_key) / 2;
    uint16_t TOKEN_LENGTH = strlen(hex_token) / 2;

    PACKET_HEADER_LENGTH = MAGIC_LENGTH + 1 + KEY_LENGTH + 1 + TOKEN_LENGTH;

    packet_header = malloc(PACKET_HEADER_LENGTH);
    memcpy(packet_header, (uint8_t *)LMQ0, MAGIC_LENGTH);
    packet_header[MAGIC_LENGTH] = KEY_LENGTH;
    memcpy(packet_header+MAGIC_LENGTH+1, (uint8_t *)unhex((const uint8_t *)hex_key, KEY_LENGTH * 2), KEY_LENGTH);

    packet_header[MAGIC_LENGTH+1+KEY_LENGTH] = TOKEN_LENGTH;
    memcpy(packet_header+MAGIC_LENGTH+1+KEY_LENGTH+1, (uint8_t *)unhex((const uint8_t *)hex_token, TOKEN_LENGTH * 2), TOKEN_LENGTH);

    lora2mqtt_inited = true;
}

uint16_t to_uint16(const uint8_t h, const uint8_t l) {
    return ((h << 8) & 0xff00) + (l & 0xff);
}

void from_uint16(uint16_t src, uint8_t *h, uint8_t *l) {
    *h = (uint8_t)(src >> 8);
    *l = (uint8_t)src;
}

void swap_one(uint8_t * payload, const int offset) {
    uint8_t tmp;
    tmp = payload[offset];
    payload[offset] = payload[offset + 1];
    payload[offset + 1] = tmp;
}

void swap_edition(uint8_t * payload) {
    swap_one(payload, PACKET_HEADER_LENGTH + 4); // crc16
    swap_one(payload, PACKET_HEADER_LENGTH + 2); // length
    swap_one(payload, PACKET_HEADER_LENGTH);     // id
}

void lora2mqtt_to_binary_raw(const lora2mqtt_t * m, uint8_t * payload) {
    memcpy(payload, packet_header, PACKET_HEADER_LENGTH);
    memcpy(payload+PACKET_HEADER_LENGTH, (const uint8_t *)m, lora2mqtt_get_length(m) - PACKET_HEADER_LENGTH);
    swap_edition(payload);
}

void lora2mqtt_to_binary(const lora2mqtt_t * m, uint8_t * payload) {
    lora2mqtt_to_binary_raw(m, payload);

    payload[PACKET_HEADER_LENGTH + 4] = 0;
    payload[PACKET_HEADER_LENGTH + 5] = 0;

    uint16_t crc = crc_16(payload, lora2mqtt_get_length(m));

    uint8_t crch;
    uint8_t crcl;

    from_uint16(crc, &crch, &crcl);

    payload[PACKET_HEADER_LENGTH + 4] = crch;
    payload[PACKET_HEADER_LENGTH + 5] = crcl;
}

bool lora2mqtt_from_binary(lora2mqtt_t * m, const uint8_t * payload, const uint16_t length) {
    if (length < PACKET_HEADER_LENGTH + MINI_PACKET_LENGTH) {
        return false;
    }

    uint8_t idh = payload[PACKET_HEADER_LENGTH];
    uint8_t idl = payload[PACKET_HEADER_LENGTH + 1];

    m -> id = to_uint16(idh, idl);

    uint8_t lenh = payload[PACKET_HEADER_LENGTH + 2];
    uint8_t lenl = payload[PACKET_HEADER_LENGTH + 3];

    m -> length = to_uint16(lenh, lenl);

    uint8_t crch = payload[PACKET_HEADER_LENGTH + 4];
    uint8_t crcl = payload[PACKET_HEADER_LENGTH + 5];

    m -> crc16 = to_uint16(crch, crcl);

    m -> type = payload[PACKET_HEADER_LENGTH + 6];

    if (m -> length > TYPE_LENGTH) {
        memcpy(m->data, payload + PACKET_HEADER_LENGTH + MINI_PACKET_LENGTH,
                length - PACKET_HEADER_LENGTH - MINI_PACKET_LENGTH);
    }

    return true;
}

uint16_t lora2mqtt_get_length(const lora2mqtt_t *m) {
    return m -> length + PACKET_HEADER_LENGTH + MINI_PACKET_LENGTH - TYPE_LENGTH;
}

uint16_t lora2mqtt_get_data_length(const uint8_t * payload, const uint16_t length) {
    if (length < PACKET_HEADER_LENGTH + MINI_PACKET_LENGTH - TYPE_LENGTH) {
        return 0;
    }
    uint8_t lenh = payload[PACKET_HEADER_LENGTH + 2];
    uint8_t lenl = payload[PACKET_HEADER_LENGTH + 3];
    return to_uint16(lenh, lenl);
}

lora2mqtt_t * lora2mqtt_new(const char * hex_key, const char * hex_token) {
    if (!lora2mqtt_inited) {
        lora2mqtt_init(hex_key, hex_token);
    }

    lora2mqtt_t * m = (lora2mqtt_t *)malloc(MINI_PACKET_LENGTH);

    m -> id = 0;
    m -> length = TYPE_LENGTH;
    m -> crc16 = 0;
    m -> type = PING;
    return m;
}

void lora2mqtt_reset(lora2mqtt_t * m) {
    m -> id = 0;
    m -> length = TYPE_LENGTH;
    m -> crc16 = 0;
    m -> type = PING;
}

void lora2mqtt_free(lora2mqtt_t * m) {
    if (m) {
        free(m);
    }
}

void lora2mqtt_set_id(lora2mqtt_t * m, const uint16_t id) {
    m->id = id;
}

void lora2mqtt_set_type(lora2mqtt_t * m, const uint8_t type) {
    m->type = type;
    m->length = TYPE_LENGTH;
}

void lora2mqtt_set_data(lora2mqtt_t * m, const uint8_t * data, const uint16_t length) {
    memcpy(m->data, data, length);
    m->length = TYPE_LENGTH + length;
}

bool lora2mqtt_discover_magic(const uint8_t * payload, const uint16_t length) {
    if (length < MAGIC_LENGTH) {
        return false;
    }
    if (payload[0] == 'L'
            && payload[1] == 'M'
            && payload[2] == 'Q'
            && payload[3] == '0') {
        return true;
    }
    return false;
}

bool lora2mqtt_check_crc16(uint8_t * payload, uint16_t length) {
    if (length < PACKET_HEADER_LENGTH + MINI_PACKET_LENGTH) {
        return false;
    }

    uint8_t crch = payload[PACKET_HEADER_LENGTH + 4];
    uint8_t crcl = payload[PACKET_HEADER_LENGTH + 5];
    uint16_t crc0 = to_uint16(crch, crcl);

    payload[PACKET_HEADER_LENGTH + 4] = 0x00;
    payload[PACKET_HEADER_LENGTH + 5] = 0x00;

    uint16_t crc = crc_16(payload, length);

    payload[PACKET_HEADER_LENGTH + 4] = crch;
    payload[PACKET_HEADER_LENGTH + 5] = crcl;

    return crc == crc0;
}

bool lora2mqtt_check_packet_header(const uint8_t * payload, const uint16_t length) {
    if (length < PACKET_HEADER_LENGTH) {
        return false;
    }

    for (uint16_t i = 0; i < PACKET_HEADER_LENGTH; i ++) {
        if (packet_header[i] != payload[i]) {
            return false;
        }
    }
    return true;
}

void shift_data(uint8_t * payload, uint16_t length) {
    for (uint16_t i = 0; i < length - 1; i ++) {
        payload[i] = payload[i + 1];
    }
}

bool lora2mqtt_recv(uint8_t * payload, uint16_t * length, uint8_t c) {
    uint16_t headLen = *length;
    bool recved = false;
    payload[headLen] = c;
    headLen = headLen + 1;
    if (lora2mqtt_discover_magic(payload, headLen)) {
        if (headLen >= PACKET_HEADER_LENGTH + MINI_PACKET_LENGTH - 1) {
            uint16_t dataLen = lora2mqtt_get_data_length(payload, headLen);
            if (headLen >= PACKET_HEADER_LENGTH + MINI_PACKET_LENGTH - 1 + dataLen) {
                if (lora2mqtt_check_crc16(payload, headLen)
                        && lora2mqtt_check_packet_header(payload, headLen)) {
                    recved = true;
                } else {
                    headLen = 0;
                }
            }
        }
    } else {
        if (headLen >= MAGIC_LENGTH) {
            shift_data(payload, headLen);
            headLen -= 1;
        }
    }

    *length = headLen;
    return recved;
}
