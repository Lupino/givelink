#ifndef __givelink_common_h__
#define __givelink_common_h__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#define REQUEST     1
#define RESPONSE    2
#define ATTRIBUTE   3
#define TELEMETRY   4
#define PING        5
#define SUCCESS     6
#define ERROR       7
#define AUTHREQ     8
#define AUTHRES     9
#define TRNS        10
#define CTRLREQ     11
#define CTRLRES     12
#define CTRLREQ1    13
#define DROP        14
#define SWITCH      15
#define SWITCHBEAT  16
#define SYNCTIME    17
#define GETSETTOKEN 18

#define PACKET_MAGIC_LENGTH 4
#define PACKET_TYPE_LENGTH 1
#define PACKET_ID_LENGTH 2
#define PACKET_CRC_LENGTH 2
#define PACKET_LENGTH_LENGTH 2
#define PACKET_ADDR_LENGTH 4

#define PACKET_BROADCAST_HEAD_LENGTH PACKET_MAGIC_LENGTH + 2

#define INIT_CRC 0x0000

// givelink packet magic
#define GLP0 "GLP0"

uint16_t givelink_crc16(const uint8_t *input_str, const size_t num_bytes, const uint16_t init_crc);

uint16_t givelink_bin2uint16(const uint8_t h, const uint8_t l);
void givelink_uint2bin16(const uint16_t src, uint8_t *h, uint8_t *l);
bool givelink_discover_magic(const uint8_t * payload, const uint16_t length);
void givelink_shift_data_raw(uint8_t * payload, const uint16_t length);
void givelink_shift_data(uint8_t * payload, uint16_t * length);

bool givelink_is_broadcast(const uint8_t * payload, const uint16_t length);

#ifdef __cplusplus
}
#endif

#endif
