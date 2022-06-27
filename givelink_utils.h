#ifndef __givelink_utils_h__
#define __givelink_utils_h__

#ifdef __cplusplus
extern "C" {
#endif

void gen_packet(
        uint8_t * pkt,
        const uint8_t * key, const uint16_t keyLen,
        const uint8_t * token, const uint16_t tokenLen,
        const uint16_t id, const uint8_t tp,
        const uint8_t * data, const uint16_t dataLen);

void gen_packet_no_data(
        uint8_t * pkt,
        const uint8_t * key, const uint16_t keyLen,
        const uint8_t * token, const uint16_t tokenLen,
        const uint16_t id, const uint8_t tp);

void gen_packet_with_addr(
        uint8_t * pkt,
        const uint8_t * addr,
        const uint16_t id, const uint8_t tp,
        const uint8_t * data, const uint16_t dataLen);

void gen_packet_with_addr_no_data(
        uint8_t * pkt,
        const uint8_t * addr,
        const uint16_t id, const uint8_t tp);

#ifdef __cplusplus
}
#endif

#endif
