#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "app_config.h"
#include "app_state.h"
#include "admin.h"
#include "auth.h"
#include "sd_card.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static void display_admin_menu(void)
{
    printf("\n");
    printf("====================================\n");
    printf("       COCIS MONITORING SYSTEM      \n");
    printf("        ADMINISTRATOR MENU          \n");
    printf("====================================\n");
    printf("1. View building occupancy\n");
    printf("2. View Room 1 status\n");
    printf("3. View Room 2 status\n");
    printf("4. View SD-card event log\n");
    printf("5. Manage student authorization\n");
    printf("6. Configure access schedule\n");
    printf("7. View system status\n");
    printf("8. Exit menu\n");
    printf("====================================\n");
    printf("Select an option: ");
    fflush(stdout);
}


static void display_building_occupancy(void)
{
    int total_occupancy = room1_occupancy + room2_occupancy;

    printf("\n========== BUILDING OCCUPANCY ==========\n");
    printf("Room 1 Occupancy: %d\n", room1_occupancy);
    printf("Room 2 Occupancy: %d\n", room2_occupancy);
    printf("----------------------------------------\n");
    printf("Total Occupancy:  %d\n", total_occupancy);
    printf("========================================\n");
}

static void display_room1_status(void)
{
    printf("\n========== ROOM 1 STATUS ==========\n");
    printf("Occupancy: %d\n", room1_occupancy);
    printf("Lighting: %s\n", room1_light_on ? "ON" : "OFF");
    printf("===================================\n");
}

static void display_room2_status(void)
{
    printf("\n========== ROOM 2 STATUS ==========\n");
    printf("Occupancy: %d\n", room2_occupancy);
    printf("Lighting: %s\n", room2_light_on ? "ON" : "OFF");
    printf("===================================\n");
}

static void display_event_log(void)
{
    FILE *file = fopen("/sdcard/cocis_log.csv", "r");
    if (file == NULL) {
        printf("\nCould not open event log.\n");
        printf("Check whether the SD card is mounted.\n");
        return;
    }
    char line[256];
    printf("\n========== COCIS EVENT LOG ==========\n");
    while (fgets(line, sizeof(line), file) != NULL) {
        printf("%s", line);
    }
    printf("\n=====================================\n");
    fclose(file);
}

static void display_system_status(void)
{
    printf("\n========== SYSTEM STATUS ==========\n");
    printf("SD Card: %s\n",
           sd_card_is_mounted() ? "Mounted" : "Not mounted");
    printf("Room 1 Occupancy: %d\n", room1_occupancy);
    printf("Room 1 Lighting:  %s\n",
           room1_light_on ? "ON" : "OFF");
    printf("Room 2 Occupancy: %d\n", room2_occupancy);
    printf("Room 2 Lighting:  %s\n",
           room2_light_on ? "ON" : "OFF");
    printf("===================================\n");
}

void admin_menu_task(void *pvParameters)
{
    (void)pvParameters;
    char input[16];

    while (1) {
        display_admin_menu();
        size_t index = 0;

        while (1) {
            int received = getchar();
            if (received == EOF) {
                continue;
            }
            char ch = (char)received;

            if (ch == '\r' || ch == '\n') {
                if (index == 0) {
                    continue;
                }
                input[index] = '\0';
                printf("\n");
                break;
            }

            if (ch == '\b' || ch == 127) {
                if (index > 0) {
                    index--;
                }
                continue;
            }

            if (ch >= '0' && ch <= '9' &&
                index < sizeof(input) - 1) {
                input[index++] = ch;
                putchar(ch);
                fflush(stdout);
            }
        }

        int choice = atoi(input);
        switch (choice) {
            case 1:
                display_building_occupancy();
                break;
            case 2:
                display_room1_status();
                break;
            case 3:
                display_room2_status();
                break;
            case 4:
                display_event_log();
                break;
            case 5:
                student_authorization();
                break;
            case 6:
                printf("Access schedule selected.\n");
                break;
            case 7:
                display_system_status();
                break;
            case 8:
                printf("Administrator menu closed.\n");
                vTaskDelete(NULL);
                break;
            default:
                printf("Invalid option. Enter 1-8.\n");
                break;
        }
    }
}
