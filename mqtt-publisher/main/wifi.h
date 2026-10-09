#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"


#include "lwip/err.h"
#include "lwip/sys.h"

#include "non-volatile-storage.h"

class Wifi
{
    public:
    Wifi();
    static void event_handler(void*, esp_event_base_t, int32_t, void*);
    void init_sta();
    private:
    int _s_retry_num = 0;
    const char* _TAG;
    EventGroupHandle_t _s_wifi_event_group;
    Nvs _nvs;
};