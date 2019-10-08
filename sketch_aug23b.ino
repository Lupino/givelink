#include "lora2mqtt.h"

#define DEBUG 0

#define USE_DHT 0
#define DHTPIN 9     // what digital pin we're connected to
// Uncomment whatever type you're using!
#define DHTTYPE DHT11   // DHT 11
// #define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
// #define DHTTYPE DHT21   // DHT 21 (AM2301)

#define USE_DS18B20 1
#define ONE_WIRE_BUS 9

#define KEY "e72ae1431038b939f88b"
#define TOKEN "e01fbebf6b0146039784884e4e5b1080"

#define ENABLE_POWER_DOWN 0

#define MAX_PAYLOAD_LENGTH 127
#define JSON_LENGTH 50

#if USE_DHT
// https://github.com/adafruit/DHT-sensor-library.git
#include <DHT.h>
#endif

#if USE_DS18B20
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

// arrays to hold device address
DeviceAddress insideThermometer;
#endif

#if ENABLE_POWER_DOWN
// https://github.com/rocketscream/Low-Power.git
#include "LowPower.h"
#endif

uint8_t inByte = 0;
uint8_t outByte = 0;

unsigned long timedelta = 0;
unsigned long get_current_time();

unsigned long send_timer = get_current_time();
unsigned long send_delay = 60000;
uint16_t id = 0;

uint16_t headLen = 0;
uint8_t payload[MAX_PAYLOAD_LENGTH + 1];
uint8_t payloadSend[MAX_PAYLOAD_LENGTH + 1];
lora2mqtt_t * m = lora2mqtt_new((const uint8_t *)KEY, (const uint8_t *)TOKEN);

#if USE_DHT
DHT dht(DHTPIN, DHTTYPE);
#endif

char jsonPayload[JSON_LENGTH];
uint16_t length;
char * tpl = (char *)malloc(30);

#if ENABLE_POWER_DOWN
// power down timer
unsigned long wake_timer = get_current_time();
unsigned long wake_delay = 1000;
unsigned long can_power_down = true;
#endif

void setup() {
    // put your setup code here, to run once:
    Serial.begin(115200);
    while (!Serial) {;}
    #if DEBUG
    Serial.println(F("Setup"));
    #endif
    #if USE_DHT
    dht.begin();
    #endif
    #if USE_DS18B20
    sensors.begin();
    #if DEBUG
    Serial.print("Found ");
    Serial.print(sensors.getDeviceCount(), DEC);
    Serial.println(" devices.");
    #endif

    // report parasite power requirements
    #if DEBUG
    Serial.print("Parasite power is: ");
    #endif
    if (sensors.isParasitePowerMode()) {
        #if DEBUG
        Serial.println("ON");
        #endif
    } else {
        #if DEBUG
        Serial.println("OFF");
        #endif
    }

    // Assign address manually. The addresses below will beed to be changed
    // to valid device addresses on your bus. Device address can be retrieved
    // by using either oneWire.search(deviceAddress) or individually via
    // sensors.getAddress(deviceAddress, index)
    // Note that you will need to use your specific address here
    //insideThermometer = { 0x28, 0x1D, 0x39, 0x31, 0x2, 0x0, 0x0, 0xF0 };

    // Method 1:
    // Search for devices on the bus and assign based on an index. Ideally,
    // you would do this to initially discover addresses on the bus and then
    // use those addresses and manually assign them (see above) once you know
    // the devices on your bus (and assuming they don't change).
    if (!sensors.getAddress(insideThermometer, 0)) {
        #if DEBUG
        Serial.print("Unable to find address for Device 0");
        #endif
        delay(10000);
    }

    // method 2: search()
    // search() looks for the next device. Returns 1 if a new address has been
    // returned. A zero might mean that the bus is shorted, there are no devices,
    // or you have already retrieved all of them. It might be a good idea to
    // check the CRC to make sure you didn't get garbage. The order is
    // deterministic. You will always get the same devices in the same order
    //
    // Must be called before search()
    //oneWire.reset_search();
    // assigns the first address found to insideThermometer
    //if (!oneWire.search(insideThermometer)) Serial.println("Unable to find address for insideThermometer");

    // show the addresses we found on the bus
    // Serial.print("Device 0 Address: ");
    // printAddress(insideThermometer);
    // Serial.println();

    // set the resolution to 9 bit (Each Dallas/Maxim device is capable of several different resolutions)
    sensors.setResolution(insideThermometer, 9);
    #endif
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

                    #if USE_DHT
                    if (strcmp("{\"method\":\"get_dht_value\"}", (const char *)m -> data) == 0) {
                        if (!read_dht()) {
                            set_error("read dht error");
                        }
                    } else {
                        set_error("not support");
                    }
                    #endif

                    #if USE_DS18B20
                    if (strcmp("{\"method\":\"get_ds18b20_value\"}", (const char *)m -> data) == 0) {
                        if (!read_ds18b20()) {
                            set_error("read ds18b20 error");
                        }
                    } else {
                        set_error("not support");
                    }
                    #endif

                    send_packet();
                }
                #if ENABLE_POWER_DOWN
                can_power_down = true;
                wake_timer = get_current_time();
                #endif
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

    if (send_timer + send_delay < get_current_time()) {
        #if ENABLE_POWER_DOWN
        can_power_down = false;
        #endif
        lora2mqtt_reset(m);
        lora2mqtt_set_id(m, id);
        lora2mqtt_set_type(m, TELEMETRY);
        id ++;
        #if USE_DHT
        if (read_dht()) {
            send_packet();
        #if ENABLE_POWER_DOWN
        } else {
            can_power_down = true;
        #endif
        }
        #endif
        #if USE_DS18B20
        if (read_ds18b20()) {
            send_packet();
        #if ENABLE_POWER_DOWN
        } else {
            can_power_down = true;
        #endif
        }
        #endif
    }

    #if ENABLE_POWER_DOWN
    if (can_power_down) {
        if (wake_timer + wake_delay < get_current_time()) {
            enter_power_down();
        }
    }
    #endif
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
    send_timer = get_current_time();
}

#if USE_DHT
bool read_dht() {
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
        send_timer = get_current_time();
        return false;
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
    return true;
}
#endif

#if USE_DS18B20
bool read_ds18b20() {
    // call sensors.requestTemperatures() to issue a global temperature
    // request to all devices on the bus
    #if DEBUG
    Serial.print("Requesting temperatures...");
    #endif
    sensors.requestTemperatures(); // Send the command to get temperatures
    #if DEBUG
    Serial.println("DONE");
    #endif
    // method 1 - slower
    //Serial.print("Temp C: ");
    //Serial.print(sensors.getTempC(deviceAddress));
    //Serial.print(" Temp F: ");
    //Serial.print(sensors.getTempF(deviceAddress)); // Makes a second call to getTempC and then converts to Fahrenheit

    // method 2 - faster
    float tempC = sensors.getTempC(insideThermometer);
    #if DEBUG
    Serial.print("Temp C: ");
    Serial.print(tempC);
    Serial.print(" Temp F: ");
    Serial.println(DallasTemperature::toFahrenheit(tempC)); // Converts tempC to Fahrenheit
    #endif

    // Check if any reads failed and exit early (to try again).
    if (isnan(tempC)) {
        #if DEBUG
        Serial.println(F("Failed to read from DS18B20 sensor!"));
        #endif
        send_timer = get_current_time();
        return false;
    }

    jsonPayload[0] = '\0';
    int hh = (int)tempC;
    int hl = (int)((tempC - hh) * 100);
    sprintf(jsonPayload, FC(F("{\"temperature\": %d.%d}")), hh, hl);
    set_data();
    return true;
}

#endif

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

unsigned long get_current_time() {
    return millis() + timedelta;
}

#if ENABLE_POWER_DOWN
void enter_power_down() {
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
    timedelta += 8000;
    wake_timer = get_current_time();
}
#endif
