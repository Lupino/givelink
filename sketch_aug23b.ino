#include "lora2mqtt.h"

// https://github.com/adafruit/DHT-sensor-library.git
#include <DHT.h>

// https://github.com/bblanchon/ArduinoJson.git
#include <ArduinoJson.h>

uint8_t inByte = 0;
uint8_t outByte = 0;

unsigned long sendTimer = millis();
uint16_t id = 0;


#define MAX_PAYLOAD_LENGTH 127
uint16_t headLen = 0;
uint8_t payload[128];
uint8_t payloadSend[128];
lora2mqtt_t * m = lora2mqtt_new();

// uint8_t hello[20] = "{\"temperature\": 30}";

#define DEBUG 1

#define DHTPIN 9     // what digital pin we're connected to

// Uncomment whatever type you're using!
#define DHTTYPE DHT11   // DHT 11
// #define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
// #define DHTTYPE DHT21   // DHT 21 (AM2301)

DHT dht(DHTPIN, DHTTYPE);

StaticJsonDocument<50> jsonData;
char jsonPayload[50];

void setup() {
    // put your setup code here, to run once:
    Serial.begin(115200);
    while (!Serial) {;}
    #if DEBUG
    Serial.println("Setup");
    #endif
    dht.begin();
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
        read_dht();
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

void read_dht() {
    // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    float h = dht.readHumidity();
    // Read temperature as Celsius (the default)
    float t = dht.readTemperature();
    // // Read temperature as Fahrenheit (isFahrenheit = true)
    // float f = dht.readTemperature(true);

    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t) /*|| isnan(f) */) {
        #if DEBUG
        Serial.println("Failed to read from DHT sensor!");
        #endif
        sendTimer = millis();
        return;
    }

    // // Compute heat index in Fahrenheit (the default)
    // float hif = dht.computeHeatIndex(f, h);
    // // Compute heat index in Celsius (isFahreheit = false)
    // float hic = dht.computeHeatIndex(t, h, false);

    jsonData["humidity"] = h;
    jsonData["temperature"] = t;

    serializeJson(jsonData, jsonPayload);

    uint16_t length = strlen(jsonPayload);

    lora2mqtt_reset(m);
    lora2mqtt_set_id(m, id);
    id ++;
    lora2mqtt_set_type(m, TELEMETRY);
    lora2mqtt_set_data(m, (const uint8_t* )jsonPayload, length);
    send_packet();
    #if DEBUG
    Serial.println("DHT sensor data sended!");
    #endif
}
