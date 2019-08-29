#include "lora2mqtt.h"

uint8_t inByte = 0;
uint8_t outByte = 0;

unsigned long sendTimer = millis();
uint16_t id = 0;


#define MAX_PAYLOAD_LENGTH 127
uint16_t headLen = 0;
uint8_t payload[128];
uint8_t payloadSend[128];
lora2mqtt_t * m = lora2mqtt_new();

uint8_t hello[20] = "{\"temperature\": 30}";

#define DEBUG 1

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  while (!Serial) {;}
}

void loop() {

    if (Serial.available() > 0) {
        outByte = Serial.read();
        if (lora2mqtt_recv(payload, &headLen, outByte)) {
            if (lora2mqtt_from_binary(m, payload, headLen)) {
                #if DEBUG
                Serial.print("Recv Id: ");
                Serial.print(m -> id);
                Serial.print(" Type: ");
                Serial.print(m -> type);
                if (m -> length > TYPE_LENGTH) {
                    Serial.print(" Data: ");
                    for (uint16_t i = 0; i < m -> length - TYPE_LENGTH; i ++) {
                        Serial.write(m -> data[i]);
                    }
                }
                Serial.println("");
                #endif

                if (m -> type == REQUEST) {
                    uint16_t length = m -> length - TYPE_LENGTH;
                    lora2mqtt_set_type(m, RESPONSE);
                    lora2mqtt_set_data(m, m -> data, length);
                    send_packet();
                }
            }
            headLen = 0;
        }
        if (headLen > MAX_PAYLOAD_LENGTH) {
            #if DEBUG
            Serial.println("Error: payload to large");
            #endif
            headLen = 0;
        }
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
    #if DEBUG
    Serial.print("Send Id: ");
    Serial.print(m -> id);
    Serial.print(" Type: ");
    Serial.print(m -> type);
    if (m -> length > TYPE_LENGTH) {
        Serial.print(" Data: ");
        for (uint16_t i = 0; i < m -> length - TYPE_LENGTH; i ++) {
            Serial.write(m -> data[i]);
        }
    }
    Serial.println("");
    #endif
    lora2mqtt_to_binary(m, payloadSend);
    uint16_t length = lora2mqtt_get_length(m);
    for (uint16_t i = 0; i < length; i ++) {
        Serial.write(payloadSend[i]);
    }
    Serial.write('\r');
    Serial.write('\n');
    sendTimer = millis();
}
