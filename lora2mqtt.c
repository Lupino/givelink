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
    swap_one(payload, headerLength - 2);
    swap_one(payload, headerLength - 2 - 2);
    swap_one(payload, headerLength - 2 - 2 - 2);
}

void lora2mqtt_to_binary_raw(const lora2mqtt_t * m, uint8_t * payload) {
    memcpy(payload, (const uint8_t *)m, lora2mqtt_get_length(m));
    swap_edition(payload);
}

void lora2mqtt_to_binary(lora2mqtt_t * m, uint8_t * payload) {
    if (m -> length < packetTypeLength) {
        m -> length = packetTypeLength;
    }
    m -> crc16 = 0;
    lora2mqtt_to_binary_raw(m, payload);
    uint16_t crc = crc_16(payload, lora2mqtt_get_length(m));

    uint8_t crch = crc / 256;
    uint8_t crcl = crc % 256;

    payload[headerLength - 2] = crch;
    payload[headerLength - 1] = crcl;
}

bool lora2mqtt_from_binary(lora2mqtt_t * m, uint8_t * payload, uint16_t length) {
    if (length < headerLength + 1) {
        return false;
    }

    swap_edition(payload);

    memcpy((uint8_t *)m, payload, headerLength + 1);

    swap_edition(payload);

    if (m -> length > 1) {
        memcpy(m->data, payload + headerLength + 1, length - headerLength - 1);
    }

    return true;
}

uint16_t lora2mqtt_get_length(const lora2mqtt_t *m) {
    return m -> length + headerLength;
}

uint16_t lora2mqtt_get_data_length(const uint8_t * payload, uint16_t length) {
    if (length < headerLength) {
        return 0;
    }
    uint8_t lenh = payload[headerLength - 2 - 2];
    uint8_t lenl = payload[headerLength - 2 - 1];
    return lenh * 256 + lenl;
}

lora2mqtt_t * lora2mqtt_new() {
    lora2mqtt_t * m = malloc(headerLength + 1);
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
    m->length = packetTypeLength + length;
}

bool lora2mqtt_discover_magic(const uint8_t * payload, uint16_t length) {
    if (length < magicLength) {
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
    if (length < headerLength + 1) {
        return false;
    }

    uint8_t crch = payload[headerLength - 2];
    uint8_t crcl = payload[headerLength - 1];
    uint16_t crc0 = crch * 256 + crcl;

    payload[headerLength - 2] = 0x00;
    payload[headerLength - 1] = 0x00;

    uint16_t crc = crc_16(payload, length);

    payload[headerLength - 2] = crch;
    payload[headerLength - 1] = crcl;

    return crc == crc0;
}

bool lora2mqtt_check_key(const uint8_t * payload, uint16_t length) {
    if (length < headerLength + 1) {
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
    if (length < headerLength + 1) {
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
        if (headLen >= headerLength) {
            uint16_t dataLen = lora2mqtt_get_data_length(payload, headLen);
            if (headLen >= headerLength + dataLen) {
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
