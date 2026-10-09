idf_component_register(SRCS "main.cpp" "mqtt-publisher.cpp" "non-volatile-storage.cpp" "wifi.cpp" "ultrasonic-sensor.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES esp_wifi mqtt nvs_flash ultrasonic
                    EMBED_TXTFILES "server.crt")
