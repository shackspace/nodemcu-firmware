#ifndef _SDK_OVERRIDE_OSAPI_H_
#define _SDK_OVERRIDE_OSAPI_H_

#include "rom.h"
void ets_timer_arm_new (ETSTimer *a, int b, int c, int isMstimer);

int atoi(const char *nptr);
int os_printf(const char *format, ...) __attribute__ ((format (printf, 1, 2)));
int os_printf_plus(const char *format, ...)  __attribute__ ((format (printf, 1, 2)));

#include_next "osapi.h"

#endif
