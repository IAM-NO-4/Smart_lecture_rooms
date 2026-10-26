#include <stdio.h>

#include "admin.h"
#include "rfid.h"
#include "sd_card.h"
#include "time_mng.h"
#include "wifi.h"

#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "SD_CARD";

void app_main(void)
{
    printf("\n=== CoCIS Entrance Controller ===\n");

    wifi_init();
    esp_err_t sd_result = sd_card_init();
    if (sd_result == ESP_OK) {
        log_event("SYSTEM", "ESP-A started");
    } else {
        ESP_LOGW(TAG, "SD card initialization failed: %s",
                 esp_err_to_name(sd_result));
    }

    esp_err_t rfid_result = rfid_init();
    if (rfid_result != ESP_OK) {
        ESP_LOGE(TAG, "RFID initialization failed: %s",
                 esp_err_to_name(rfid_result));
    }

    esp_err_t time_result = time_manager_init();
    if (time_result != ESP_OK) {
        ESP_LOGW(
            TAG,
            "Time is not synchronized; new card entries will be denied"
        );
    }

    xTaskCreate(admin_menu_task, "admin_menu", 4096, NULL, 5, NULL);
}
