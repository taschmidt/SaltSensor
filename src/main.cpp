#include <Arduino.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <NTPClient.h>

#include "config.h"

#define LED_PIN 2
#define TRIGGER_PIN D1
#define ECHO_PIN D2

#define DISTANCE_REPORT_INTERVAL 5 * 60 * 1000

WiFiManager wifiManager;
BearSSL::WiFiClientSecure secureClient;
WiFiUDP udp;
NTPClient ntpClient(udp);

// CERTIFICATES
X509List ca(AmazonRootCA1_pem, AmazonRootCA1_pem_len);
X509List cert(__373b5b37d9_certificate_pem_crt, __373b5b37d9_certificate_pem_crt_len);
PrivateKey key(__373b5b37d9_private_pem_key, __373b5b37d9_private_pem_key_len);

PubSubClient pubSubClient(AWS_IOT_ADDRESS, 8883, secureClient);
const char *clientId = ("ESP8266_" + String(ESP.getChipId(), HEX)).c_str();

void setup()
{
    Serial.begin(115200);

    pinMode(TRIGGER_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    pinMode(LED_PIN, OUTPUT);

    // clear the LED
    digitalWrite(LED_PIN, HIGH);

    secureClient.setTrustAnchors(&ca);
    secureClient.setClientRSACert(&cert, &key);
    secureClient.setBufferSizes(512, 512);

    ntpClient.begin();

    Serial.println("setup() done.");
}

bool mqttReconnect()
{
    while (!ntpClient.update())
    {
        ntpClient.forceUpdate();
    }

    secureClient.setX509Time(ntpClient.getEpochTime());

    Serial.print("Attempting MQTT connection... ");
    if (pubSubClient.connect(clientId))
    {
        Serial.println("connected.");
        return true;
    }
    else
    {
        Serial.print("failed, rc=");
        Serial.println(pubSubClient.state());

        char buf[256];
        secureClient.getLastSSLError(buf, sizeof(buf) / sizeof(*buf));
        Serial.printf("Last SSL Error: %s\n", buf);

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

    digitalWrite(LED_PIN, LOW);

    if (!lastReport || millis() - lastReport > DISTANCE_REPORT_INTERVAL)
    {
        // blink the LED
        digitalWrite(LED_PIN, LOW);

        if (wifiManager.autoConnect() && (pubSubClient.connected() || mqttReconnect()))
        {
            char buf[128];

            float measuredDistance = getDistance();
            if (measuredDistance > 0)
            {
                StaticJsonDocument<128> doc;
                JsonObject state = doc.createNestedObject("state");
                JsonObject reported = state.createNestedObject("reported");
                reported["distance"] = measuredDistance;
                reported["ramFree"] = system_get_free_heap_size();
                serializeJson(doc, buf, sizeof(buf) / sizeof(*buf));

                Serial.printf("Publishing distance of %f... ", measuredDistance);
                pubSubClient.publish("$aws/things/salt-sensor/shadow/update", buf);
                Serial.println("done.");

                lastReport = millis();
            }
        }
    }

    digitalWrite(LED_PIN, HIGH);

    delay(10000);
}
