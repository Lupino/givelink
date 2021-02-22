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

void givelink_to_binary(uint8_t * payload);
bool givelink_from_binary(const uint8_t * payload, const uint16_t length);

uint16_t givelink_get_length();

bool givelink_recv(uint8_t * payload, uint16_t * length, const uint8_t c);

bool givelink_context_authed();
void givelink_set_auth(bool authed);
void givelink_context_init(givelink_context_t * ctx, uint8_t * unauth_header);
void givelink_context_set_key(const uint8_t * key, const uint16_t key_len);
void givelink_context_set_token(const uint8_t * token, const uint16_t token_len);

#ifdef __cplusplus
}
#endif

#endif
