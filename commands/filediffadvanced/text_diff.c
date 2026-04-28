#include "text_diff.h"
#include "signals.h"
#include "util.h"
#include <stdlib.h>
#include <string.h>

static unsigned long fnv1a(const char *data, size_t len) {
    unsigned long hash = 2166136261UL;
    for (size_t i = 0; i < len; i++) {
        hash ^= (unsigned char)data[i];
        hash *= 16777619UL;
    }
    return hash;
}

int parse_lines(const unsigned char *data, size_t size, line_t **lines, int *count) {
    if (size == 0) {
        *lines = NULL;
        *count = 0;
        return 0;
    }

    int cap = 1024;
    int n = 0;
    line_t *arr = malloc(sizeof(line_t) * (size_t)cap);
    if (!arr) return -1;

    const char *p = (const char *)data;
    const char *end = p + size;
    const char *line_start = p;

    while (p <= end) {
        if (p == end || *p == '\n') {
            if (n >= cap) {
                cap *= 2;
                line_t *tmp = realloc(arr, sizeof(line_t) * (size_t)cap);
                if (!tmp) { free(arr); return -1; }
                arr = tmp;
            }
            size_t len = (size_t)(p - line_start);
            arr[n].start = line_start;
            arr[n].len = len;
            arr[n].hash = fnv1a(line_start, len);
            n++;
            line_start = p + 1;
            if (p == end) break;
        }
        p++;
    }

    *lines = arr;
    *count = n;
    return 0;
}

static int lines_equal(const line_t *a, const line_t *b) {
    if (a->hash != b->hash) return 0;
    if (a->len != b->len) return 0;
    return memcmp(a->start, b->start, a->len) == 0;
}

int compute_diff(const line_t *lines1, int count1, const line_t *lines2, int count2,
                 diff_entry_t **edits, int *num_edits) {
    int r = count1 + 1;
    int c = count2 + 1;

    int *prev = calloc((size_t)c, sizeof(int));
    int *curr = calloc((size_t)c, sizeof(int));
    if (!prev || !curr) { free(prev); free(curr); return -1; }

    for (int i = 1; i < r; i++) {
        if (g_interrupted) { free(prev); free(curr); return -1; }
        for (int j = 1; j < c; j++) {
            if (lines_equal(&lines1[i - 1], &lines2[j - 1]))
                curr[j] = prev[j - 1] + 1;
            else
                curr[j] = curr[j - 1] > prev[j] ? curr[j - 1] : prev[j];
        }
        int *tmp = prev;
        prev = curr;
        curr = tmp;
        memset(curr, 0, sizeof(int) * (size_t)c);
    }

    free(prev);
    free(curr);

    int **dp = malloc(sizeof(int *) * (size_t)r);
    if (!dp) return -1;
    for (int i = 0; i < r; i++) {
        dp[i] = calloc((size_t)c, sizeof(int));
        if (!dp[i]) {
            for (int k = 0; k < i; k++) free(dp[k]);
            free(dp);
            return -1;
        }
    }

    for (int i = 1; i < r; i++) {
        if (g_interrupted) break;
        for (int j = 1; j < c; j++) {
            if (lines_equal(&lines1[i - 1], &lines2[j - 1]))
                dp[i][j] = dp[i - 1][j - 1] + 1;
            else
                dp[i][j] = dp[i - 1][j] > dp[i][j - 1] ? dp[i - 1][j] : dp[i][j - 1];
        }
    }

    int cap = count1 + count2;
    diff_entry_t *result = malloc(sizeof(diff_entry_t) * (size_t)cap);
    if (!result) {
        for (int i = 0; i < r; i++) free(dp[i]);
        free(dp);
        return -1;
    }

    int n = 0;
    int i = count1, j = count2;
    while (i > 0 || j > 0) {
        if (g_interrupted) break;
        if (i > 0 && j > 0 && lines_equal(&lines1[i - 1], &lines2[j - 1])) {
            result[n].op = DIFF_EQUAL;
            result[n].line_num1 = i;
            result[n].line_num2 = j;
            result[n].line = &lines1[i - 1];
            n++; i--; j--;
        } else if (j > 0 && (i == 0 || dp[i][j - 1] >= dp[i - 1][j])) {
            result[n].op = DIFF_ADD;
            result[n].line_num1 = -1;
            result[n].line_num2 = j;
            result[n].line = &lines2[j - 1];
            n++; j--;
        } else {
            result[n].op = DIFF_DEL;
            result[n].line_num1 = i;
            result[n].line_num2 = -1;
            result[n].line = &lines1[i - 1];
            n++; i--;
        }
    }

    for (int a = 0, b = n - 1; a < b; a++, b--) {
        diff_entry_t tmp = result[a];
        result[a] = result[b];
        result[b] = tmp;
    }

    for (int k = 0; k < r; k++) free(dp[k]);
    free(dp);

    *edits = result;
    *num_edits = n;
    return 0;
}

void print_unified(FILE *out, const diff_entry_t *edits, int num_edits,
                   const char *path1, const char *path2, int context) {
    fprintf(out, "--- %s\n", path1);
    fprintf(out, "+++ %s\n", path2);

    int idx = 0;
    while (idx < num_edits) {
        int change_start = -1;
        for (int k = idx; k < num_edits; k++) {
            if (edits[k].op != DIFF_EQUAL) {
                change_start = k;
                break;
            }
        }
        if (change_start == -1) break;

        int hunk_start = change_start - context;
        if (hunk_start < 0) hunk_start = 0;
        if (hunk_start < idx) hunk_start = idx;

        int hunk_end = change_start + 1;
        while (hunk_end < num_edits) {
            if (edits[hunk_end].op != DIFF_EQUAL) {
                hunk_end++;
                continue;
            }
            int eq_run = 0;
            int k = hunk_end;
            while (k < num_edits && edits[k].op == DIFF_EQUAL) { eq_run++; k++; }
            if (eq_run > context * 2) {
                hunk_end += context;
                break;
            }
            hunk_end = k;
        }
        if (hunk_end > num_edits) hunk_end = num_edits;

        int l1_start = 0, l1_count = 0, l2_start = 0, l2_count = 0;
        for (int k = hunk_start; k < hunk_end; k++) {
            if (edits[k].op == DIFF_EQUAL || edits[k].op == DIFF_DEL) {
                if (l1_count == 0) l1_start = edits[k].line_num1;
                l1_count++;
            }
            if (edits[k].op == DIFF_EQUAL || edits[k].op == DIFF_ADD) {
                if (l2_count == 0) l2_start = edits[k].line_num2;
                l2_count++;
            }
        }

        fprintf(out, "@@ -%d,%d +%d,%d @@\n", l1_start, l1_count, l2_start, l2_count);

        for (int k = hunk_start; k < hunk_end; k++) {
            const line_t *l = edits[k].line;
            switch (edits[k].op) {
                case DIFF_EQUAL:
                    fprintf(out, " %.*s\n", (int)l->len, l->start);
                    break;
                case DIFF_DEL:
                    fprintf(out, "-%.*s\n", (int)l->len, l->start);
                    break;
                case DIFF_ADD:
                    fprintf(out, "+%.*s\n", (int)l->len, l->start);
                    break;
            }
        }

        idx = hunk_end;
    }
}

void print_side_by_side(FILE *out, const diff_entry_t *edits, int num_edits) {
    int width = 40;
    fprintf(out, "%-*s | %s\n", width, "File 1", "File 2");
    for (int i = 0; i < width * 2 + 3; i++) fputc('-', out);
    fputc('\n', out);

    for (int i = 0; i < num_edits; i++) {
        const line_t *l = edits[i].line;
        int print_len = (int)l->len;
        if (print_len > width) print_len = width;

        switch (edits[i].op) {
            case DIFF_EQUAL:
                fprintf(out, "%-*.*s   %.*s\n", width, print_len, l->start, print_len, l->start);
                break;
            case DIFF_DEL:
                fprintf(out, "%-*.*s <\n", width, print_len, l->start);
                break;
            case DIFF_ADD:
                fprintf(out, "%-*s > %.*s\n", width, "", print_len, l->start);
                break;
        }
    }
}

int text_diff(const mapped_file_t *f1, const mapped_file_t *f2,
              int side_by_side, int context, FILE *out) {
    line_t *lines1 = NULL, *lines2 = NULL;
    int count1 = 0, count2 = 0;

    if (parse_lines(f1->data, f1->size, &lines1, &count1) < 0) {
        err_exit("failed to parse lines from %s", f1->path);
    }
    if (parse_lines(f2->data, f2->size, &lines2, &count2) < 0) {
        free(lines1);
        err_exit("failed to parse lines from %s", f2->path);
    }

    if ((long)count1 * (long)count2 > 100000000L) {
        warn_msg("warning: large files (%d x %d lines), comparison may be slow", count1, count2);
    }

    diff_entry_t *edits = NULL;
    int num_edits = 0;
    int rc = compute_diff(lines1, count1, lines2, count2, &edits, &num_edits);
    if (rc < 0) {
        free(lines1);
        free(lines2);
        return -1;
    }

    int differs = 0;
    for (int i = 0; i < num_edits; i++) {
        if (edits[i].op != DIFF_EQUAL) { differs = 1; break; }
    }

    if (differs) {
        if (side_by_side)
            print_side_by_side(out, edits, num_edits);
        else
            print_unified(out, edits, num_edits, f1->path, f2->path, context);
    }

    free(edits);
    free(lines1);
    free(lines2);
    return differs;
}
