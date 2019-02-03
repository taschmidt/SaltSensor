#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <microsmooth.h>
#include <ArduinoJson.h>
#include <user_interface.h>

#define MQTT_BROKER "192.168.7.25"
#define MQTT_TOPIC "metrics/salt"

#define LED_PIN 2
#define TRIGGER_PIN D2
#define ECHO_PIN D1

WiFiManager wifiManager;
WiFiClient wifiClient;
PubSubClient mqttClient = PubSubClient(wifiClient);
uint16_t *statHistory;

String clientId = "ESP8266_" + String(ESP.getChipId(), HEX);

void setup()
{
    Serial.begin(115200);

    pinMode(TRIGGER_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    pinMode(LED_PIN, OUTPUT);

    // clear the LED
    digitalWrite(LED_PIN, HIGH);

    statHistory = ms_init(EMA);

    wifiManager.autoConnect();
    mqttClient.setServer(MQTT_BROKER, 1883);
}

bool mqttReconnect()
{
    Serial.print("Attempting MQTT connection...");
    if (mqttClient.connect(clientId.c_str()))
    {
        Serial.println("connected");
        return true;
    }
    else
    {
        Serial.print("failed, rc=");
        Serial.println(mqttClient.state());
        return false;
    }
}

float getDistance()
{
    // clear the trigger pin
    digitalWrite(TRIGGER_PIN, LOW);
    delayMicroseconds(2);

    // set the trigger pin on HIGH state for 10 micro seconds
    digitalWrite(TRIGGER_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIGGER_PIN, LOW);

    // read the echo pin, returns the sound wave travel time in microseconds
    ulong duration = pulseIn(ECHO_PIN, HIGH);

    // calculate the distance (0.0343 = speed of sound in cm/microsecond)
    float distance = duration * 0.0343 / 2;
    return distance;
}

void loop()
{
    // blink the LED
    digitalWrite(LED_PIN, LOW);

    float measuredDistance = getDistance();
    int rounded = measuredDistance + 0.5;
    int filtered = ema_filter(rounded, statHistory);

    if (mqttClient.connected() || mqttReconnect())
    {
        StaticJsonBuffer<128> jsonBuffer;
        JsonObject &root = jsonBuffer.createObject();
        root["measured"] = measuredDistance;
        root["filtered"] = filtered;
        root["ramFree"] = system_get_free_heap_size();

        String strJson;
        root.printTo(strJson);

        Serial.printf("MQTT: %s -> %s\n", MQTT_TOPIC, strJson.c_str());
        mqttClient.publish(MQTT_TOPIC, strJson.c_str());
    }

    // turn off the LED
    digitalWrite(LED_PIN, HIGH);

    delay(30 * 1000);
}
