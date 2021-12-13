#include "givelink.h"
#include <string.h>

givelink_context_t * context;
givelink_t * gl_obj;

bool givelink_context_authed() {
    return context->authed;
}

void givelink_context_set_auth(const bool authed) {
    context->authed = authed;
    if (authed) {
        context->header_length = context->authed_header_length;
    } else {
        context->header_length = context->unauth_header_length;
    }
}

void givelink_context_set_unauth_magic() {
    context->unauth_header_start = 0;
    context->unauth_header_length = PACKET_MAGIC_LENGTH;
    memcpy(context->buffer, (uint8_t *)GLP0, PACKET_MAGIC_LENGTH);
}

void givelink_context_set_authed_magic() {
    context->authed_header_start = context->unauth_header_start+context->unauth_header_length;
    context->authed_header_length = PACKET_MAGIC_LENGTH;
    memcpy(context->buffer+context->authed_header_start, (uint8_t *)GLP0, PACKET_MAGIC_LENGTH);
}

void givelink_context_init(givelink_context_t * ctx, uint8_t * buffer) {
    context = ctx;
    context->buffer = buffer;

    context->unauth_header_start = 0;
    context->unauth_header_length = 0;

    context->authed_header_start = 0;
    context->authed_header_length = 0;

    context->authed = false;
    givelink_context_set_unauth_magic();
}

void givelink_context_set_key(const uint8_t * key, const uint16_t key_len) {
    context->buffer[context->unauth_header_start+context->unauth_header_length] = key_len;
    context->unauth_header_length += 1;
    memcpy(context->buffer+context->unauth_header_start+context->unauth_header_length, key, key_len);
    context->unauth_header_length += key_len;
}

void givelink_context_set_token(const uint8_t * token, const uint16_t token_len) {
    context->buffer[context->unauth_header_start+context->unauth_header_length] = token_len;
    context->unauth_header_length += 1;
    memcpy(context->buffer+context->unauth_header_start+context->unauth_header_length, token, token_len);
    context->unauth_header_length += token_len;

    givelink_context_set_auth(false);
}

void givelink_context_set_addr(const uint8_t * addr, const uint16_t addr_len) {
    givelink_context_set_authed_magic();
    context->buffer[context->authed_header_start+context->authed_header_length] = addr_len;
    context->authed_header_length += 1;
    memcpy(context->buffer+context->authed_header_start+context->authed_header_length, addr, addr_len);
    context->authed_header_length += addr_len;
    context->buffer[context->authed_header_start+context->authed_header_length] = 0;
    context->authed_header_length += 1;
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

void givelink_to_binary_raw(uint8_t * payload) {
    if (context->authed) {
        memcpy(payload, context->buffer+context->authed_header_start, context->authed_header_length);
    } else {
        memcpy(payload, context->buffer+context->unauth_header_start, context->unauth_header_length);
    }
    memcpy(payload+context->header_length, (const uint8_t *)gl_obj, MINI_PACKET_LENGTH);
    memcpy(payload+context->header_length+MINI_PACKET_LENGTH, gl_obj->data, gl_obj->length - PACKET_TYPE_LENGTH);
    swap_edition(payload);
}

void givelink_to_binary(uint8_t * payload) {
    givelink_to_binary_raw(payload);

    payload[context->header_length + 4] = 0;
    payload[context->header_length + 5] = 0;

    uint16_t crc = givelink_crc16(payload, givelink_get_length(), INIT_CRC);

    uint8_t crch;
    uint8_t crcl;

    givelink_fromuint16(crc, &crch, &crcl);

    payload[context->header_length + 4] = crch;
    payload[context->header_length + 5] = crcl;
}

bool givelink_from_binary(const uint8_t * payload,
        const uint16_t length) {
    uint16_t header_length = context->header_length;
    if (givelink_is_broadcast(payload, headLen)) {
        header_length = PACKET_BROADCAST_HEAD_LENGTH;
    }

    if (length < header_length + MINI_PACKET_LENGTH) {
        return false;
    }

    uint8_t idh = payload[header_length];
    uint8_t idl = payload[header_length + 1];

    gl_obj -> id = givelink_touint16(idh, idl);

    uint8_t lenh = payload[header_length + 2];
    uint8_t lenl = payload[header_length + 3];

    gl_obj -> length = givelink_touint16(lenh, lenl);

    uint8_t crch = payload[header_length + 4];
    uint8_t crcl = payload[header_length + 5];

    gl_obj -> crc16 = givelink_touint16(crch, crcl);

    gl_obj -> type = payload[header_length + 6];

    if (gl_obj -> length > PACKET_TYPE_LENGTH) {
        memcpy(gl_obj->data, payload + header_length + MINI_PACKET_LENGTH,
                length - header_length - MINI_PACKET_LENGTH);
    }

    gl_obj -> data[gl_obj -> length - PACKET_TYPE_LENGTH] = '\0';

    if (gl_obj -> type == AUTHRES) {
        givelink_context_set_addr(gl_obj->data, gl_obj->length - PACKET_TYPE_LENGTH);
        givelink_context_set_auth(true);
    }

    if (gl_obj -> type == DROP) {
        givelink_context_set_auth(false);
    }

    return true;
}

uint16_t givelink_get_length() {
    return gl_obj -> length + context->header_length + MINI_PACKET_LENGTH - PACKET_TYPE_LENGTH;
}

uint16_t givelink_get_data_length0(const uint16_t header_length, const uint8_t * payload,
        const uint16_t length) {
    if (length < header_length + MINI_PACKET_LENGTH - PACKET_TYPE_LENGTH) {
        return 0;
    }
    uint8_t lenh = payload[header_length + 2];
    uint8_t lenl = payload[header_length + 3];
    return givelink_touint16(lenh, lenl);
}

uint16_t givelink_get_data_length() {
    return gl_obj->length - PACKET_TYPE_LENGTH;
}

void givelink_init(givelink_t * m, uint8_t *data) {
    gl_obj = m;
    gl_obj->id = 0;
    gl_obj->length = PACKET_TYPE_LENGTH;
    gl_obj->crc16 = 0;
    gl_obj->type = PING;
    gl_obj->data = data;
}

void givelink_reset() {
    gl_obj -> id = 0;
    gl_obj -> length = PACKET_TYPE_LENGTH;
    gl_obj -> crc16 = 0;
    gl_obj -> type = PING;
    gl_obj -> data[0] = '\0';
}

void givelink_set_id(const uint16_t id) {
    gl_obj->id = id;
}

void givelink_set_type(const uint8_t type) {
    gl_obj->type = type;
}

void givelink_set_data(const uint8_t * data, const uint16_t length) {
    memcpy(gl_obj->data, data, length);
    givelink_set_data_length(length);
}

void givelink_set_data_length(const uint16_t length) {
    gl_obj->length = PACKET_TYPE_LENGTH + length;
}

bool givelink_check_crc16(const uint16_t header_length, uint8_t * payload, const uint16_t length) {
    uint8_t crch = payload[header_length + 4];
    uint8_t crcl = payload[header_length + 5];
    uint16_t crc0 = givelink_touint16(crch, crcl);

    payload[header_length + 4] = 0x00;
    payload[header_length + 5] = 0x00;

    uint16_t crc = givelink_crc16(payload, length, INIT_CRC);

    payload[header_length + 4] = crch;
    payload[header_length + 5] = crcl;

    return crc == crc0;
}

bool givelink_check_packet_header(const uint8_t * payload) {
    const uint16_t header_start = context->authed ? context->authed_header_start : context->unauth_header_start;
    for (uint16_t i = 0; i < context->header_length; i ++) {
        if (context->buffer[header_start+i] != payload[i]) {
            return false;
        }
    }
    return true;
}

bool givelink_recv(uint8_t * payload, uint16_t * length, const uint8_t c) {
    uint16_t headLen = *length;
    bool recved = false;
    payload[headLen] = c;
    headLen = headLen + 1;
    if (givelink_discover_magic(payload, headLen)) {
        if (givelink_is_broadcast(payload, headLen)) {
            if (headLen >= PACKET_BROADCAST_HEAD_LENGTH + MINI_PACKET_LENGTH - PACKET_TYPE_LENGTH) {
                uint16_t dataLen = givelink_get_data_length0(PACKET_BROADCAST_HEAD_LENGTH, payload, headLen);
                if (headLen >= PACKET_BROADCAST_HEAD_LENGTH + MINI_PACKET_LENGTH - PACKET_TYPE_LENGTH + dataLen) {
                    if (givelink_check_crc16(PACKET_BROADCAST_HEAD_LENGTH, payload, headLen)) {
                        recved = true;
                    } else {
                        givelink_shift_data(payload, &headLen);
                    }
                }
            }
        } else if (headLen >= context->header_length + MINI_PACKET_LENGTH - PACKET_TYPE_LENGTH) {
            uint16_t dataLen = givelink_get_data_length0(context->header_length, payload, headLen);
            if (headLen >= context->header_length + MINI_PACKET_LENGTH - PACKET_TYPE_LENGTH + dataLen) {
                if (givelink_check_crc16(context->header_length, payload, headLen)) {
                    recved = true;
                } else {
                    givelink_shift_data(payload, &headLen);
                }
            }
        } else if (headLen == context->header_length) {
            if (!givelink_check_packet_header(payload)) {
                // wrong packet, ignore
                givelink_shift_data(payload, &headLen);
            }
        }
    } else {
        givelink_shift_data(payload, &headLen);
    }

    *length = headLen;
    return recved;
}
