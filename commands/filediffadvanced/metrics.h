#ifndef METRICS_H
#define METRICS_H

#include <stddef.h>
#include <time.h>
#include <sys/resource.h>

typedef struct {
    struct timespec wall_start;
    struct timespec wall_end;
    struct rusage ru_start;
    struct rusage ru_end;
    size_t bytes_compared;
    size_t file1_size;
    size_t file2_size;
    int threads_used;
} perf_metrics_t;

void metrics_start(perf_metrics_t *pm);
void metrics_stop(perf_metrics_t *pm);
void metrics_print(const perf_metrics_t *pm);

#endif
