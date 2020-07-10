#include <stdio.h>
#include <string.h>

#include "lora2mqtt.h"
#include "unhex.h"

uint16_t hex_buff_size = 118;
uint8_t hex_buff[] = "4c4d513009e72ae1431038b939f810e01fbebf6b0146039784884e4e5b1080000100147b22047b2274656d7065726174757265223a2033307d0d0a";

char hex_key[] = "e72ae1431038b939f8";
char hex_token[] = "e01fbebf6b0146039784884e4e5b1080";

void print_hex(const uint8_t * str, const uint16_t length) {
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

    uint16_t buff_size = hex_buff_size / 2;
    uint8_t buff[buff_size];

    memcpy(buff, (uint8_t *)unhex(hex_buff, hex_buff_size), buff_size);

    lora2mqtt_init(hex_key, hex_token);
    lora2mqtt_t * data = lora2mqtt_new();

    uint8_t payload[200];
    uint16_t len = 0;

    for (uint16_t i = 0; i < buff_size; i ++) {
        if (lora2mqtt_recv(payload, &len, buff[i])) {
            printf("got\n");
            break;
        }
    }

    print_hex(payload, len);
    lora2mqtt_from_binary(data, payload, len);
    printf("payloadLength: %d\n", lora2mqtt_get_length(data));

    printf("data length: %d\n", data -> length);
    print_data(data -> data, data -> length - 1);

    lora2mqtt_to_binary(data, payload);
    print_hex(payload, lora2mqtt_get_length(data));
}
