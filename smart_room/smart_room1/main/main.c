#include <stdio.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_err.h"
#include "mqtt_client.h"

#define PIR1 GPIO_NUM_2
#define PIR2 GPIO_NUM_3
#define LIGHT GPIO_NUM_4

#define SEQUENCE_TIMEOUT_US 3000000LL

int occupancy = 0;
int first_sensor = 0;
int64_t first_time = 0;

#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASSWORD ""

//#define MQTT_URI "mqtt://test.mosquitto.org"
#define MQTT_URI "mqtt://broker.hivemq.com:1883"
#define MQTT_TOPIC "cocis/room1/status"

static const char *TAG = "ROOM1";

static esp_mqtt_client_handle_t mqtt_client = NULL;
static bool mqtt_connected = false;

extern int occupancy;

static void publish_room_status(int count, bool light_on)
{
    if (!mqtt_connected) {
        ESP_LOGW(TAG, "MQTT disconnected; status not published");
        return;
    }

    char message[100];

    snprintf(
        message,
        sizeof(message),
        "{\"room\":\"Room 1\",\"occupancy\":%d,\"light\":%s}",
        count,
        light_on ? "true" : "false"
    );

    int message_id = esp_mqtt_client_publish(
        mqtt_client,
        MQTT_TOPIC,
        message,
        0,
        1,
        0
    );

    ESP_LOGI(TAG, "Published message ID: %d", message_id);
    ESP_LOGI(TAG, "Message: %s", message);
}

static void mqtt_event_handler(
    void *handler_args,
    esp_event_base_t base,
    int32_t event_id,
    void *event_data)
{
    switch (event_id) {
        case MQTT_EVENT_CONNECTED:
            mqtt_connected = true;
            ESP_LOGI(TAG, "Connected to MQTT broker");
            break;

        case MQTT_EVENT_DISCONNECTED:
            mqtt_connected = false;
            ESP_LOGW(TAG, "Disconnected from MQTT broker");
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT connection error");
            break;

        default:
            break;
    }
}

static void start_mqtt(void)
{
    esp_mqtt_client_config_t mqtt_config = {
        .broker.address.uri = MQTT_URI,
        .credentials.client_id = "room1_controller",
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_config);

    ESP_ERROR_CHECK(
        esp_mqtt_client_register_event(
            mqtt_client,
            ESP_EVENT_ANY_ID,
            mqtt_event_handler,
            NULL
        )
    );

    ESP_ERROR_CHECK(esp_mqtt_client_start(mqtt_client));
}

static void wifi_event_handler(
    void *arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data)
{
    if (event_base == WIFI_EVENT &&
        event_id == WIFI_EVENT_STA_START) {

        esp_wifi_connect();

    } else if (event_base == WIFI_EVENT &&
               event_id == WIFI_EVENT_STA_DISCONNECTED) {

        ESP_LOGW(TAG, "Wi-Fi disconnected; reconnecting");
        esp_wifi_connect();

    } else if (event_base == IP_EVENT &&
               event_id == IP_EVENT_STA_GOT_IP) {

        ESP_LOGI(TAG, "Wi-Fi connected; IP address obtained");

        if (mqtt_client == NULL) {
            start_mqtt();
        }
    }
}

static void start_wifi(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(
        esp_event_handler_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            wifi_event_handler,
            NULL
        )
    );

    ESP_ERROR_CHECK(
        esp_event_handler_register(
            IP_EVENT,
            IP_EVENT_STA_GOT_IP,
            wifi_event_handler,
            NULL
        )
    );

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD
        }
    };

    ESP_ERROR_CHECK(
        esp_wifi_set_mode(WIFI_MODE_STA)
    );

    ESP_ERROR_CHECK(
        esp_wifi_set_config(
            WIFI_IF_STA,
            &wifi_config
        )
    );

    ESP_ERROR_CHECK(esp_wifi_start());
}

void app_main(void)
{
    gpio_config_t pir_config = {
        .pin_bit_mask = (1ULL << PIR1) | (1ULL << PIR2),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    ESP_ERROR_CHECK(gpio_config(&pir_config));

    gpio_config_t light_config = {
        .pin_bit_mask = (1ULL << LIGHT),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    ESP_ERROR_CHECK(gpio_config(&light_config));

    printf("Room 1 controller started\n");


    start_wifi();

    while (1) {
        int pir1 = gpio_get_level(PIR1);
        int pir2 = gpio_get_level(PIR2);
        int64_t now = esp_timer_get_time();


        if (first_sensor != 0 &&
            now - first_time > SEQUENCE_TIMEOUT_US) {
            first_sensor = 0;
        }

        if (pir1 && !pir2 && first_sensor == 0) {
            first_sensor = 1;
            first_time = now;
        }

        // PIR 2 activates first
        else if (pir2 && !pir1 && first_sensor == 0) {
            first_sensor = 2;
            first_time = now;
        }

        // PIR 1 followed by PIR 2: entering
        else if (pir1 && pir2 && first_sensor == 1) {
            occupancy++;
            gpio_set_level(LIGHT, occupancy > 0);
            publish_room_status(occupancy, occupancy > 0);
            first_sensor = 0;

            printf("Entry detected. Occupancy: %d\n", occupancy);
        }


        else if (pir1 && pir2 && first_sensor == 2) {
            if (occupancy > 0) {
                occupancy--;

                gpio_set_level(LIGHT, occupancy > 0);
                publish_room_status(occupancy, occupancy > 0);
            }

            first_sensor = 0;

            printf("Exit detected. Occupancy: %d\n", occupancy);
        }

        // Light follows estimated occupancy
        gpio_set_level(LIGHT, occupancy > 0 ? 1 : 0);

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}