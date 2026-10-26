#include <stdio.h>
#include <string.h>

#include "app_config.h"
#include "app_state.h"
#include "auth.h"
#include "rfid.h"
#include "sd_card.h"
#include "time_mng.h"

#include "driver/rc522_spi.h"
#include "esp_log.h"
#include "rc522.h"
#include "rc522_picc.h"

#define MAX_TRACKED_CARDS 64
#define UID_STRING_SIZE (RC522_PICC_UID_SIZE_MAX * 2 + 1)

static const char *TAG = "RFID";

typedef struct {
    char uid[UID_STRING_SIZE];
    bool inside;
} tracked_card_t;

static tracked_card_t tracked_cards[MAX_TRACKED_CARDS];
static rc522_driver_handle_t rc522_driver;
static rc522_handle_t rc522_scanner;

static tracked_card_t *find_tracked_card(const char *uid)
{
    for (size_t i = 0; i < MAX_TRACKED_CARDS; i++) {
        if (tracked_cards[i].uid[0] != '\0' &&
            strcmp(tracked_cards[i].uid, uid) == 0) {
            return &tracked_cards[i];
        }
    }
    return NULL;
}

static tracked_card_t *find_free_card_slot(void)
{
    for (size_t i = 0; i < MAX_TRACKED_CARDS; i++) {
        if (tracked_cards[i].uid[0] == '\0') {
            return &tracked_cards[i];
        }
    }
    return NULL;
}

void process_card_scan(const char *uid)
{
    if (uid == NULL || uid[0] == '\0') {
        ESP_LOGE(TAG, "Ignoring an empty card UID");
        return;
    }

    tracked_card_t *card = find_tracked_card(uid);
    if (card != NULL && card->inside) {
        card->inside = false;
        if (building_occupancy > 0) {
            building_occupancy--;
        }
        printf("EXIT RECORDED: %s\n", uid);
        log_event("EXIT_RECORDED", uid);
        printf("Building occupancy: %d\n", building_occupancy);
        return;
    }

    if (!is_student_authorized(uid)) {
        printf("ACCESS DENIED: Unregistered or unauthorized card %s\n", uid);
        log_event("ACCESS_DENIED", uid);
        return;
    }

    if (!time_manager_is_within_window(
            ACCESS_START_HOUR,
            ACCESS_START_MINUTE,
            ACCESS_END_HOUR,
            ACCESS_END_MINUTE)) {
        if (time_manager_is_ready()) {
            struct tm current_time;
            if (time_manager_get_time(&current_time) == ESP_OK) {
                printf(
                    "ACCESS DENIED: %s, access allowed %02d:%02d-%02d:%02d "
                    "EAT (current time %02d:%02d)\n",
                    uid,
                    ACCESS_START_HOUR,
                    ACCESS_START_MINUTE,
                    ACCESS_END_HOUR,
                    ACCESS_END_MINUTE,
                    current_time.tm_hour,
                    current_time.tm_min
                );
                log_event("ACCESS_OUTSIDE_HOURS", uid);
                return;
            }
        }
        printf(
            "ACCESS DENIED: current time unavailable; "
            "cannot verify access hours\n"
        );
        log_event("ACCESS_TIME_UNAVAILABLE", uid);
        return;
    }

    if (card == NULL) {
        card = find_free_card_slot();
        if (card == NULL) {
            ESP_LOGE(TAG, "Card tracking limit reached; entry denied");
            log_event("ACCESS_DENIED_TRACKING_FULL", uid);
            return;
        }
        snprintf(card->uid, sizeof(card->uid), "%s", uid);
    }

    card->inside = true;
    building_occupancy++;
    printf("ENTRY GRANTED: %s\n", uid);
    log_event("ENTRY_GRANTED", uid);
    printf("Building occupancy: %d\n", building_occupancy);
}

static void on_picc_state_changed(
    void *arg,
    esp_event_base_t base,
    int32_t event_id,
    void *data)
{
    (void)arg;
    (void)base;
    (void)event_id;

    rc522_picc_state_changed_event_t *event =
        (rc522_picc_state_changed_event_t *)data;
    if (event == NULL || event->picc == NULL ||
        event->picc->state != RC522_PICC_STATE_ACTIVE) {
        return;
    }

    char uid[UID_STRING_SIZE] = {0};
    const rc522_picc_uid_t *picc_uid = &event->picc->uid;
    if (picc_uid->length == 0 ||
        picc_uid->length > RC522_PICC_UID_SIZE_MAX) {
        ESP_LOGE(TAG, "Reader returned an invalid UID length: %u",
                 (unsigned int)picc_uid->length);
        return;
    }

    for (size_t i = 0; i < picc_uid->length; i++) {
        snprintf(&uid[i * 2], sizeof(uid) - i * 2, "%02X",
                 picc_uid->value[i]);
    }

    ESP_LOGI(TAG, "Card detected, UID: %s", uid);
    process_card_scan(uid);
}

esp_err_t rfid_init(void)
{
    rc522_spi_config_t driver_config = {
        .host_id = SPI2_HOST,
        .bus_config = NULL,
        .dev_config = {
            .spics_io_num = RFID_CS_PIN,
            .clock_speed_hz = 1000000
        },
        .rst_io_num = RFID_RST_PIN
    };

    esp_err_t err = rc522_spi_create(&driver_config, &rc522_driver);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create SPI driver: %s",
                 esp_err_to_name(err));
        return err;
    }

    err = rc522_driver_install(rc522_driver);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install SPI driver: %s",
                 esp_err_to_name(err));
        return err;
    }

    rc522_config_t scanner_config = {
        .driver = rc522_driver
    };
    err = rc522_create(&scanner_config, &rc522_scanner);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create scanner: %s",
                 esp_err_to_name(err));
        return err;
    }

    err = rc522_register_events(
        rc522_scanner,
        RC522_EVENT_PICC_STATE_CHANGED,
        on_picc_state_changed,
        NULL
    );
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register card event handler: %s",
                 esp_err_to_name(err));
        return err;
    }

    err = rc522_start(rc522_scanner);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start scanner: %s",
                 esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "RFID reader initialized; waiting for cards");
    return ESP_OK;
}
