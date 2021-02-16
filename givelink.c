#include "crc16.h"
#include "givelink.h"
#include <string.h>

givelink_context_t * context;

bool givelink_authed() {
    return context->authed;
}

void givelink_set_auth(bool authed) {
    context->authed = authed;
    if (authed) {
        context->header_length = context->authed_header_length;
    } else {
        context->header_length = context->unauth_header_length;
    }
}

void givelink_context_set_magic() {
    context->unauth_header_length = MAGIC_LENGTH;
    memcpy(context->unauth_header, (uint8_t *)GLP0, MAGIC_LENGTH);
    context->authed_header_length = MAGIC_LENGTH;
    memcpy(context->authed_header, (uint8_t *)GLP0, MAGIC_LENGTH);
}

void givelink_context_init(givelink_context_t * ctx, uint8_t * unauth_header) {
    context = ctx;
    uint8_t addr_packet_header[MAGIC_LENGTH + 1 + ADDR_LENGTH + 1 + 0];
    context->authed = false;
    context->authed_header_length = 0;
    context->unauth_header_length = 0;
    context->authed_header = addr_packet_header;
    context->unauth_header = unauth_header;
    givelink_context_set_magic();
    return context;
}

void givelink_context_set_key(const uint8_t * key, const uint16_t key_len) {
    context->unauth_header[context->unauth_header_length] = key_len;
    context->unauth_header_length += 1;
    memcpy(context->unauth_header+context->unauth_header_length, key, key_len);
    context->unauth_header_length += key_len;
}

void givelink_context_set_token(const uint8_t * token, const uint16_t token_len) {
    context->unauth_header[context->unauth_header_length] = token_len;
    context->unauth_header_length += 1;
    memcpy(context->unauth_header+context->unauth_header_length, token, token_len);
    context->unauth_header_length += token_len;
}

void givelink_context_set_addr(const uint8_t * addr, const uint16_t addr_len) {
    context->authed_header[context->authed_header_length] = addr_len;
    context->authed_header_length += 1;
    memcpy(context->authed_header+context->authed_header_length, addr, addr_len);
    context->authed_header_length += addr_len;
    context->authed_header[context->authed_header_length] = 0;
    context->authed_header_length += 1;
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
    swap_one(payload, context->header_length + 4); // crc16
    swap_one(payload, context->header_length + 2); // length
    swap_one(payload, context->header_length);     // id
}

void givelink_to_binary_raw(const givelink_t * m, uint8_t * payload) {
    if (context->authed) {
        memcpy(payload, context->authed_header, context->authed_header_length);
    } else {
        memcpy(payload, context->unauth_header, context->unauth_header_length);
    }
    memcpy(payload+context->header_length, (const uint8_t *)m, MINI_PACKET_LENGTH);
    memcpy(payload+context->header_length+MINI_PACKET_LENGTH, m->data, m->length - TYPE_LENGTH);
    swap_edition(payload);
}

void givelink_to_binary(const givelink_t * m, uint8_t * payload) {
    givelink_to_binary_raw(m, payload);

    payload[context->header_length + 4] = 0;
    payload[context->header_length + 5] = 0;

    uint16_t crc = crc_16(payload, givelink_get_length(m));

    uint8_t crch;
    uint8_t crcl;

    from_uint16(crc, &crch, &crcl);

    payload[context->header_length + 4] = crch;
    payload[context->header_length + 5] = crcl;
}

bool givelink_from_binary(givelink_t * m, const uint8_t * payload,
        const uint16_t length) {
    if (length < context->header_length + MINI_PACKET_LENGTH) {
        return false;
    }

    uint8_t idh = payload[context->header_length];
    uint8_t idl = payload[context->header_length + 1];

    m -> id = to_uint16(idh, idl);

    uint8_t lenh = payload[context->header_length + 2];
    uint8_t lenl = payload[context->header_length + 3];

    m -> length = to_uint16(lenh, lenl);

    uint8_t crch = payload[context->header_length + 4];
    uint8_t crcl = payload[context->header_length + 5];

    m -> crc16 = to_uint16(crch, crcl);

    m -> type = payload[context->header_length + 6];

    if (m -> length > TYPE_LENGTH) {
        memcpy(m->data, payload + context->header_length + MINI_PACKET_LENGTH,
                length - context->header_length - MINI_PACKET_LENGTH);
    }

    m -> data[m -> length - TYPE_LENGTH] = '\0';

    if (m -> type == AUTHRES) {
        givelink_context_set_addr(m->data, m->length - TYPE_LENGTH);
        givelink_set_auth(true);
    }

    if (m -> type == ERROR) {
        givelink_set_auth(false);
    }

    return true;
}

uint16_t givelink_get_length(const givelink_t *m) {
    return m -> length + context->header_length + MINI_PACKET_LENGTH - TYPE_LENGTH;
}

uint16_t givelink_get_data_length(const uint8_t * payload,
        const uint16_t length) {
    if (length < context->header_length + MINI_PACKET_LENGTH - TYPE_LENGTH) {
        return 0;
    }
    uint8_t lenh = payload[context->header_length + 2];
    uint8_t lenl = payload[context->header_length + 3];
    return to_uint16(lenh, lenl);
}

givelink_t * givelink_init(givelink_t * m, uint8_t *data) {
    m->id = 0;
    m->length = TYPE_LENGTH;
    m->crc16 = 0;
    m->type = PING;
    m->data = data;
    return m;
}

void givelink_reset(givelink_t * m) {
    m -> id = 0;
    m -> length = TYPE_LENGTH;
    m -> crc16 = 0;
    m -> type = PING;
    m -> data[0] = '\0';
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
    uint8_t crch = payload[context->header_length + 4];
    uint8_t crcl = payload[context->header_length + 5];
    uint16_t crc0 = to_uint16(crch, crcl);

    payload[context->header_length + 4] = 0x00;
    payload[context->header_length + 5] = 0x00;

    uint16_t crc = crc_16(payload, length);

    payload[context->header_length + 4] = crch;
    payload[context->header_length + 5] = crcl;

    return crc == crc0;
}

bool givelink_check_packet_header(const uint8_t * payload, const uint8_t * header) {
    for (uint16_t i = 0; i < context->header_length; i ++) {
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
        if (headLen == context->header_length) {
            if (!givelink_check_packet_header(payload, context->authed ? context->authed_header : context->unauth_header)) {
                // wrong packet, ignore
                headLen = 0;
            }
        } else if (headLen >= context->header_length + MINI_PACKET_LENGTH - 1) {
            uint16_t dataLen = givelink_get_data_length(payload, headLen);
            if (headLen >= context->header_length + MINI_PACKET_LENGTH - 1 + dataLen) {
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
