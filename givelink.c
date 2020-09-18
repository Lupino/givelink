#include "crc16.h"
#include "unhex.h"
#include "givelink.h"
#include <string.h>

uint8_t *raw_packet_header;
uint16_t raw_packet_header_length;

uint8_t *addr_packet_header;
uint16_t addr_packet_header_length;


uint16_t PACKET_HEADER_LENGTH;
bool authed = false;

bool givelink_authed() {
    return authed;
}

void givelink_set_auth(bool auth_info) {
    authed = auth_info;
    if (auth_info) {
        PACKET_HEADER_LENGTH = addr_packet_header_length;
    } else {
        PACKET_HEADER_LENGTH = raw_packet_header_length;
    }
}

void givelink_init(const char * hex_key, const char * hex_token) {
    uint16_t KEY_LENGTH = strlen(hex_key) / 2;
    uint16_t TOKEN_LENGTH = strlen(hex_token) / 2;

    raw_packet_header_length = MAGIC_LENGTH + 1 + KEY_LENGTH + 1 + TOKEN_LENGTH;

    raw_packet_header = malloc(raw_packet_header_length);
    memcpy(raw_packet_header, (uint8_t *)GLP0, MAGIC_LENGTH);
    raw_packet_header[MAGIC_LENGTH] = KEY_LENGTH;
    memcpy(raw_packet_header+MAGIC_LENGTH+1,
            (uint8_t *)unhex((const uint8_t *)hex_key, KEY_LENGTH * 2), KEY_LENGTH);

    raw_packet_header[MAGIC_LENGTH+1+KEY_LENGTH] = TOKEN_LENGTH;
    memcpy(raw_packet_header+MAGIC_LENGTH+1+KEY_LENGTH+1,
            (uint8_t *)unhex((const uint8_t *)hex_token, TOKEN_LENGTH * 2), TOKEN_LENGTH);

    PACKET_HEADER_LENGTH = raw_packet_header_length;
    givelink_set_auth(false);
}

void givelink_set_addr(const uint8_t * addr, const uint16_t addr_length) {
    addr_packet_header_length = MAGIC_LENGTH + 1 + addr_length + 1 + 0;

    addr_packet_header = malloc(addr_packet_header_length);
    memcpy(addr_packet_header, (uint8_t *)GLP0, MAGIC_LENGTH);
    addr_packet_header[MAGIC_LENGTH] = addr_length;
    memcpy(addr_packet_header+MAGIC_LENGTH+1, addr, addr_length);

    addr_packet_header[MAGIC_LENGTH+1+addr_length] = 0;

    PACKET_HEADER_LENGTH = addr_packet_header_length;
    givelink_set_auth(true);
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

void givelink_to_binary_raw(const givelink_t * m, uint8_t * payload) {
    if (authed) {
        memcpy(payload, addr_packet_header, addr_packet_header_length);
    } else {
        memcpy(payload, raw_packet_header, raw_packet_header_length);
    }
    memcpy(payload+PACKET_HEADER_LENGTH, (const uint8_t *)m, MINI_PACKET_LENGTH);
    memcpy(payload+PACKET_HEADER_LENGTH+MINI_PACKET_LENGTH, m->data, m->length - TYPE_LENGTH);
    swap_edition(payload);
}

void givelink_to_binary(const givelink_t * m, uint8_t * payload) {
    givelink_to_binary_raw(m, payload);

    payload[PACKET_HEADER_LENGTH + 4] = 0;
    payload[PACKET_HEADER_LENGTH + 5] = 0;

    uint16_t crc = crc_16(payload, givelink_get_length(m));

    uint8_t crch;
    uint8_t crcl;

    from_uint16(crc, &crch, &crcl);

    payload[PACKET_HEADER_LENGTH + 4] = crch;
    payload[PACKET_HEADER_LENGTH + 5] = crcl;
}

bool givelink_from_binary(givelink_t * m, const uint8_t * payload,
        const uint16_t length) {
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

    m -> data[m -> length - TYPE_LENGTH] = '\0';

    if (m -> type == AUTHRES) {
        givelink_set_addr(m -> data, m -> length - TYPE_LENGTH);
    }

    if (m -> type == ERROR) {
        givelink_set_auth(false);
    }

    return true;
}

uint16_t givelink_get_length(const givelink_t *m) {
    return m -> length + PACKET_HEADER_LENGTH + MINI_PACKET_LENGTH - TYPE_LENGTH;
}

uint16_t givelink_get_data_length(const uint8_t * payload,
        const uint16_t length) {
    if (length < PACKET_HEADER_LENGTH + MINI_PACKET_LENGTH - TYPE_LENGTH) {
        return 0;
    }
    uint8_t lenh = payload[PACKET_HEADER_LENGTH + 2];
    uint8_t lenl = payload[PACKET_HEADER_LENGTH + 3];
    return to_uint16(lenh, lenl);
}

givelink_t * givelink_new(const uint16_t length) {
    givelink_t * m = (givelink_t *)malloc(MINI_PACKET_LENGTH);

    m -> id = 0;
    m -> length = TYPE_LENGTH;
    m -> crc16 = 0;
    m -> type = PING;
    m -> data = malloc(length - MINI_PACKET_LENGTH);
    return m;
}

void givelink_reset(givelink_t * m) {
    m -> id = 0;
    m -> length = TYPE_LENGTH;
    m -> crc16 = 0;
    m -> type = PING;
    m -> data[0] = '\0';
}

void givelink_free(givelink_t * m) {
    if (m) {
        free(m);
    }
}

void givelink_set_id(givelink_t * m, const uint16_t id) {
    m->id = id;
}

void givelink_set_type(givelink_t * m, const uint8_t type) {
    m->type = type;
}

void givelink_set_data(givelink_t * m, const uint8_t * data,
        const uint16_t length) {
    memcpy(m->data, data, length);
    m->length = TYPE_LENGTH + length;
}

bool givelink_discover_magic(const uint8_t * payload, const uint16_t length) {
    if (length < MAGIC_LENGTH) {
        return false;
    }
    if (payload[0] == 'G'
            && payload[1] == 'L'
            && payload[2] == 'P'
            && payload[3] == '0') {
        return true;
    }
    return false;
}

bool givelink_check_crc16(uint8_t * payload, uint16_t length) {
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

bool givelink_check_packet_header(const uint8_t * payload, const uint8_t * header) {
    for (uint16_t i = 0; i < PACKET_HEADER_LENGTH; i ++) {
        if (header[i] != payload[i]) {
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

bool givelink_recv(uint8_t * payload, uint16_t * length, uint8_t c) {
    uint16_t headLen = *length;
    bool recved = false;
    payload[headLen] = c;
    headLen = headLen + 1;
    if (givelink_discover_magic(payload, headLen)) {
        if (headLen == PACKET_HEADER_LENGTH) {
            if (!givelink_check_packet_header(payload, authed ? addr_packet_header : raw_packet_header)) {
                // wrong packet, ignore
                headLen = 0;
            }
        } else if (headLen >= PACKET_HEADER_LENGTH + MINI_PACKET_LENGTH - 1) {
            uint16_t dataLen = givelink_get_data_length(payload, headLen);
            if (headLen >= PACKET_HEADER_LENGTH + MINI_PACKET_LENGTH - 1 + dataLen) {
                if (givelink_check_crc16(payload, headLen)) {
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
