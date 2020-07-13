#ifndef __givelink_h__
#define __givelink_h__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>

#define REQUEST   1
#define RESPONSE  2
#define ATTRIBUTE 3
#define TELEMETRY 4
#define PING      5
#define SUCCESS   6
#define ERROR     7
#define AUTHREQ   8
#define AUTHRES   9

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
    uint8_t  data[0];
} __attribute__ ((packed)) givelink_t;

void givelink_init(const char * hex_key, const char * hex_token);
givelink_t * givelink_new();
void givelink_reset(givelink_t * m);
void givelink_set_id(givelink_t * m, const uint16_t id);
void givelink_set_type(givelink_t * m, const uint8_t type);
void givelink_set_data(givelink_t * m, const uint8_t * data, const uint16_t length);

void givelink_to_binary(const givelink_t * m, uint8_t * payload);
bool givelink_from_binary(givelink_t * m, const uint8_t * payload, const uint16_t length);

uint16_t givelink_get_length(const givelink_t *m);

void givelink_free(givelink_t * m);
bool givelink_recv(uint8_t * payload, uint16_t * length, const uint8_t c);

bool givelink_authed();

#ifdef __cplusplus
}
#endif

#endif
