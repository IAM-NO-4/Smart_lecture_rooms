#ifndef AUTH_H
#define AUTH_H

#include <stdbool.h>

bool register_student(const char *uid);
bool is_student_authorized(const char *uid);
bool revoke_student(const char *uid);
void student_authorization(void);

#endif