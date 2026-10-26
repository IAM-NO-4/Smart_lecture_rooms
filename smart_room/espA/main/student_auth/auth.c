#include <stdio.h>
#include <string.h>

#include "app_config.h"
#include "auth.h"
#include "sd_card.h"

bool register_student(const char *uid)
{
    if (uid == NULL || strlen(uid) == 0 ||
        strlen(uid) >= MAX_UID_LENGTH) {
        printf("Invalid UID.\n");
        return false;
    }

    FILE *file = fopen(AUTH_FILE, "a+");
    if (file == NULL) {
        printf("Could not open authorization database.\n");
        return false;
    }

    char line[64];
    char stored_uid[MAX_UID_LENGTH];
    int status;
    rewind(file);
    while (fgets(line, sizeof(line), file) != NULL) {
        if (sscanf(line, "%31[^,],%d", stored_uid, &status) == 2) {
            if (strcmp(stored_uid, "UID") == 0) {
                continue;
            }
            if (strcmp(stored_uid, uid) == 0) {
                fclose(file);
                printf("This card is already registered.\n");
                return false;
            }
        }
    }

    if (fprintf(file, "%s,1\n", uid) < 0) {
        fclose(file);
        printf("Failed to write authorization database.\n");
        return false;
    }
    fclose(file);
    printf("Student card registered successfully: %s\n", uid);
    log_event("STUDENT_REGISTERED", uid);
    return true;
}

bool is_student_authorized(const char *uid)
{
    if (uid == NULL) {
        return false;
    }

    FILE *file = fopen(AUTH_FILE, "r");
    if (file == NULL) {
        return false;
    }

    char line[64];
    char stored_uid[MAX_UID_LENGTH];
    int status;
    bool authorized = false;

    while (fgets(line, sizeof(line), file) != NULL) {
        if (sscanf(line, "%31[^,],%d", stored_uid, &status) != 2) {
            continue;
        }
        if (strcmp(stored_uid, "UID") == 0) {
            continue;
        }
        if (strcmp(stored_uid, uid) == 0) {
            authorized = (status == 1);
            break;
        }
    }
    fclose(file);
    return authorized;
}

bool revoke_student(const char *uid)
{
    if (uid == NULL || strlen(uid) == 0) {
        return false;
    }

    FILE *file = fopen(AUTH_FILE, "r");
    if (file == NULL) {
        printf("Authorization database not found.\n");
        return false;
    }

    FILE *temp = fopen(AUTH_TEMP_FILE, "w");
    if (temp == NULL) {
        fclose(file);
        printf("Could not create temporary database.\n");
        return false;
    }

    char line[64];
    char stored_uid[MAX_UID_LENGTH];
    int status;
    bool found = false;

    while (fgets(line, sizeof(line), file) != NULL) {
        if (sscanf(line, "%31[^,],%d", stored_uid, &status) != 2) {
            fputs(line, temp);
            continue;
        }
        if (strcmp(stored_uid, uid) == 0) {
            status = 0;
            found = true;
        }
        fprintf(temp, "%s,%d\n", stored_uid, status);
    }
    fclose(file);
    fclose(temp);

    if (!found) {
        remove(AUTH_TEMP_FILE);
        printf("Student UID not found.\n");
        return false;
    }
    if (remove(AUTH_FILE) != 0) {
        printf("Could not replace authorization database.\n");
        remove(AUTH_TEMP_FILE);
        return false;
    }
    if (rename(AUTH_TEMP_FILE, AUTH_FILE) != 0) {
        printf("Could not install updated authorization database.\n");
        return false;
    }

    printf("Authorization revoked for UID: %s\n", uid);
    log_event("STUDENT_REVOKED", uid);
    return true;
}

void student_authorization(void)
{
    char uid[MAX_UID_LENGTH];
    int choice;

    printf("\n1. Register card\n");
    printf("2. Check authorization\n");
    printf("3. Revoke authorization\n");
    printf("Choice: ");

    if (scanf("%d", &choice) != 1) {
        printf("Invalid choice.\n");
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF) {
        }
        return;
    }

    printf("Enter test UID: ");
    if (scanf("%31s", uid) != 1) {
        printf("Could not read UID.\n");
        return;
    }

    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF) {
    }

    switch (choice) {
        case 1:
            register_student(uid);
            break;
        case 2:
            printf("Authorization: %s\n",
                   is_student_authorized(uid) ? "GRANTED" : "DENIED");
            break;
        case 3:
            revoke_student(uid);
            break;
        default:
            printf("Invalid choice.\n");
            break;
    }
}
