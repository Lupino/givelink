#include "lora2mqtt.h"

// https://github.com/adafruit/DHT-sensor-library.git
#include <DHT.h>

// https://github.com/rocketscream/Low-Power.git
#include "LowPower.h"

uint8_t inByte = 0;
uint8_t outByte = 0;

unsigned long sendTimer = millis();
uint16_t id = 0;


#define KEY "e72ae1431038b939f88b"
#define TOKEN "e01fbebf6b0146039784884e4e5b1080"

#define MAX_PAYLOAD_LENGTH 127
uint16_t headLen = 0;
uint8_t payload[MAX_PAYLOAD_LENGTH + 1];
uint8_t payloadSend[MAX_PAYLOAD_LENGTH + 1];
lora2mqtt_t * m = lora2mqtt_new((const uint8_t *)KEY, (const uint8_t *)TOKEN);

#define DEBUG 0

#define DHTPIN 9     // what digital pin we're connected to

// Uncomment whatever type you're using!
#define DHTTYPE DHT11   // DHT 11
// #define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
// #define DHTTYPE DHT21   // DHT 21 (AM2301)

DHT dht(DHTPIN, DHTTYPE);

#define JSON_LENGTH 50
char jsonPayload[JSON_LENGTH];
uint16_t length;
char * tpl = (char *)malloc(30);

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

                    if (strcmp("{\"method\":\"get_dht_value\"}", (const char *)m -> data) == 0) {
                        read_dht();
                    } else {
                        set_error("not support");
                    }

                    send_packet();
                }
                enter_power_down();
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
    sprintf(jsonPayload, FC(F("{\"humidity\": %d.%d, \"temperature\": %d.%d}")), hh, hl, th, tl);
    set_data();
}

void set_error(const char * error) {
    jsonPayload[0] = '\0';
    sprintf(jsonPayload, FC(F("{\"err\": \"%s\"}")), error);
    set_data();
}

void set_data() {
    length = strlen(jsonPayload);
    lora2mqtt_set_data(m, (const uint8_t*)jsonPayload, length);
}

char * FC(const __FlashStringHelper *ifsh) {
    PGM_P p = reinterpret_cast<PGM_P>(ifsh);
    size_t n = 0;
    while (1) {
        unsigned char c = pgm_read_byte(p++);
        tpl[n] = c;
        n++;
        if (c == 0) break;
    }
    return tpl;
}

void enter_power_down() { // 9 seconds
    delay(1000);  // wait serial writed
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
}
