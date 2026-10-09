#include "ultrasonic-sensor.h"

#include "esp_err.h"

int UltrasonicSensor::_id = 0;

UltrasonicSensor::UltrasonicSensor(int triggerPin, int echoPin, const char* unit) : _errors(0), _unit(unit)
{
    _sensor.trigger_pin = static_cast<gpio_num_t>(triggerPin);
    _sensor.echo_pin = static_cast<gpio_num_t>(echoPin);

    ESP_ERROR_CHECK(ultrasonic_init(&_sensor));
    _id++;
}

float UltrasonicSensor::getDistanceCm()
{
    uint32_t distanceCm = 0;

    esp_err_t result = ultrasonic_measure_cm(&_sensor, _MAX_DISTANCE_CM, &distanceCm);

    if (result != ESP_OK)
    {
        _errors++;
        return -1.0f;
    }

    return static_cast<float>(distanceCm);
}

int UltrasonicSensor::sensorID() const
{
    return _id;
}

const char* UltrasonicSensor::unit() const
{
    return _unit;
}