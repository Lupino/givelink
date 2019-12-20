#ifndef __lora2mqtt_h__
#define __lora2mqtt_h__

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

#define MAGIC_LENGTH 4

#define TYPE_LENGTH 1

#define MINI_PACKET_LENGTH 2 + 2 + 2 + TYPE_LENGTH

// Magic
#define LMQ0 "LMQ0"

typedef struct {
    uint16_t id;
    uint16_t length;
    uint16_t crc16;
    uint8_t  type;
    uint8_t  data[0];
} __attribute__ ((packed)) lora2mqtt_t;

lora2mqtt_t * lora2mqtt_new(const char * hex_key, const char * hex_token);
void lora2mqtt_reset(lora2mqtt_t * m);
void lora2mqtt_set_id(lora2mqtt_t * m, const uint16_t id);
void lora2mqtt_set_type(lora2mqtt_t * m, const uint8_t type);
void lora2mqtt_set_data(lora2mqtt_t * m, const uint8_t * data, const uint16_t length);

void lora2mqtt_to_binary(const lora2mqtt_t * m, uint8_t * payload);
bool lora2mqtt_from_binary(lora2mqtt_t * m, const uint8_t * payload, const uint16_t length);

uint16_t lora2mqtt_get_length(const lora2mqtt_t *m);

void lora2mqtt_free(lora2mqtt_t * m);
bool lora2mqtt_recv(uint8_t * payload, uint16_t * length, const uint8_t c);

#ifdef __cplusplus
}
#endif

#endif
