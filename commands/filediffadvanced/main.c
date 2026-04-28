#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "util.h"
#include "mmap_io.h"
#include "signals.h"
#include "metrics.h"
#include "text_diff.h"
#include "binary_diff.h"
#include "parallel.h"

#define MODE_AUTO   0
#define MODE_TEXT   1
#define MODE_BINARY 2

#define FMT_UNIFIED     0
#define FMT_SIDE_BY_SIDE 1
#define FMT_BRIEF       2

typedef struct {
    const char *file1;
    const char *file2;
    const char *output_file;
    int mode;
    int format;
    int context_lines;
    int parallel;
    int num_threads;
    int show_metrics;
    int verbose;
} options_t;

static void print_usage(const char *prog) {
    fprintf(stderr,
        "Usage: %s [OPTIONS] FILE1 FILE2\n\n"
        "Compare files for textual and binary differences with performance metrics.\n\n"
        "Options:\n"
        "  -t, --text           Force text comparison mode\n"
        "  -b, --binary         Force binary comparison mode\n"
        "  -u, --unified        Unified diff output (default for text)\n"
        "  -y, --side-by-side   Side-by-side diff output\n"
        "  -q, --brief          Only report whether files differ\n"
        "  -p, --parallel       Enable multithreaded binary comparison\n"
        "  -j, --threads=N      Number of threads (default: 4)\n"
        "  -m, --metrics        Show performance metrics after comparison\n"
        "  -c, --context=N      Lines of context around changes (default: 3)\n"
        "  -v, --verbose        Verbose output\n"
        "  -h, --help           Show this help message\n"
        "  -V, --version        Show version\n",
        prog);
}

static void print_version(void) {
    fprintf(stderr, "filediffadvanced 1.0\n");
}

static int parse_args(int argc, char *argv[], options_t *opts) {
    opts->file1 = NULL;
    opts->file2 = NULL;
    opts->output_file = NULL;
    opts->mode = MODE_AUTO;
    opts->format = FMT_UNIFIED;
    opts->context_lines = 3;
    opts->parallel = 0;
    opts->num_threads = 4;
    opts->show_metrics = 0;
    opts->verbose = 0;

    static struct option long_options[] = {
        {"text",         no_argument,       NULL, 't'},
        {"binary",       no_argument,       NULL, 'b'},
        {"unified",      no_argument,       NULL, 'u'},
        {"side-by-side", no_argument,       NULL, 'y'},
        {"brief",        no_argument,       NULL, 'q'},
        {"parallel",     no_argument,       NULL, 'p'},
        {"threads",      required_argument, NULL, 'j'},
        {"metrics",      no_argument,       NULL, 'm'},
        {"output",       required_argument, NULL, 'o'},
        {"context",      required_argument, NULL, 'c'},
        {"verbose",      no_argument,       NULL, 'v'},
        {"help",         no_argument,       NULL, 'h'},
        {"version",      no_argument,       NULL, 'V'},
        {NULL, 0, NULL, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "tbuyqpj:mo:c:vhV", long_options, NULL)) != -1) {
        switch (opt) {
            case 't': opts->mode = MODE_TEXT; break;
            case 'b': opts->mode = MODE_BINARY; break;
            case 'u': opts->format = FMT_UNIFIED; break;
            case 'y': opts->format = FMT_SIDE_BY_SIDE; break;
            case 'q': opts->format = FMT_BRIEF; break;
            case 'p': opts->parallel = 1; break;
            case 'j': opts->num_threads = atoi(optarg); break;
            case 'm': opts->show_metrics = 1; break;
            case 'o': opts->output_file = optarg; break;
            case 'c': opts->context_lines = atoi(optarg); break;
            case 'v': opts->verbose = 1; break;
            case 'h': print_usage(argv[0]); exit(0);
            case 'V': print_version(); exit(0);
            default:  print_usage(argv[0]); return -1;
        }
    }

    if (argc - optind != 2) {
        print_usage(argv[0]);
        return -1;
    }

    opts->file1 = argv[optind];
    opts->file2 = argv[optind + 1];

    if (opts->num_threads < 1) opts->num_threads = 1;
    if (opts->num_threads > 64) opts->num_threads = 64;
    if (opts->context_lines < 0) opts->context_lines = 0;

    return 0;
}

int main(int argc, char *argv[]) {
    setup_signals();

    options_t opts;
    if (parse_args(argc, argv, &opts) < 0)
        return 2;

    mapped_file_t f1, f2;
    if (mmap_file(opts.file1, &f1) < 0)
        return 2;
    if (mmap_file(opts.file2, &f2) < 0) {
        mmap_close(&f1);
        return 2;
    }

    if (opts.verbose) {
        char buf1[32], buf2[32];
        fprintf(stderr, "File 1: %s (%s)\n", opts.file1, human_size(f1.size, buf1, sizeof(buf1)));
        fprintf(stderr, "File 2: %s (%s)\n", opts.file2, human_size(f2.size, buf2, sizeof(buf2)));
    }

    int mode = opts.mode;
    if (mode == MODE_AUTO) {
        int bin1 = f1.data ? is_binary(f1.data, f1.size) : 0;
        int bin2 = f2.data ? is_binary(f2.data, f2.size) : 0;
        mode = (bin1 || bin2) ? MODE_BINARY : MODE_TEXT;
        if (opts.verbose)
            fprintf(stderr, "Auto-detected mode: %s\n", mode == MODE_BINARY ? "binary" : "text");
    }

    FILE *out = stdout;
    if (opts.output_file) {
        out = fopen(opts.output_file, "w");
        if (!out) {
            perror(opts.output_file);
            mmap_close(&f1);
            mmap_close(&f2);
            return 2;
        }
    }

    perf_metrics_t pm;
    metrics_start(&pm);
    pm.file1_size = f1.size;
    pm.file2_size = f2.size;

    int result = 0;
    size_t bytes_compared = 0;

    if (opts.format == FMT_BRIEF) {
        if (f1.size != f2.size) {
            fprintf(out, "Files %s and %s differ\n", opts.file1, opts.file2);
            result = 1;
        } else if (f1.size == 0 && f2.size == 0) {
            fprintf(out, "Files %s and %s are identical (both empty)\n", opts.file1, opts.file2);
            result = 0;
        } else if (memcmp(f1.data, f2.data, f1.size) == 0) {
            fprintf(out, "Files %s and %s are identical\n", opts.file1, opts.file2);
            result = 0;
        } else {
            fprintf(out, "Files %s and %s differ\n", opts.file1, opts.file2);
            result = 1;
        }
        bytes_compared = f1.size < f2.size ? f1.size : f2.size;
    } else if (mode == MODE_TEXT) {
        result = text_diff(&f1, &f2, opts.format == FMT_SIDE_BY_SIDE, opts.context_lines, out);
        bytes_compared = f1.size + f2.size;
    } else {
        if (opts.parallel) {
            pm.threads_used = opts.num_threads;
            result = parallel_binary_diff(&f1, &f2, opts.num_threads, out, &bytes_compared);
        } else {
            result = binary_diff(&f1, &f2, 0, out, &bytes_compared);
        }
    }

    pm.bytes_compared = bytes_compared;
    metrics_stop(&pm);

    if (opts.show_metrics)
        metrics_print(&pm);

    if (opts.output_file && out != stdout)
        fclose(out);

    mmap_close(&f1);
    mmap_close(&f2);

    if (result < 0) return 2;
    return result ? 1 : 0;
}
