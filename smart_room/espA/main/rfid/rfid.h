#ifndef RFID_H
#define RFID_H

#include "esp_err.h"

esp_err_t rfid_init(void);
void process_card_scan(const char *uid);

#endif
