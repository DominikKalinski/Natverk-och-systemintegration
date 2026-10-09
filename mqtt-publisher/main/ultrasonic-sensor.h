#pragma once

#include "driver/gpio.h"
#include "ultrasonic.h"

class UltrasonicSensor
{
public:
    UltrasonicSensor(int triggerPin, int echoPin, const char*);

    float getDistanceCm();
    int sensorID() const;
    const char* unit() const;
private:
    ultrasonic_sensor_t _sensor;

    static constexpr uint32_t _MAX_DISTANCE_CM = 400;
    uint32_t _errors;
    static int _id;
    const char* _unit;
};