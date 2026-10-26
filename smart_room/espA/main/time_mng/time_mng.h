#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <stdbool.h>
#include <time.h>
#include "esp_err.h"

esp_err_t time_manager_init(void);
bool time_manager_is_ready(void);
esp_err_t time_manager_get_time(struct tm *current_time);
bool time_manager_matches(int hour, int minute);
bool time_manager_is_within_window(
    int start_hour,
    int start_minute,
    int end_hour,
    int end_minute
);

#endif
