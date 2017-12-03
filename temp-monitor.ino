// Library
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Hash.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266TrueRandom.h>

#define ONE_WIRE_BUS 4  // DS18B20 pin
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);

// WiFi Settings
const char* ssid = "[WIFI SSID]";
const char* password = "[WIFI PASSWORD]";

// Sensor Settings
const String sensor_name = "[SENSOR NAME]";
const String secret_key = "[SECRET KEY]";
const int read_interval_minutes = 60;

// Request Settings
const String api_endpoint = "[API ENDPOINT]";

// Request variables
char salt[17] = "";
float temp = 0.0;
String json = "";
String hash = "";

HTTPClient http;

void setup() {
  float temp;
  String json;

  Serial.begin(115200);

  // Connect to WiFi
  int attempts = 0;
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    attempts++;
  }

  if (WiFi.status() != WL_CONNECTED) {
    goToSleep();
  }

  DS18B20.requestTemperatures();
  temp = DS18B20.getTempCByIndex(0);

  int stableCount = 0;
  for (int retries = 0; retries < 10 && stableCount < 3; ++retries) {
    DS18B20.requestTemperatures();
    float testTemp = DS18B20.getTempCByIndex(0);
    if (temp == DS18B20.getTempCByIndex(0)) {
      stableCount++;
    }
    else {
      temp = testTemp;
      stableCount = 0;
    }
    delay(500);
  }

  gen_salt(salt, 16);

  hash = sha1(secret_key + sensor_name + temp + salt);

  http.begin(api_endpoint);
  http.addHeader("Content-Type", "application/json");

  json += "{\"sensor_name\":\"";
  json += sensor_name;
  json += "\",\"value\":\"";
  json += temp;
  json += "\",\"salt\":\"";
  json += salt;
  json += "\",\"hash\":\"";
  json += hash;
  json += "\"}";
  http.POST(json);
  http.end();

  Serial.end();

  goToSleep();
}

void loop() {
}

void gen_salt(char *s, const int len) {
  static const char alphanum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

  for (int i = 0; i < len; i++) {
    s[i] = alphanum[ESP8266TrueRandom.random(strlen(alphanum))];
  }
}

void goToSleep() {
  ESP.deepSleep(read_interval_minutes * 60 * 1000000);
}
