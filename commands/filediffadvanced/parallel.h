#ifndef PARALLEL_H
#define PARALLEL_H

#include <stddef.h>
#include <stdio.h>
#include "mmap_io.h"

int parallel_binary_diff(const mapped_file_t *f1, const mapped_file_t *f2,
                         int num_threads, FILE *out, size_t *bytes_compared);

#endif
