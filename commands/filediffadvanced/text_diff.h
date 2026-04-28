#ifndef TEXT_DIFF_H
#define TEXT_DIFF_H

#include <stddef.h>
#include <stdio.h>
#include "mmap_io.h"

typedef struct {
    const char *start;
    size_t len;
    unsigned long hash;
} line_t;

typedef enum { DIFF_EQUAL, DIFF_ADD, DIFF_DEL } diff_op_t;

typedef struct {
    diff_op_t op;
    int line_num1;
    int line_num2;
    const line_t *line;
} diff_entry_t;

int parse_lines(const unsigned char *data, size_t size, line_t **lines, int *count);
int compute_diff(const line_t *lines1, int count1, const line_t *lines2, int count2,
                 diff_entry_t **edits, int *num_edits);
void print_unified(FILE *out, const diff_entry_t *edits, int num_edits,
                   const char *path1, const char *path2, int context);
void print_side_by_side(FILE *out, const diff_entry_t *edits, int num_edits);
int text_diff(const mapped_file_t *f1, const mapped_file_t *f2,
              int side_by_side, int context, FILE *out);

#endif
