#ifndef __givelink_common_h__
#define __givelink_common_h__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#define REQUEST    1
#define RESPONSE   2
#define ATTRIBUTE  3
#define TELEMETRY  4
#define PING       5
#define SUCCESS    6
#define ERROR      7
#define AUTHREQ    8
#define AUTHRES    9
#define TRNS       10
#define CTRLREQ    11
#define CTRLRES    12
#define CTRLREQ1   13
#define DROP       14
#define SWITCH     15
#define SWITCHBEAT 16
#define SYNCTIME   17

#define MAGIC_LENGTH 4

#define TYPE_LENGTH 1

#define MINI_PACKET_LENGTH 2 + 2 + 2 + TYPE_LENGTH

// givelink packet magic
#define GLP0 "GLP0"

uint16_t givelink_crc16(const uint8_t *input_str, size_t num_bytes);

uint16_t givelink_touint16(const uint8_t h, const uint8_t l);
void givelink_fromuint16(uint16_t src, uint8_t *h, uint8_t *l);

#ifdef __cplusplus
}
#endif

#endif
