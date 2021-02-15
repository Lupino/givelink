#ifndef __givelink_h__
#define __givelink_h__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#define REQUEST   1
#define RESPONSE  2
#define ATTRIBUTE 3
#define TELEMETRY 4
#define PING      5
#define SUCCESS   6
#define ERROR     7
#define AUTHREQ   8
#define AUTHRES   9
#define TRNS      10

#define MAGIC_LENGTH 4
#define ADDR_LENGTH 4

#define TYPE_LENGTH 1

#define MINI_PACKET_LENGTH 2 + 2 + 2 + TYPE_LENGTH

// givelink packet magic
#define GLP0 "GLP0"

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
    uint8_t * unauth_header;
    uint16_t unauth_header_length;
    uint8_t * authed_header;
    uint16_t authed_header_length;
    uint16_t header_length;
    bool authed;
} givelink_context_t;

givelink_t * givelink_new(uint8_t *data);
void givelink_reset(givelink_t * m);
void givelink_set_id(givelink_t * m, const uint16_t id);
void givelink_set_type(givelink_t * m, const uint8_t type);
void givelink_set_data(givelink_t * m, const uint8_t * data, const uint16_t length);

void givelink_to_binary(givelink_context_t * ctx, const givelink_t * m, uint8_t * payload);
bool givelink_from_binary(givelink_context_t * ctx, givelink_t * m, const uint8_t * payload, const uint16_t length);

uint16_t givelink_get_length(givelink_context_t * ctx, const givelink_t *m);

bool givelink_recv(givelink_context_t * ctx, uint8_t * payload, uint16_t * length, const uint8_t c);

bool givelink_authed(givelink_context_t * ctx);
void givelink_set_auth(givelink_context_t * ctx, bool authed);
givelink_context_t * givelink_context_new(uint8_t * unauth_header);
void givelink_context_set_key(givelink_context_t * ctx, const uint8_t * key, const uint16_t key_len);
void givelink_context_set_token(givelink_context_t * ctx, const uint8_t * token, const uint16_t token_len);

#ifdef __cplusplus
}
#endif

#endif
