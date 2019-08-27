#include "lora2mqtt.h"

uint8_t inByte = 0;
uint8_t outByte = 0;

unsigned long sendTimer = millis();
uint16_t id = 0;


#define MAX_PAYLOAD_LENGTH 255
uint16_t headLen = 0;
uint8_t payload[256];
lora2mqtt_t * m = lora2mqtt_new();

uint8_t hello[20] = "{\"temperature\": 30}";

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial1.begin(115200);
  while (!Serial) {;}
}

void loop() {
    if (Serial.available() > 0) {
        inByte = Serial.read();
        Serial1.write(inByte);
    }

    if (Serial1.available() > 0) {
        outByte = Serial1.read();
        if (lora2mqtt_recv(payload, &headLen, (uint8_t)outByte)) {
            if (lora2mqtt_from_binary(m, payload, headLen)) {
                Serial.print("\nGet Type: ");
                Serial.println(m -> type);
                if (m -> length > packetTypeLength) {
                    Serial.print("Get Data: ");
                    for (uint16_t i = 0; i < m -> length - packetTypeLength; i ++) {
                        Serial.write(m -> data[i]);
                    }
                    Serial.write('\n');
                }

                if (m -> type == REQUEST) {
                    lora2mqtt_set_type(m, RESPONSE);
                    lora2mqtt_set_data(m, m -> data, m -> length - packetTypeLength);
                    if (sendTimer + 10000 > millis()) {
                        delay(sendTimer + 10000 - millis());
                    }
                    send_packet();
                }
            }
            headLen = 0;
        }
        if (headLen > MAX_PAYLOAD_LENGTH) {
            Serial.println("Error: payload to large");
            headLen = 0;
        }
        Serial.write(outByte);
    }

    if (sendTimer + 10000 < millis()) {
        lora2mqtt_reset(m);
        lora2mqtt_set_id(m, id);
        lora2mqtt_set_type(m, TELEMETRY);
        lora2mqtt_set_data(m, hello, 19);
        send_packet();
        id ++;
    }
}

void send_packet() {
    Serial.println((char *)m->data);
    lora2mqtt_to_binary(m, payload);
    uint16_t length = lora2mqtt_get_length(m);
    for (uint16_t i = 0; i < length; i ++) {
        Serial.write(payload[i]);
        Serial1.write(payload[i]);
    }
    Serial1.write('\r');
    Serial1.write('\n');
    Serial.write('\r');
    Serial.write('\n');
    sendTimer = millis();
}
