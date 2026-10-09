#include "mqtt-publisher.h"
#include "mqtt_client.h"
#include "esp_log.h"

extern const char server_cert_start[] asm("_binary_server_crt_start");

static const char *TAG = "MqttPublisher";

MqttPublisher::MqttPublisher(const char* broker_uri) : _broker_uri(broker_uri), _client(nullptr), _connected(false),
_messages_published(0), _error_count(0) {}

bool MqttPublisher::start()
{
    if(_client != nullptr)
    {
        ESP_LOGW(TAG, "MQTT client has already been initialized");
        return false;
    }
    esp_mqtt_client_config_t config = {};
    config.broker.address.uri = _broker_uri;
    config.broker.verification.certificate = server_cert_start;
    
    _client = esp_mqtt_client_init(&config);

    if(_client == nullptr)
    {
        ESP_LOGE(TAG, "Failed to initialize MQTT client");
        return false;
    }

    esp_err_t result = esp_mqtt_client_register_event(_client, MQTT_EVENT_ANY, eventHandler, this);

    if(result != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to register MQTT event handler");
        return false;
    }
    result = esp_mqtt_client_start(_client);
    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to start MQTT client");
        return false;
    }
    return true;
}

void MqttPublisher::publish(const char* topic, const char* message)
{
    if(!_connected)
    {
        ESP_LOGW(TAG, "Publishing failed, MQTT not connected");
        _error_count++;
        return;
    }
    int message_id = esp_mqtt_client_publish(_client, topic, message, 0, 1, 0);
    if(message_id < 0)
    {
        ESP_LOGE(TAG, "Failed to publish message");
        _error_count++;
        return;
    }
    _messages_published++;
    ESP_LOGI(TAG, "Published message with id %d", message_id);
    ESP_LOGI("MONITOR", "Messages published: %lu", _messages_published
    );
}


void MqttPublisher::eventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    MqttPublisher* publisher = static_cast<MqttPublisher*>(arg);
    switch(event_id)
    {
        case MQTT_EVENT_CONNECTED:
        {
            publisher->_connected = true;
            
            ESP_LOGI(TAG, "Connected to MQTT broker");
            printf("CONNECTED FFS\n");
            break;
        }
        
        case MQTT_EVENT_DISCONNECTED:
        {
            publisher->_connected = false;
            ESP_LOGW(TAG, "Disconnected from MQTT broker");
             printf("DISCONNECTED FFS\n");
            break;
        }
        case MQTT_EVENT_ERROR:
        {
            ESP_LOGE(TAG, "MQTT error");
             printf("ERROR FFS\n");
               auto *event = static_cast<esp_mqtt_event_handle_t>(event_data);

            if (event != nullptr && event->error_handle != nullptr)
            {
                auto *error = event->error_handle;
            
                ESP_LOGE(TAG,
                         "error_type=%d tls_err=0x%x tls_stack=%d cert_flags=0x%x socket_errno=%d",
                         error->error_type,
                         error->esp_tls_last_esp_err,
                         error->esp_tls_stack_err,
                         error->esp_tls_cert_verify_flags,
                         error->esp_transport_sock_errno);
            }
            else
            {
                ESP_LOGE(TAG, "MQTT error: no error details");
            }
            break;
        }
        default:
        {
            printf("MQTT event_id: %ld\n", static_cast<long>(event_id));
        } break;
    }
}


