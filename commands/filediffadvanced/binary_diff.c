#include "binary_diff.h"
#include "signals.h"
#include <string.h>

int binary_diff(const mapped_file_t *f1, const mapped_file_t *f2,
                int brief, FILE *out, size_t *bytes_compared) {
    size_t min_size = f1->size < f2->size ? f1->size : f2->size;
    size_t max_size = f1->size > f2->size ? f1->size : f2->size;
    size_t diff_count = 0;
    int differs = 0;

    if (f1->size != f2->size) {
        differs = 1;
        if (brief) {
            fprintf(out, "Files %s and %s differ (sizes: %zu vs %zu bytes)\n",
                    f1->path, f2->path, f1->size, f2->size);
            *bytes_compared = 0;
            return 1;
        }
        fprintf(out, "File sizes differ: %s (%zu bytes) vs %s (%zu bytes)\n",
                f1->path, f1->size, f2->path, f2->size);
    }

    size_t block_size = 4096;
    for (size_t offset = 0; offset < min_size; offset += block_size) {
        if (g_interrupted) break;

        size_t chunk = block_size;
        if (offset + chunk > min_size) chunk = min_size - offset;

        if (memcmp(f1->data + offset, f2->data + offset, chunk) != 0) {
            differs = 1;
            if (brief) {
                fprintf(out, "Files %s and %s differ\n", f1->path, f2->path);
                *bytes_compared = offset + chunk;
                return 1;
            }
            for (size_t i = 0; i < chunk; i++) {
                size_t pos = offset + i;
                if (f1->data[pos] != f2->data[pos]) {
                    diff_count++;
                    if (diff_count <= 50) {
                        fprintf(out, "offset 0x%08zX: 0x%02X vs 0x%02X\n",
                                pos, f1->data[pos], f2->data[pos]);
                    }
                }
            }
        }

        if (g_progress_request) {
            g_progress_request = 0;
            fprintf(stderr, "Progress: %zu / %zu bytes (%.1f%%)\n",
                    offset + chunk, min_size,
                    100.0 * (double)(offset + chunk) / (double)min_size);
        }
    }

    *bytes_compared = max_size;

    if (diff_count > 50)
        fprintf(out, "... and %zu more differences\n", diff_count - 50);

    if (min_size < max_size) {
        fprintf(out, "Extra %zu bytes in %s\n",
                max_size - min_size,
                f1->size > f2->size ? f1->path : f2->path);
    }

    if (!differs) {
        fprintf(out, "Files %s and %s are identical (%zu bytes)\n",
                f1->path, f2->path, f1->size);
    } else {
        double pct = min_size > 0 ? 100.0 * (1.0 - (double)diff_count / (double)min_size) : 0.0;
        fprintf(out, "\nSummary: %zu bytes compared, %zu differences (%.2f%% match)\n",
                min_size, diff_count, pct);
    }

    return differs;
}
