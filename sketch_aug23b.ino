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

#define DEBUG 1

#define DHTPIN 9     // what digital pin we're connected to

// Uncomment whatever type you're using!
#define DHTTYPE DHT11   // DHT 11
// #define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
// #define DHTTYPE DHT21   // DHT 21 (AM2301)

DHT dht(DHTPIN, DHTTYPE);

#define JSON_LENGTH 50
char jsonPayload[JSON_LENGTH];
uint16_t length;

StaticJsonDocument<JSON_LENGTH> reqJsonData;

void setup() {
    // put your setup code here, to run once:
    Serial.begin(115200);
    while (!Serial) {;}
    #if DEBUG
    Serial.println(F("Setup"));
    #endif
    dht.begin();
}

void loop() {

    if (Serial.available() > 0) {
        outByte = Serial.read();
        if (lora2mqtt_recv(payload, &headLen, outByte)) {
            if (lora2mqtt_from_binary(m, payload, headLen)) {
                #if DEBUG
                Serial.print(F("Recv Id: "));
                Serial.print(m -> id);
                Serial.print(F(" Type: "));
                Serial.print(m -> type);
                if (m -> length > TYPE_LENGTH) {
                    Serial.print(F(" Data: "));
                    for (uint16_t i = 0; i < m -> length - TYPE_LENGTH; i ++) {
                        Serial.write(m -> data[i]);
                    }
                }
                Serial.println();
                #endif

                if (m -> type == REQUEST) {
                    length = m -> length - TYPE_LENGTH;
                    m -> data[length] = '\0';
                    lora2mqtt_set_type(m, RESPONSE);

                    // Deserialize the JSON document
                    DeserializationError error = deserializeJson(reqJsonData, (char *)m -> data);

                    // Test if parsing succeeds.
                    if (error) {
                        #if DEBUG
                        Serial.print(F("deserializeJson() failed: "));
                        Serial.println(error.c_str());
                        #endif
                        set_error(error.c_str());
                    } else {
                        char * method = reqJsonData["method"];
                        if (strcmp("get_dht_value", method) == 0) {
                            read_dht();
                        } else {
                            set_error("no support");
                        }
                    }
                    send_packet();
                }
            }
            headLen = 0;
        }
        if (headLen > MAX_PAYLOAD_LENGTH) {
            #if DEBUG
            Serial.println(F("Error: payload to large"));
            #endif
            headLen = 0;
        }
    }

    if (sendTimer + 10000 < millis()) {
        lora2mqtt_reset(m);
        lora2mqtt_set_id(m, id);
        lora2mqtt_set_type(m, TELEMETRY);
        id ++;
        read_dht();
        send_packet();
    }
}

void send_packet() {
    #if DEBUG
    Serial.print(F("Send Id: "));
    Serial.print(m -> id);
    Serial.print(F(" Type: "));
    Serial.print(m -> type);
    if (m -> length > TYPE_LENGTH) {
        Serial.print(F(" Data: "));
        for (uint16_t i = 0; i < m -> length - TYPE_LENGTH; i ++) {
            Serial.write(m -> data[i]);
        }
    }
    Serial.println();
    #endif
    lora2mqtt_to_binary(m, payloadSend);
    length = lora2mqtt_get_length(m);
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
        Serial.println(F("Failed to read from DHT sensor!"));
        #endif
        sendTimer = millis();
        return;
    }

    // // Compute heat index in Fahrenheit (the default)
    // float hif = dht.computeHeatIndex(f, h);
    // // Compute heat index in Celsius (isFahreheit = false)
    // float hic = dht.computeHeatIndex(t, h, false);

    jsonPayload[0] = '\0';
    int hh = (int)h;
    int hl = (int)((h - hh) * 100);
    int th = (int)t;
    int tl = (int)((t - th) * 100);
    sprintf(jsonPayload, "{\"humidity\": %d.%d, \"temperature\": %d.%d}", hh, hl, th, tl);
    set_data();
}

void set_error(const char * error) {
    jsonPayload[0] = '\0';
    sprintf(jsonPayload, "{\"err\": \"%s\"}", error);
    set_data();
}

void set_data() {
    length = strlen(jsonPayload);
    lora2mqtt_set_data(m, (const uint8_t*)jsonPayload, length);
}
