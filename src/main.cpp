#include <Arduino.h>
#include "mgos_arduino.h"

#include "MedianFilterLib.h"

MedianFilter<double> medianFilter(25);

extern "C" double getDistance(uint8_t triggerPin, uint8_t echoPin, bool raw)
{
    // clear the trigger pin
    digitalWrite(triggerPin, LOW);
    delayMicroseconds(2);

    // set the trigger pin on HIGH state for 10 micro seconds
    digitalWrite(triggerPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(triggerPin, LOW);

    // read the echo pin, returns the sound wave travel time in microseconds
    unsigned long duration = pulseInLong(echoPin, HIGH);

    // calculate the distance (0.0343 = speed of sound in cm/microsecond)
    double distance = duration * 0.0343 / 2;
    distance = roundf(distance * 100) / 100;

    if (raw)
    {
        return distance;
    }
    else
    {
        medianFilter.AddValue(distance);
        double filtered = medianFilter.GetFiltered();

        printf("distance - raw: %f, filtered: %f\n", distance, filtered);

        return filtered;
    }
}
