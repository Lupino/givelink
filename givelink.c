#include "givelink.h"
#include <string.h>

givelink_context_t * context;
givelink_t * gl_obj;

bool givelink_context_authed() {
    return context->authed;
}

void givelink_context_set_auth(bool authed) {
    context->authed = authed;
    if (authed) {
        context->header_length = context->authed_header_length;
    } else {
        context->header_length = context->unauth_header_length;
    }
}

void givelink_context_set_unauth_magic() {
    context->unauth_header_start = 0;
    context->unauth_header_length = MAGIC_LENGTH;
    memcpy(context->buffer, (uint8_t *)GLP0, MAGIC_LENGTH);
}

void givelink_context_set_authed_magic() {
    context->authed_header_start = context->unauth_header_start+context->unauth_header_length;
    context->authed_header_length = MAGIC_LENGTH;
    memcpy(context->buffer+context->authed_header_start, (uint8_t *)GLP0, MAGIC_LENGTH);
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

void givelink_to_binary_raw(uint8_t * payload) {
    if (context->authed) {
        memcpy(payload, context->buffer+context->authed_header_start, context->authed_header_length);
    } else {
        memcpy(payload, context->buffer+context->unauth_header_start, context->unauth_header_length);
    }
    memcpy(payload+context->header_length, (const uint8_t *)gl_obj, MINI_PACKET_LENGTH);
    memcpy(payload+context->header_length+MINI_PACKET_LENGTH, gl_obj->data, gl_obj->length - TYPE_LENGTH);
    swap_edition(payload);
}

void givelink_to_binary(uint8_t * payload) {
    givelink_to_binary_raw(payload);

    payload[context->header_length + 4] = 0;
    payload[context->header_length + 5] = 0;

    uint16_t crc = givelink_crc16(payload, givelink_get_length());

    uint8_t crch;
    uint8_t crcl;

    from_uint16(crc, &crch, &crcl);

    payload[context->header_length + 4] = crch;
    payload[context->header_length + 5] = crcl;
}

bool givelink_from_binary(const uint8_t * payload,
        const uint16_t length) {
    if (length < context->header_length + MINI_PACKET_LENGTH) {
        return false;
    }

    uint8_t idh = payload[context->header_length];
    uint8_t idl = payload[context->header_length + 1];

    gl_obj -> id = to_uint16(idh, idl);

    uint8_t lenh = payload[context->header_length + 2];
    uint8_t lenl = payload[context->header_length + 3];

    gl_obj -> length = to_uint16(lenh, lenl);

    uint8_t crch = payload[context->header_length + 4];
    uint8_t crcl = payload[context->header_length + 5];

    gl_obj -> crc16 = to_uint16(crch, crcl);

    gl_obj -> type = payload[context->header_length + 6];

    if (gl_obj -> length > TYPE_LENGTH) {
        memcpy(gl_obj->data, payload + context->header_length + MINI_PACKET_LENGTH,
                length - context->header_length - MINI_PACKET_LENGTH);
    }

    gl_obj -> data[gl_obj -> length - TYPE_LENGTH] = '\0';

    if (gl_obj -> type == AUTHRES) {
        givelink_context_set_addr(gl_obj->data, gl_obj->length - TYPE_LENGTH);
        givelink_context_set_auth(true);
    }

    if (gl_obj -> type == DROP) {
        givelink_context_set_auth(false);
    }

    return true;
}

uint16_t givelink_get_length() {
    return gl_obj -> length + context->header_length + MINI_PACKET_LENGTH - TYPE_LENGTH;
}

uint16_t givelink_get_data_length0(const uint8_t * payload,
        const uint16_t length) {
    if (length < context->header_length + MINI_PACKET_LENGTH - TYPE_LENGTH) {
        return 0;
    }
    uint8_t lenh = payload[context->header_length + 2];
    uint8_t lenl = payload[context->header_length + 3];
    return to_uint16(lenh, lenl);
}

uint16_t givelink_get_data_length() {
    return gl_obj->length - TYPE_LENGTH;
}

void givelink_init(givelink_t * m, uint8_t *data) {
    gl_obj = m;
    gl_obj->id = 0;
    gl_obj->length = TYPE_LENGTH;
    gl_obj->crc16 = 0;
    gl_obj->type = PING;
    gl_obj->data = data;
}

void givelink_reset() {
    gl_obj -> id = 0;
    gl_obj -> length = TYPE_LENGTH;
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
    gl_obj->length = TYPE_LENGTH + length;
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

    uint16_t crc = givelink_crc16(payload, length);

    payload[context->header_length + 4] = crch;
    payload[context->header_length + 5] = crcl;

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
            if (!givelink_check_packet_header(payload)) {
                // wrong packet, ignore
                headLen = 0;
            }
        } else if (headLen >= context->header_length + MINI_PACKET_LENGTH - 1) {
            uint16_t dataLen = givelink_get_data_length0(payload, headLen);
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


#if !defined(__SAM3X8E__)
#if defined(__AVR__ ) || defined(__IMXRT1052__) || defined(__IMXRT1062__) || defined(ARDUINO_ARCH_STM32F1)
#include <avr/pgmspace.h>
#elif defined(ARDUINO_ARCH_STM8)
#elif !defined(__arm__)
#include <pgmspace.h>
#endif
#endif

#if defined(__arm__) || defined(ARDUINO_ARCH_STM8)
const uint16_t crc_tab16[256] = {
#else
const uint16_t crc_tab16[256] PROGMEM = {
#endif
    0x0000, 0xc0c1, 0xc181, 0x0140
  , 0xc301, 0x03c0, 0x0280, 0xc241
  , 0xc601, 0x06c0, 0x0780, 0xc741
  , 0x0500, 0xc5c1, 0xc481, 0x0440
  , 0xcc01, 0x0cc0, 0x0d80, 0xcd41
  , 0x0f00, 0xcfc1, 0xce81, 0x0e40
  , 0x0a00, 0xcac1, 0xcb81, 0x0b40
  , 0xc901, 0x09c0, 0x0880, 0xc841
  , 0xd801, 0x18c0, 0x1980, 0xd941
  , 0x1b00, 0xdbc1, 0xda81, 0x1a40
  , 0x1e00, 0xdec1, 0xdf81, 0x1f40
  , 0xdd01, 0x1dc0, 0x1c80, 0xdc41
  , 0x1400, 0xd4c1, 0xd581, 0x1540
  , 0xd701, 0x17c0, 0x1680, 0xd641
  , 0xd201, 0x12c0, 0x1380, 0xd341
  , 0x1100, 0xd1c1, 0xd081, 0x1040
  , 0xf001, 0x30c0, 0x3180, 0xf141
  , 0x3300, 0xf3c1, 0xf281, 0x3240
  , 0x3600, 0xf6c1, 0xf781, 0x3740
  , 0xf501, 0x35c0, 0x3480, 0xf441
  , 0x3c00, 0xfcc1, 0xfd81, 0x3d40
  , 0xff01, 0x3fc0, 0x3e80, 0xfe41
  , 0xfa01, 0x3ac0, 0x3b80, 0xfb41
  , 0x3900, 0xf9c1, 0xf881, 0x3840
  , 0x2800, 0xe8c1, 0xe981, 0x2940
  , 0xeb01, 0x2bc0, 0x2a80, 0xea41
  , 0xee01, 0x2ec0, 0x2f80, 0xef41
  , 0x2d00, 0xedc1, 0xec81, 0x2c40
  , 0xe401, 0x24c0, 0x2580, 0xe541
  , 0x2700, 0xe7c1, 0xe681, 0x2640
  , 0x2200, 0xe2c1, 0xe381, 0x2340
  , 0xe101, 0x21c0, 0x2080, 0xe041
  , 0xa001, 0x60c0, 0x6180, 0xa141
  , 0x6300, 0xa3c1, 0xa281, 0x6240
  , 0x6600, 0xa6c1, 0xa781, 0x6740
  , 0xa501, 0x65c0, 0x6480, 0xa441
  , 0x6c00, 0xacc1, 0xad81, 0x6d40
  , 0xaf01, 0x6fc0, 0x6e80, 0xae41
  , 0xaa01, 0x6ac0, 0x6b80, 0xab41
  , 0x6900, 0xa9c1, 0xa881, 0x6840
  , 0x7800, 0xb8c1, 0xb981, 0x7940
  , 0xbb01, 0x7bc0, 0x7a80, 0xba41
  , 0xbe01, 0x7ec0, 0x7f80, 0xbf41
  , 0x7d00, 0xbdc1, 0xbc81, 0x7c40
  , 0xb401, 0x74c0, 0x7580, 0xb541
  , 0x7700, 0xb7c1, 0xb681, 0x7640
  , 0x7200, 0xb2c1, 0xb381, 0x7340
  , 0xb101, 0x71c0, 0x7080, 0xb041
  , 0x5000, 0x90c1, 0x9181, 0x5140
  , 0x9301, 0x53c0, 0x5280, 0x9241
  , 0x9601, 0x56c0, 0x5780, 0x9741
  , 0x5500, 0x95c1, 0x9481, 0x5440
  , 0x9c01, 0x5cc0, 0x5d80, 0x9d41
  , 0x5f00, 0x9fc1, 0x9e81, 0x5e40
  , 0x5a00, 0x9ac1, 0x9b81, 0x5b40
  , 0x9901, 0x59c0, 0x5880, 0x9841
  , 0x8801, 0x48c0, 0x4980, 0x8941
  , 0x4b00, 0x8bc1, 0x8a81, 0x4a40
  , 0x4e00, 0x8ec1, 0x8f81, 0x4f40
  , 0x8d01, 0x4dc0, 0x4c80, 0x8c41
  , 0x4400, 0x84c1, 0x8581, 0x4540
  , 0x8701, 0x47c0, 0x4680, 0x8641
  , 0x8201, 0x42c0, 0x4380, 0x8341
  , 0x4100, 0x81c1, 0x8081, 0x4040
};

/*
 * uint16_t givelink_crc16( const uint8_t *input_str, size_t num_bytes );
 *
 * The function givelink_crc16() calculates the 16 bits CRC16 in one pass for a byte
 * string of which the beginning has been passed to the function. The number of
 * bytes to check is also a parameter. The number of the bytes in the string is
 * limited by the constant SIZE_MAX.
 */

uint16_t givelink_crc16( const uint8_t *input_str, size_t num_bytes ) {

  uint16_t crc;
  const uint8_t *ptr;
  size_t a;

  crc = 0x0000;
  ptr = input_str;

  if ( ptr != NULL ) for (a=0; a<num_bytes; a++) {

    #if defined(__arm__) || defined(ARDUINO_ARCH_STM8)
    crc = (crc >> 8) ^ crc_tab16[ (crc ^ (uint16_t) *ptr++) & 0x00FF ];
    #else
    crc = (crc >> 8) ^ pgm_read_word(&crc_tab16[ (crc ^ (uint16_t) *ptr++) & 0x00FF ]);
    #endif
  }

  return crc;

}  /* givelink_crc16 */
