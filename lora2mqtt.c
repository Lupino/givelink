#include "crc16.h"
#include "unhex.h"
#include "lora2mqtt.h"
#include <string.h>

void swap_one(uint8_t * payload, int offset) {
    uint8_t tmp;
    tmp = payload[offset];
    payload[offset] = payload[offset + 1];
    payload[offset + 1] = tmp;
}

void swap_edition(uint8_t * payload) {
    swap_one(payload, HEADER_LENGTH - 2);         // crc16
    swap_one(payload, HEADER_LENGTH - 2 - 2);     // length
    swap_one(payload, HEADER_LENGTH - 2 - 2 - 2); // id
}

void lora2mqtt_to_binary_raw(const lora2mqtt_t * m, uint8_t * payload) {
    memcpy(payload, (const uint8_t *)m, lora2mqtt_get_length(m));
    swap_edition(payload);
}

void lora2mqtt_to_binary(lora2mqtt_t * m, uint8_t * payload) {
    if (m -> length < TYPE_LENGTH) {
        m -> length = TYPE_LENGTH;
    }
    m -> crc16 = 0;
    lora2mqtt_to_binary_raw(m, payload);
    uint16_t crc = crc_16(payload, lora2mqtt_get_length(m));

    uint8_t crch = crc / 256;
    uint8_t crcl = crc % 256;

    payload[HEADER_LENGTH - 2] = crch;
    payload[HEADER_LENGTH - 1] = crcl;
}

bool lora2mqtt_from_binary(lora2mqtt_t * m, uint8_t * payload, uint16_t length) {
    if (length < HEADER_LENGTH + TYPE_LENGTH) {
        return false;
    }

    uint8_t idh = payload[HEADER_LENGTH - 2 - 2 - 2];
    uint8_t idl = payload[HEADER_LENGTH - 2 - 2 - 1];

    m -> id = idh * 256 + idl;

    uint8_t lenh = payload[HEADER_LENGTH - 2 - 2];
    uint8_t lenl = payload[HEADER_LENGTH - 2 - 1];

    m -> length = lenh * 256 + lenl;

    uint8_t crch = payload[HEADER_LENGTH - 2];
    uint8_t crcl = payload[HEADER_LENGTH - 1];

    m -> crc16 = crch * 256 + crcl;

    m -> type = payload[HEADER_LENGTH];

    if (m -> length > TYPE_LENGTH) {
        memcpy(m->data, payload + HEADER_LENGTH + TYPE_LENGTH, length - HEADER_LENGTH - TYPE_LENGTH);
    }

    return true;
}

uint16_t lora2mqtt_get_length(const lora2mqtt_t *m) {
    return m -> length + HEADER_LENGTH;
}

uint16_t lora2mqtt_get_data_length(const uint8_t * payload, uint16_t length) {
    if (length < HEADER_LENGTH) {
        return 0;
    }
    uint8_t lenh = payload[HEADER_LENGTH - 2 - 2];
    uint8_t lenl = payload[HEADER_LENGTH - 2 - 1];
    return lenh * 256 + lenl;
}

lora2mqtt_t * lora2mqtt_new() {
    lora2mqtt_t * m = malloc(HEADER_LENGTH + 1);
    memcpy(m -> magic, (uint8_t *)LMQ0, 4);
    memcpy(m -> key, (uint8_t *)unhex((const uint8_t *)KEY, 20), 10);
    memcpy(m -> token, (uint8_t *)unhex((const uint8_t *)TOKEN, 32), 16);
    m -> id = 0;
    m -> length = 1;
    m -> crc16 = 0;
    m -> type = PING;
    return m;
}

void lora2mqtt_reset(lora2mqtt_t * m) {
    m -> id = 0;
    m -> length = 1;
    m -> crc16 = 0;
    m -> type = PING;
}

void lora2mqtt_free(lora2mqtt_t * m) {
    if (m) {
        free(m);
    }
}

void lora2mqtt_set_id(lora2mqtt_t * m, uint16_t id) {
    m->id = id;
}

void lora2mqtt_set_type(lora2mqtt_t * m, uint8_t type) {
    m->type = type;
}

void lora2mqtt_set_data(lora2mqtt_t * m, uint8_t * data, uint16_t length) {
    memcpy(m->data, data, length);
    m->length = TYPE_LENGTH + length;
}

bool lora2mqtt_discover_magic(const uint8_t * payload, uint16_t length) {
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
    if (length < HEADER_LENGTH + 1) {
        return false;
    }

    uint8_t crch = payload[HEADER_LENGTH - 2];
    uint8_t crcl = payload[HEADER_LENGTH - 1];
    uint16_t crc0 = crch * 256 + crcl;

    payload[HEADER_LENGTH - 2] = 0x00;
    payload[HEADER_LENGTH - 1] = 0x00;

    uint16_t crc = crc_16(payload, length);

    payload[HEADER_LENGTH - 2] = crch;
    payload[HEADER_LENGTH - 1] = crcl;

    return crc == crc0;
}

bool lora2mqtt_check_key(const uint8_t * payload, uint16_t length) {
    if (length < HEADER_LENGTH + 1) {
        return false;
    }
    uint8_t * key = (uint8_t *)unhex((const uint8_t *)KEY, 20);

    for (uint16_t i = 0; i < 10; i ++) {
        if (key[i] != payload[i + 4]) {
            return false;
        }
    }
    return true;
}

bool lora2mqtt_check_token(const uint8_t * payload, uint16_t length) {
    if (length < HEADER_LENGTH + 1) {
        return false;
    }
    uint8_t * token = (uint8_t *)unhex((const uint8_t *)TOKEN, 32);

    for (uint16_t i = 0; i < 16; i ++) {
        if (token[i] != payload[i + 4 + 10]) {
            return false;
        }
    }
    return true;
}

bool lora2mqtt_recv(uint8_t * payload, uint16_t * length, uint8_t c) {
    uint16_t headLen = *length;
    bool recved = false;
    payload[headLen] = c;
    headLen = headLen + 1;
    if (lora2mqtt_discover_magic(payload, headLen)) {
        if (headLen >= HEADER_LENGTH) {
            uint16_t dataLen = lora2mqtt_get_data_length(payload, headLen);
            if (headLen >= HEADER_LENGTH + dataLen) {
                if (lora2mqtt_check_crc16(payload, headLen)
                        && lora2mqtt_check_key(payload, headLen)
                        && lora2mqtt_check_token(payload, headLen)) {
                    recved = true;

                } else {
                    headLen = 0;
                }
            }
        }
    } else {
        if (headLen >= 4) {
            payload[0] = payload[headLen - 3];
            payload[1] = payload[headLen - 2];
            payload[2] = payload[headLen - 1];
            headLen = 3;
        }
    }

    *length = headLen;
    return recved;
}
