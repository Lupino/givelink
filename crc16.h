#ifndef __crc16_h__
#define __crc16_h__

#ifdef __cplusplus
extern "C" {
#endif
#include <stdlib.h>
#include <inttypes.h>
uint16_t crc_16( const unsigned char *input_str, size_t num_bytes );
#ifdef __cplusplus
}
#endif

#endif
