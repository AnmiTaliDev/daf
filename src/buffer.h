/* SPDX-FileCopyrightText: AnmiTaliDev <anmitalidev@nuros.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef DAF_BUFFER_H
#define DAF_BUFFER_H

#include <stddef.h>

#include "encoding.h"

typedef struct {
    char *chars;
    size_t len;
    size_t cap;
} line_t;

typedef struct {
    line_t *lines;
    size_t num_lines;
    size_t cap_lines;
    char *filename;
    int dirty;
    encoding_t encoding;
    int use_crlf;
    int trailing_newline;
} buffer_t;

void buffer_init(buffer_t *buf);
void buffer_free(buffer_t *buf);

int buffer_load(buffer_t *buf, const char *filename);
int buffer_save(buffer_t *buf);
int buffer_save_as(buffer_t *buf, const char *filename);

line_t *buffer_line(buffer_t *buf, size_t row);

void buffer_insert_bytes(buffer_t *buf, size_t row, size_t col, const char *bytes, size_t n);
void buffer_delete_range(buffer_t *buf, size_t r1, size_t c1, size_t r2, size_t c2);
void buffer_split_line(buffer_t *buf, size_t row, size_t col);
void buffer_join_lines(buffer_t *buf, size_t row);

char *buffer_extract_text(const buffer_t *buf, size_t r1, size_t c1, size_t r2, size_t c2,
                           size_t *out_len);

void buffer_insert_text(buffer_t *buf, size_t row, size_t col, const char *text, size_t len);

#endif /* DAF_BUFFER_H */
