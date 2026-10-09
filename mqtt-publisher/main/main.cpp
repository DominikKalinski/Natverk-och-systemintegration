#include "wifi.h"
#include "mqtt-publisher.h"
#include <string>
#include "ultrasonic-sensor.h"
extern "C" void app_main()
{
    UltrasonicSensor sensor(1, 0, "cm");
    Wifi wifi;
    wifi.init_sta();
    MqttPublisher mqtt("mqtts://192.168.1.115:8883");
    mqtt.start();
    vTaskDelay(pdMS_TO_TICKS(5000));
    while(true)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
        char payload[128];

        float distance = sensor.getDistanceCm();
        if(distance == -1) {continue;}
        int id = sensor.sensorID();
        const char* unit = sensor.unit();
        snprintf(payload, sizeof(payload), "{\n\"sensorIdd\":%d,\n\"unit\":\"%s\",\n\"value\":%.2f\n}", id, unit, distance);

        mqtt.publish("sensor/distance", payload);
    }
}