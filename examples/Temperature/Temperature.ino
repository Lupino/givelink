#include "givelink.h"

#define DEBUG 0

#define USE_L76X 0

#define USE_MICROBIT 0
#define MICROBIT_RX 14
#define MICROBIT_TX 15

#define USE_DHT 1
#define DHTPIN 9     // what digital pin we're connected to
// Uncomment whatever type you're using!
#define DHTTYPE DHT11   // DHT 11
// #define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
// #define DHTTYPE DHT21   // DHT 21 (AM2301)

#define USE_DS18B20 0
#define ONE_WIRE_BUS 9

#define ENABLE_POWER_DOWN 0

#define MAX_PAYLOAD_LENGTH 127
#define JSON_LENGTH 50

#if USE_L76X
#include <SoftwareSerial.h>
#include "DEV_Config.h"
#include "L76X.h"
uint8_t L76X_buff[100] = {0};
uint16_t L76X_buff_size = 0;
GNRMC GPS1;
Coordinates coord;
Coordinates prevcoord;
#endif

#if USE_MICROBIT
#include <Adafruit_Microbit.h>

// see https://github.com/sandeepmistry/arduino-nRF5/pull/205
#include "SoftwareSerial.h"
SoftwareSerial GL_SERIAL(MICROBIT_RX, MICROBIT_TX);
Adafruit_Microbit microbit;
#define OVERSAMPLE 50
#else
#define GL_SERIAL Serial
#endif

#if USE_DHT
// https://github.com/adafruit/DHT-sensor-library.git
#include <DHT.h>
#endif

#if USE_DS18B20
#include <Wire.h>
// https://github.com/PaulStoffregen/OneWire.git
#include <OneWire.h>
// https://github.com/milesburton/Arduino-Temperature-Control-Library.git
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

uint8_t outByte = 0;

unsigned long timedelta = 0;
unsigned long get_current_time();

unsigned long auth_timer = get_current_time();
unsigned long auth_delay = 1000;

unsigned long send_timer = get_current_time();
unsigned long send_delay = 3000;
uint16_t id = 0;

uint16_t headLen = 0;
uint8_t payload[MAX_PAYLOAD_LENGTH + 1];
uint8_t payloadSend[MAX_PAYLOAD_LENGTH + 1];

#define HEX_KEY "bdde6db9f3daf38f3a"
#define HEX_TOKEN "14b61d617a9c428a95542dbd097d7a0e"

givelink_t * m = givelink_new();

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
    givelink_init(HEX_KEY, HEX_TOKEN);
    GL_SERIAL.begin(115200);

    #if USE_L76X
    DEV_Set_Baudrate(115200);
    L76X_Send_Command(SET_NMEA_OUTPUT);
    L76X_Send_Command(SET_NMEA_BAUDRATE_9600);
    DEV_Delay_ms(500);

    DEV_Set_Baudrate(9600);
    DEV_Delay_ms(500);
    L76X_Send_Command(SET_NMEA_OUTPUT);
    L76X_Send_Command(SET_POS_FIX_10S);
    #endif


    #if USE_MICROBIT
    microbit.BTLESerial.begin();
    microbit.BTLESerial.setLocalName("microbit");

    // Start LED matrix driver after radio (required)
    microbit.begin();
    #if DEBUG
    Serial.begin(115200);
    #endif
    #endif

    while (!GL_SERIAL) {;}

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

    #if USE_MICROBIT
    microbit.BTLESerial.poll();
    #endif

    #if USE_L76X
    while (DEV_Uart_Avaliable() > 0) {
        outByte = DEV_Uart_Read();
        if (L76X_recv(L76X_buff, &L76X_buff_size, outByte)) {
            #if DEBUG
            Serial.print("Buff: ");
            for (int i = 0; i < L76X_buff_size; i ++) {
                Serial.write(L76X_buff[i]);
            }
            Serial.println("");
            #endif

            GPS1 = L76X_Gat_GNRMC(L76X_buff, L76X_buff_size);
            #if DEBUG
            Serial.print("Time:");
            Serial.print(GPS1.Time_H);
            Serial.print(":");
            Serial.print(GPS1.Time_M);
            Serial.print(":");
            Serial.println(GPS1.Time_S);
            #endif
            L76X_buff_size = 0;
        }
    }
    #endif

    while (GL_SERIAL.available() > 0) {
        outByte = GL_SERIAL.read();
        if (givelink_recv(payload, &headLen, outByte)) {
            if (givelink_from_binary(m, payload, headLen)) {
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
                    givelink_set_type(m, RESPONSE);

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

                    #if USE_MICROBIT
                    if (strcmp("{\"method\":\"get_temp\"}", (const char *)m -> data) == 0) {
                        if (!microbit_read_temp()) {
                            set_error("microbit read temperature error");
                        }
                    } else {
                        set_error("not support");
                    }
                    #endif

                    #if USE_L76X
                    if (strcmp("{\"method\":\"get_gps\"}", (const char *)m -> data) == 0) {
                        if (!l76x_read_coord(false)) {
                            set_error("l76x read gps error");
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
        givelink_reset(m);
        givelink_set_id(m, id);
        givelink_set_type(m, TELEMETRY);
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
        #if USE_MICROBIT
        if (microbit_read_temp()) {
            send_packet();
        #if ENABLE_POWER_DOWN
        } else {
            can_power_down = true;
        #endif
        }
        #endif
        #if USE_L76X
        if (l76x_read_coord(true)) {
            send_packet();
        #if ENABLE_POWER_DOWN
        } else {
            can_power_down = true;
        #endif
        }
        #endif
    }

    if (!givelink_authed()) {
        if (auth_timer + auth_delay < get_current_time()) {
            givelink_reset(m);
            givelink_set_id(m, id);
            givelink_set_type(m, AUTHREQ);
            id ++;
            send_packet();
            auth_timer = get_current_time();
        }
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
    givelink_to_binary(m, payloadSend);
    length = givelink_get_length(m);
    for (uint16_t i = 0; i < length; i ++) {
        GL_SERIAL.write(payloadSend[i]);
    }
    GL_SERIAL.write('\r');
    GL_SERIAL.write('\n');
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

    char temp[16];
    dtostrf(t, 8, 6, temp);

    char hum[16];
    dtostrf(h, 8, 6, hum);

    sprintf(jsonPayload, FC(F("{\"humidity\": %s, \"temperature\": %s}")), hum, temp);
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
    char temp[16];
    dtostrf(tempC, 8, 6, temp);
    sprintf(jsonPayload, FC(F("{\"temperature\": %s}")), temp);
    set_data();
    return true;
}

#endif

#if USE_MICROBIT
bool microbit_read_temp() {
    // Take 'OVERSAMPLES' measurements and average them!
    float avgtemp = 0;
    for (int i = 0; i < OVERSAMPLE; i++) {
      int32_t temp;
      do {
        temp = microbit.getDieTemp();
      } while (temp == 0);  // re run until we get valid data
      avgtemp += temp;
      delay(1);
    }
    avgtemp /= OVERSAMPLE;

    // Send just the raw reading over bluetooth
    microbit.BTLESerial.println(avgtemp);

    jsonPayload[0] = '\0';
    char temp[16];
    dtostrf(avgtemp, 8, 6, temp);
    sprintf(jsonPayload, FC(F("{\"temperature\": %s}")), temp);
    set_data();
    return true;
}
#endif

#if USE_L76X
bool l76x_read_coord(bool checked) {
    if (GPS1.Status == 0) {
        return false;
    }

    coord = L76X_Baidu_Coordinates();

    if (checked) {
        if (coord.Lat == prevcoord.Lat && coord.Lon == prevcoord.Lon) {
            return false;
        }
    }

    prevcoord.Lat = coord.Lat;
    prevcoord.Lon = coord.Lon;

    jsonPayload[0] = '\0';
    char lat[16];
    char lon[16];
    dtostrf(coord.Lat, 8, 6, lat);
    dtostrf(coord.Lon, 8, 6, lon);

    sprintf(jsonPayload, FC(F("{\"lat\": %s, \"lon\": %s}")), lat, lon);
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
    givelink_set_data(m, (const uint8_t*)jsonPayload, length);
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
