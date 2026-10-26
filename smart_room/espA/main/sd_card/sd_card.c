#include <stdio.h>

#include "app_config.h"
#include "sd_card.h"

#include "esp_log.h"
#include "driver/spi_common.h"
#include "driver/sdspi_host.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

static const char *TAG = "SD_CARD";
static sdmmc_card_t *sd_card;
static bool sd_mounted;

esp_err_t sd_card_init(void)
{
    spi_bus_config_t bus_config = {
        .mosi_io_num = SD_MOSI_PIN,
        .miso_io_num = SD_MISO_PIN,
        .sclk_io_num = SD_SCLK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000
    };

    esp_err_t ret = spi_bus_initialize(
        SPI2_HOST,
        &bus_config,
        SDSPI_DEFAULT_DMA
    );
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI bus initialization failed: %s",
                 esp_err_to_name(ret));
        return ret;
    }

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SPI2_HOST;

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SD_CS_PIN;
    slot_config.host_id = SPI2_HOST;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    ret = esp_vfs_fat_sdspi_mount(
        MOUNT_POINT,
        &host,
        &slot_config,
        &mount_config,
        &sd_card
    );
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SD card mount failed: %s",
                 esp_err_to_name(ret));
        return ret;
    }

    sd_mounted = true;
    ESP_LOGI(TAG, "SD card mounted successfully");
    sdmmc_card_print_info(stdout, sd_card);
    return ESP_OK;
}

esp_err_t log_event(const char *event, const char *details)
{
    if (!sd_mounted) {
        ESP_LOGW(TAG, "Cannot log: SD card is not mounted");
        return ESP_FAIL;
    }

    FILE *file = fopen(MOUNT_POINT "/cocis_log.csv", "a");
    if (file == NULL) {
        ESP_LOGE(TAG, "Could not open log file");
        return ESP_FAIL;
    }

    fprintf(file, "%s,%s\n", event, details);
    if (fclose(file) != 0) {
        ESP_LOGE(TAG, "Failed to close log file correctly");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Event logged: %s", event);
    return ESP_OK;
}

bool sd_card_is_mounted(void)
{
    return sd_mounted;
}
