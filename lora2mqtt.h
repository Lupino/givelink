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

// Magic
#define LMQ0 "LMQ0"

#define KEY "e72ae1431038b939f88b"
#define TOKEN "e01fbebf6b0146039784884e4e5b1080"

typedef struct {
    uint8_t  magic[4];
    uint8_t  key[10];
    uint8_t  token[16];
    uint16_t id;
    uint16_t length;
    uint16_t crc16;
    uint8_t  type;
    uint8_t  data[0];
} __attribute__ ((packed)) lora2mqtt_t;

#define MAGIC_LENGTH 4
#define KEY_LENGTH 10
#define TOKEN_LENGTH 16
#define HEADER_LENGTH MAGIC_LENGTH + KEY_LENGTH + TOKEN_LENGTH + 2 + 2 + 2
#define TYPE_LENGTH 1

lora2mqtt_t * lora2mqtt_new();
void lora2mqtt_reset(lora2mqtt_t * m);
void lora2mqtt_set_id(lora2mqtt_t * m, uint16_t id);
void lora2mqtt_set_type(lora2mqtt_t * m, uint8_t type);
void lora2mqtt_set_data(lora2mqtt_t * m, uint8_t * data, uint16_t length);

void lora2mqtt_to_binary(lora2mqtt_t * m, uint8_t * payload);
bool lora2mqtt_from_binary(lora2mqtt_t * m, uint8_t * payload, uint16_t length);

uint16_t lora2mqtt_get_length(const lora2mqtt_t *m);

void lora2mqtt_free(lora2mqtt_t * m);
bool lora2mqtt_recv(uint8_t * payload, uint16_t * length, uint8_t c);

#ifdef __cplusplus
}
#endif

#endif
