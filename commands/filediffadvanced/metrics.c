#include "metrics.h"
#include "util.h"
#include <stdio.h>

void metrics_start(perf_metrics_t *pm) {
    clock_gettime(CLOCK_MONOTONIC, &pm->wall_start);
    getrusage(RUSAGE_SELF, &pm->ru_start);
    pm->bytes_compared = 0;
    pm->threads_used = 1;
}

void metrics_stop(perf_metrics_t *pm) {
    clock_gettime(CLOCK_MONOTONIC, &pm->wall_end);
    getrusage(RUSAGE_SELF, &pm->ru_end);
}

static double timespec_diff_ms(struct timespec start, struct timespec end) {
    double s = (double)(end.tv_sec - start.tv_sec) * 1000.0;
    double ns = (double)(end.tv_nsec - start.tv_nsec) / 1000000.0;
    return s + ns;
}

static double timeval_to_ms(struct timeval tv) {
    return (double)tv.tv_sec * 1000.0 + (double)tv.tv_usec / 1000.0;
}

void metrics_print(const perf_metrics_t *pm) {
    double wall_ms = timespec_diff_ms(pm->wall_start, pm->wall_end);
    double user_ms = timeval_to_ms(pm->ru_end.ru_utime) - timeval_to_ms(pm->ru_start.ru_utime);
    double sys_ms = timeval_to_ms(pm->ru_end.ru_stime) - timeval_to_ms(pm->ru_start.ru_stime);

    long peak_rss = pm->ru_end.ru_maxrss;
#ifdef __APPLE__
#else
    peak_rss *= 1024;
#endif

    char buf1[32], buf2[32], buf3[32];
    double throughput = 0.0;
    if (wall_ms > 0.0)
        throughput = ((double)pm->bytes_compared / (1024.0 * 1024.0)) / (wall_ms / 1000.0);

    fprintf(stderr, "\n--- Performance Metrics ---\n");
    fprintf(stderr, "Wall time:        %.2f ms\n", wall_ms);
    fprintf(stderr, "User CPU time:    %.2f ms\n", user_ms);
    fprintf(stderr, "System CPU time:  %.2f ms\n", sys_ms);
    fprintf(stderr, "Peak memory:      %s\n", human_size((size_t)peak_rss, buf1, sizeof(buf1)));
    fprintf(stderr, "File 1 size:      %s\n", human_size(pm->file1_size, buf2, sizeof(buf2)));
    fprintf(stderr, "File 2 size:      %s\n", human_size(pm->file2_size, buf3, sizeof(buf3)));
    fprintf(stderr, "Bytes compared:   %zu\n", pm->bytes_compared);
    fprintf(stderr, "Throughput:       %.2f MB/s\n", throughput);
    fprintf(stderr, "Threads used:     %d\n", pm->threads_used);
}
