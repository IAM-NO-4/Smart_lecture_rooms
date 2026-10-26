#include <stdlib.h>
#include <time.h>

#include "time_mng.h"

#include "esp_log.h"
#include "esp_netif_sntp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "TIME_MANAGER";
static bool time_synchronized = false;

esp_err_t time_manager_init(void)
{
    // East Africa Time, UTC+3.
    setenv("TZ", "EAT-3", 1);
    tzset();
    ESP_LOGI(TAG, "Starting time synchronization...");

    esp_sntp_config_t config =
        ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    esp_err_t err = esp_netif_sntp_init(&config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SNTP: %s",
                 esp_err_to_name(err));
        return err;
    }

    err = esp_netif_sntp_sync_wait(pdMS_TO_TICKS(15000));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Time synchronization failed: %s",
                 esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "Time synchronized successfully");
    time_synchronized = true;
    return ESP_OK;
}

bool time_manager_is_ready(void)
{
    return time_synchronized;
}

esp_err_t time_manager_get_time(struct tm *current_time)
{
    if (current_time == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!time_manager_is_ready()) {
        return ESP_ERR_INVALID_STATE;
    }

    time_t now;
    time(&now);
    if (localtime_r(&now, current_time) == NULL) {
        return ESP_FAIL;
    }
    return ESP_OK;
}

bool time_manager_matches(int hour, int minute)
{
    struct tm current_time;

    if (hour < 0 || hour > 23 || minute < 0 || minute > 59 ||
        time_manager_get_time(&current_time) != ESP_OK) {
        return false;
    }

    return current_time.tm_hour == hour &&
           current_time.tm_min == minute;
}

bool time_manager_is_within_window(
    int start_hour,
    int start_minute,
    int end_hour,
    int end_minute)
{
    if (start_hour < 0 || start_hour > 23 ||
        start_minute < 0 || start_minute > 59 ||
        end_hour < 0 || end_hour > 23 ||
        end_minute < 0 || end_minute > 59) {
        return false;
    }

    struct tm current_time;
    if (time_manager_get_time(&current_time) != ESP_OK) {
        return false;
    }

    int now = current_time.tm_hour * 60 + current_time.tm_min;
    int start = start_hour * 60 + start_minute;
    int end = end_hour * 60 + end_minute;

    if (start == end) {
        return false;
    }
    if (start < end) {
        return now >= start && now < end;
    }
    return now >= start || now < end;
}
