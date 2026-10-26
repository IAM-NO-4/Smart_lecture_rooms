#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#define MQTT_URI "mqtt://broker.hivemq.com:1883"
#define MQTT_TOPIC1 "cocis/room1/status"
#define MQTT_TOPIC2 "cocis/room2/status"

#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASSWORD ""

#define ACCESS_START_HOUR 8
#define ACCESS_START_MINUTE 0
#define ACCESS_END_HOUR 21
#define ACCESS_END_MINUTE 0

#define RFID_CS_PIN 2
#define RFID_SCK_PIN 3
#define RFID_MOSI_PIN 4
#define RFID_MISO_PIN 5
#define RFID_RST_PIN 6

#define SD_MOSI_PIN 4
#define SD_MISO_PIN 5
#define SD_SCLK_PIN 3
#define SD_CS_PIN 7
#define MOUNT_POINT "/sdcard"

#define AUTH_FILE MOUNT_POINT "/authorized_students.csv"
#define AUTH_TEMP_FILE MOUNT_POINT "/auth_temp.csv"
#define MAX_UID_LENGTH 32

#endif
