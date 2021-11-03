#ifndef __givelink_raw_h__
#define __givelink_raw_h__

#ifdef __cplusplus
extern "C" {
#endif

#include "givelink_common.h"

#define MINI_RAW_PACKET_LENGTH PACKET_MAGIC_LENGTH + 1 + PACKET_ADDR_LENGTH + 1 + PACKET_ID_LENGTH + PACKET_LENGTH_LENGTH + PACKET_CRC_LENGTH + PACKET_TYPE_LENGTH

bool     givelink_raw_recv(            uint8_t * payload, uint16_t * length, const uint8_t c);

void     givelink_raw_set_key(         uint8_t * payload, const uint8_t * key, const uint16_t key_len);
void     givelink_raw_set_token(       uint8_t * payload, const uint8_t * token, const uint16_t token_len);
void     givelink_raw_set_id(          uint8_t * payload, const uint16_t id);
void     givelink_raw_set_type(        uint8_t * payload, const uint8_t type);
void     givelink_raw_set_data(        uint8_t * payload, const uint8_t * data, const uint16_t length);
void     givelink_raw_set_crc16(       uint8_t * payload);

uint16_t givelink_raw_get_length(const uint8_t * payload);
uint8_t  givelink_raw_get_type(  const uint8_t * payload);
void     givelink_raw_get_data(  const uint8_t * payload, uint8_t * data, uint16_t *dataLen);
uint16_t givelink_raw_get_id(    const uint8_t * payload);
void     givelink_raw_get_key(   const uint8_t * payload, uint8_t * key, uint16_t * key_len);
void     givelink_raw_set_magic(       uint8_t * payload);

#ifdef __cplusplus
}
#endif

#endif

