#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>

void err_exit(const char *fmt, ...);
void warn_msg(const char *fmt, ...);
char *human_size(size_t bytes, char *buf, size_t buflen);
int is_binary(const unsigned char *data, size_t len);

#endif
