idf_component_register(SRCS "main.c"
                    REQUIRES mqtt esp_driver_gpio esp_timer esp_wifi esp_event esp_netif nvs_flash
                    INCLUDE_DIRS ".")
