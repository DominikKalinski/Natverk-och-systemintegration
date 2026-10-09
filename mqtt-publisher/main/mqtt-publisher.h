#pragma once
#include "mqtt_client.h"

class MqttPublisher
{
public:
    MqttPublisher(const char*);
    bool start();
    void publish(const char*, const char*);
    bool isConnected() const;
private:
    static void eventHandler(void*, esp_event_base_t, int32_t, void*);
    const char *_broker_uri;
    esp_mqtt_client_handle_t _client;
    bool _connected;
    uint32_t _messages_published;
    uint32_t _error_count;
};