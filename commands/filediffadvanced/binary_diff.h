#ifndef BINARY_DIFF_H
#define BINARY_DIFF_H

#include <stdio.h>
#include "mmap_io.h"

int binary_diff(const mapped_file_t *f1, const mapped_file_t *f2,
                int brief, FILE *out, size_t *bytes_compared);

#endif
