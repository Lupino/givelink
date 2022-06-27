#include "givelink_raw.h"
#include "givelink_utils.h"

void gen_packet(
        uint8_t * pkt,
        const uint8_t * key, const uint16_t keyLen,
        const uint8_t * token, const uint16_t tokenLen,
        const uint16_t id, const uint8_t tp,
        const uint8_t * data, const uint16_t dataLen) {
    givelink_raw_set_magic(pkt);
    givelink_raw_set_key(pkt, key, keyLen);
    givelink_raw_set_token(pkt, token, tokenLen);
    givelink_raw_set_id(pkt, id);
    givelink_raw_set_type(pkt, tp);
    givelink_raw_set_data(pkt, data, dataLen);
    givelink_raw_set_crc16(pkt);

}

void gen_packet_no_data(
        uint8_t * pkt,
        const uint8_t * key, const uint16_t keyLen,
        const uint8_t * token, const uint16_t tokenLen,
        const uint16_t id, const uint8_t tp) {
    gen_packet(pkt, key, keyLen, token, tokenLen, id, tp, NULL, 0);
}

void gen_packet_with_addr(
        uint8_t * pkt,
        const uint8_t * addr,
        const uint16_t id, const uint8_t tp,
        const uint8_t * data, const uint16_t dataLen) {
    gen_packet(pkt, addr, PACKET_ADDR_LENGTH, NULL, 0, id, tp, data, dataLen);
}

void gen_packet_with_addr_no_data(
        uint8_t * pkt,
        const uint8_t * addr,
        const uint16_t id, const uint8_t tp) {
    gen_packet_with_addr(pkt, addr, id, tp, NULL, 0);
}
