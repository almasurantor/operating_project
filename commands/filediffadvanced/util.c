#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

void err_exit(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "filediffadvanced: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(2);
}

void warn_msg(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "filediffadvanced: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}

char *human_size(size_t bytes, char *buf, size_t buflen) {
    const char *units[] = {"B", "KB", "MB", "GB", "TB"};
    double size = (double)bytes;
    int i = 0;
    while (size >= 1024.0 && i < 4) {
        size /= 1024.0;
        i++;
    }
    snprintf(buf, buflen, "%.2f %s", size, units[i]);
    return buf;
}

int is_binary(const unsigned char *data, size_t len) {
    size_t check = len < 8192 ? len : 8192;
    for (size_t i = 0; i < check; i++) {
        if (data[i] == '\0')
            return 1;
    }
    return 0;
}
