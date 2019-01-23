#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <RunningMedian.h>

#define LED_PIN 2
#define TRIGGER_PIN D2
#define ECHO_PIN D1

RunningMedian samples = RunningMedian(20);

void setup()
{
    Serial.begin(115200);
    Serial.println("setup");

    pinMode(TRIGGER_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    pinMode(LED_PIN, OUTPUT);

    // clear the LED
    digitalWrite(LED_PIN, HIGH);

    WiFiManager wifiManager;
    wifiManager.autoConnect();
}

double getDistance()
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
    double distance = duration * 0.0343 / 2;
    return distance;
}

void loop()
{
    // blink the LED
    digitalWrite(LED_PIN, LOW);

    double distance = getDistance();

    samples.add(distance);
    float median = samples.getMedian();

    Serial.printf("Distance: %f cm, count: %d, median: %f\n", distance, samples.getCount(), median);

    // turn on the LED
    digitalWrite(LED_PIN, HIGH);

    delay(60 * 1000);
}
