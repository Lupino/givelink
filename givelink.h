#ifndef __givelink_h__
#define __givelink_h__

#ifdef __cplusplus
extern "C" {
#endif

#include "givelink_common.h"

#define MINI_PACKET_LENGTH PACKET_ID_LENGTH + PACKET_LENGTH_LENGTH + PACKET_CRC_LENGTH + PACKET_TYPE_LENGTH

typedef struct {
    uint16_t id;
    uint16_t length;
    uint16_t crc16;
    uint8_t  type;
    uint8_t  *data;
#if defined(ARDUINO_ARCH_STM8)
} givelink_t;
#else
} __attribute__ ((packed)) givelink_t;
#endif

typedef struct {
    uint8_t * buffer;
    uint16_t unauth_header_start;
    uint16_t unauth_header_length;
    uint16_t authed_header_start;
    uint16_t authed_header_length;
    uint16_t header_length;
    bool authed;
} givelink_context_t;

void givelink_init(givelink_t * m, uint8_t *data);
void givelink_reset();
void givelink_set_id(const uint16_t id);
void givelink_set_type(const uint8_t type);
void givelink_set_data(const uint8_t * data, const uint16_t length);
void givelink_set_data_length(const uint16_t length);
uint16_t givelink_get_data_length();

void givelink_to_binary(uint8_t * payload);
bool givelink_from_binary(const uint8_t * payload, const uint16_t length);

uint16_t givelink_get_length();

bool givelink_recv(uint8_t * payload, uint16_t * length, const uint8_t c, bool *crc);

bool givelink_context_authed();
void givelink_context_set_auth(bool authed);
void givelink_context_init(givelink_context_t * ctx, uint8_t * unauth_header);
void givelink_context_set_key(const uint8_t * key, const uint16_t key_len);
void givelink_context_set_token(const uint8_t * token, const uint16_t token_len);
void givelink_context_set_addr(const uint8_t * addr, const uint16_t addr_len);

#ifdef __cplusplus
}
#endif

#endif
