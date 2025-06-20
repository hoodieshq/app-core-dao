#pragma once

#ifndef DEBUG
#include "../bitcoin_app_base/src/debug-helpers/debug.h"
int semihosted_printf(const char *restrict format, ...);
#define PRINT(...) semihosted_printf("[LOG] "__VA_ARGS__)
#define PRINT_HEX(buffer, len, prefix)                                        \
    semihosted_printf("[LOG] %s", prefix);                                    \
    for (int __PRINT_HEX_I = 0; __PRINT_HEX_I < (int) len; __PRINT_HEX_I++) { \
        semihosted_printf("%02x", buffer[__PRINT_HEX_I]);                     \
    }                                                                         \
    semihosted_printf("\n")
#else
#define PRINT(...)      // Nothing
#define PRINT_HEX(...)  // Nothing
#endif