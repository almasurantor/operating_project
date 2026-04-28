#include "parallel.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct {
    int thread_id;
    const unsigned char *data1;
    const unsigned char *data2;
    size_t offset;
    size_t length;
    size_t diff_count;
    size_t first_diff_off;
    int differs;
} chunk_arg_t;

static pthread_mutex_t output_mutex = PTHREAD_MUTEX_INITIALIZER;
static size_t total_diffs = 0;

static void *compare_chunk(void *arg) {
    chunk_arg_t *ca = (chunk_arg_t *)arg;
    ca->differs = 0;
    ca->diff_count = 0;
    ca->first_diff_off = 0;
    int found_first = 0;

    for (size_t i = 0; i < ca->length; i++) {
        if (ca->data1[ca->offset + i] != ca->data2[ca->offset + i]) {
            ca->diff_count++;
            if (!found_first) {
                ca->first_diff_off = ca->offset + i;
                found_first = 1;
            }
            ca->differs = 1;
        }
    }

    pthread_mutex_lock(&output_mutex);
    total_diffs += ca->diff_count;
    pthread_mutex_unlock(&output_mutex);

    return NULL;
}

int parallel_binary_diff(const mapped_file_t *f1, const mapped_file_t *f2,
                         int num_threads, FILE *out, size_t *bytes_compared) {
    size_t min_size = f1->size < f2->size ? f1->size : f2->size;
    size_t max_size = f1->size > f2->size ? f1->size : f2->size;

    if (f1->size != f2->size) {
        fprintf(out, "File sizes differ: %s (%zu bytes) vs %s (%zu bytes)\n",
                f1->path, f1->size, f2->path, f2->size);
    }

    if (min_size == 0) {
        *bytes_compared = max_size;
        if (f1->size == f2->size)
            fprintf(out, "Both files are empty\n");
        return f1->size != f2->size ? 1 : 0;
    }

    if (num_threads < 1) num_threads = 1;
    if ((size_t)num_threads > min_size) num_threads = (int)min_size;

    total_diffs = 0;

    pthread_t *threads = malloc(sizeof(pthread_t) * (size_t)num_threads);
    chunk_arg_t *args = malloc(sizeof(chunk_arg_t) * (size_t)num_threads);
    if (!threads || !args) {
        free(threads);
        free(args);
        return -1;
    }

    size_t chunk_size = min_size / (size_t)num_threads;
    size_t remainder = min_size % (size_t)num_threads;

    size_t offset = 0;
    for (int i = 0; i < num_threads; i++) {
        args[i].thread_id = i;
        args[i].data1 = f1->data;
        args[i].data2 = f2->data;
        args[i].offset = offset;
        args[i].length = chunk_size + (i < (int)remainder ? 1 : 0);
        offset += args[i].length;
        pthread_create(&threads[i], NULL, compare_chunk, &args[i]);
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    int differs = 0;
    fprintf(out, "\n--- Parallel Comparison Results (%d threads) ---\n", num_threads);
    for (int i = 0; i < num_threads; i++) {
        if (args[i].differs) {
            differs = 1;
            fprintf(out, "Thread %d: chunk [0x%08zX - 0x%08zX]: %zu differences, first at 0x%08zX\n",
                    args[i].thread_id,
                    args[i].offset,
                    args[i].offset + args[i].length - 1,
                    args[i].diff_count,
                    args[i].first_diff_off);
        } else {
            fprintf(out, "Thread %d: chunk [0x%08zX - 0x%08zX]: identical\n",
                    args[i].thread_id,
                    args[i].offset,
                    args[i].offset + args[i].length - 1);
        }
    }

    *bytes_compared = max_size;

    if (!differs && f1->size == f2->size) {
        fprintf(out, "\nFiles are identical (%zu bytes)\n", f1->size);
    } else {
        double pct = min_size > 0 ? 100.0 * (1.0 - (double)total_diffs / (double)min_size) : 0.0;
        fprintf(out, "\nSummary: %zu bytes compared, %zu total differences (%.2f%% match)\n",
                min_size, total_diffs, pct);
    }

    if (min_size < max_size) {
        fprintf(out, "Extra %zu bytes in %s\n",
                max_size - min_size,
                f1->size > f2->size ? f1->path : f2->path);
    }

    free(threads);
    free(args);
    return differs;
}
