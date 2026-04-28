#ifndef MMAP_IO_H
#define MMAP_IO_H

#include <stddef.h>

typedef struct {
    int fd;
    const char *path;
    unsigned char *data;
    size_t size;
} mapped_file_t;

int mmap_file(const char *path, mapped_file_t *mf);
void mmap_close(mapped_file_t *mf);

#endif
