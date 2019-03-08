#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <RunningMedian.h>
#include <ArduinoJson.h>
#include <user_interface.h>

#define MQTT_BROKER "192.168.1.10"
#define MQTT_TOPIC "metrics/salt"

#define LED_PIN 2
#define TRIGGER_PIN D1
#define ECHO_PIN D2

#define DISTANCE_REPORT_INTERVAL 5 * 60 * 1000

WiFiManager wifiManager;
WiFiClient wifiClient;
PubSubClient mqttClient = PubSubClient(wifiClient);
RunningMedian runningMedian = RunningMedian(9);

String clientId = "ESP8266_" + String(ESP.getChipId(), HEX);

void setup()
{
    Serial.begin(115200);

    pinMode(TRIGGER_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    pinMode(LED_PIN, OUTPUT);

    // clear the LED
    digitalWrite(LED_PIN, HIGH);

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
    static unsigned long lastReport = 0;

    if (!lastReport || millis() - lastReport > DISTANCE_REPORT_INTERVAL)
    {
        // blink the LED
        digitalWrite(LED_PIN, LOW);

        float measuredDistance = getDistance();
        runningMedian.add(measuredDistance);

        if (mqttClient.connected() || mqttReconnect())
        {
            StaticJsonBuffer<128> jsonBuffer;
            JsonObject &root = jsonBuffer.createObject();
            root["measured"] = measuredDistance;
            root["averaged"] = runningMedian.getMedian();
            root["ramFree"] = system_get_free_heap_size();

            String strJson;
            root.printTo(strJson);

            Serial.printf("MQTT: %s -> %s\n", MQTT_TOPIC, strJson.c_str());
            mqttClient.publish(MQTT_TOPIC, strJson.c_str());
        }

        // turn off the LED
        digitalWrite(LED_PIN, HIGH);

        lastReport = millis();
    }

    delay(1000);
}
