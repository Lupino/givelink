#include <stdio.h>
#include <string.h>

#include "givelink.h"

uint16_t buff_size = 59;
uint8_t buff[] = {
    0x47, 0x4c, 0x50, 0x30, 0x09, 0xe7, 0x2a, 0xe1, 0x43, 0x10, 0x38, 0xb9,
    0x39, 0xf8, 0x10, 0xe0, 0x1f, 0xbe, 0xbf, 0x6b, 0x01, 0x46, 0x03, 0x97,
    0x84, 0x88, 0x4e, 0x4e, 0x5b, 0x10, 0x80, 0x00, 0x01, 0x00, 0x14, 0x23,
    0xcc, 0x04, 0x7b, 0x22, 0x74, 0x65, 0x6d, 0x70, 0x65, 0x72, 0x61, 0x74,
    0x75, 0x72, 0x65, 0x22, 0x3a, 0x20, 0x33, 0x30, 0x7d, 0x0d, 0x0a
};

uint16_t key_size = 9;
uint8_t key[] = { 0xe7, 0x2a, 0xe1, 0x43, 0x10, 0x38, 0xb9, 0x39, 0xf8 };
uint16_t token_size = 16;
uint8_t token[] = {
    0xe0, 0x1f, 0xbe, 0xbf, 0x6b, 0x01, 0x46, 0x03, 0x97, 0x84, 0x88, 0x4e,
    0x4e, 0x5b, 0x10, 0x80
};

uint8_t ctx_buff[50];
uint8_t data_buff[127];

void print_hex(const uint8_t * str, const uint16_t length) {
    printf("length: %d\n", length);
    for (uint16_t i = 0; i < length; i ++) {
        printf("%02x", str[i]);
    }
    printf("\n");
}

void print_data(uint8_t * data, const uint16_t length) {
    data[length] = '\0';
    printf("data: %s\n", data);
}

int main() {
    givelink_context_t ctx;
    givelink_context_init(&ctx, ctx_buff);
    givelink_context_set_key(key, key_size);
    givelink_context_set_token(token, token_size);
    print_hex(ctx.buffer, ctx.header_length);

    givelink_t data0;
    givelink_t * data = givelink_init(&data0, data_buff);

    uint8_t payload[200];
    uint16_t len = 0;

    for (uint16_t i = 0; i < buff_size; i ++) {
        if (givelink_recv(payload, &len, buff[i])) {
            printf("got\n");
            break;
        }
    }

    print_hex(payload, len);
    givelink_from_binary(data, payload, len);
    printf("payloadLength: %d\n", givelink_get_length(data));

    printf("data length: %d\n", data -> length);
    print_data(data -> data, data -> length - 1);

    givelink_to_binary(data, payload);
    print_hex(payload, givelink_get_length(data));
}
