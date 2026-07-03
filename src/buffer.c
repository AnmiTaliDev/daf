/* SPDX-FileCopyrightText: AnmiTaliDev <anmitalidev@nuros.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "buffer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

static void line_ensure_cap(line_t *line, size_t needed)
{
    if (needed <= line->cap) {
        return;
    }
    size_t new_cap = line->cap == 0 ? 16 : line->cap;
    while (new_cap < needed) {
        new_cap *= 2;
    }
    line->chars = xrealloc(line->chars, new_cap);
    line->cap = new_cap;
}

static void buffer_ensure_line_cap(buffer_t *buf, size_t needed)
{
    if (needed <= buf->cap_lines) {
        return;
    }
    size_t new_cap = buf->cap_lines == 0 ? 16 : buf->cap_lines;
    while (new_cap < needed) {
        new_cap *= 2;
    }
    buf->lines = xrealloc(buf->lines, new_cap * sizeof(line_t));
    buf->cap_lines = new_cap;
}

static void buffer_append_line(buffer_t *buf, const char *data, size_t len)
{
    buffer_ensure_line_cap(buf, buf->num_lines + 1);
    line_t *line = &buf->lines[buf->num_lines];
    line->chars = NULL;
    line->cap = 0;
    line->len = 0;
    if (len > 0) {
        line_ensure_cap(line, len);
        memcpy(line->chars, data, len);
        line->len = len;
    }
    buf->num_lines += 1;
}

void buffer_init(buffer_t *buf)
{
    buf->lines = NULL;
    buf->num_lines = 0;
    buf->cap_lines = 0;
    buf->filename = NULL;
    buf->dirty = 0;
    buf->encoding = ENCODING_UTF8;
    buf->use_crlf = 0;
    buf->trailing_newline = 1;

    buffer_append_line(buf, "", 0);
}

void buffer_free(buffer_t *buf)
{
    for (size_t i = 0; i < buf->num_lines; i++) {
        free(buf->lines[i].chars);
    }
    free(buf->lines);
    free(buf->filename);
    buf->lines = NULL;
    buf->num_lines = 0;
    buf->cap_lines = 0;
    buf->filename = NULL;
}

line_t *buffer_line(buffer_t *buf, size_t row)
{
    return &buf->lines[row];
}

int buffer_load(buffer_t *buf, const char *filename)
{
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        return -1;
    }
    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return -1;
    }
    long size = ftell(fp);
    if (size < 0) {
        fclose(fp);
        return -1;
    }
    rewind(fp);

    unsigned char *data = xmalloc((size_t)size > 0 ? (size_t)size : 1);
    size_t total = fread(data, 1, (size_t)size, fp);
    fclose(fp);

    encoding_t encoding = encoding_detect(data, total);
    size_t offset = (encoding == ENCODING_UTF8_BOM) ? 3 : 0;

    for (size_t i = 0; i < buf->num_lines; i++) {
        free(buf->lines[i].chars);
    }
    buf->num_lines = 0;
    free(buf->filename);
    buf->filename = xstrdup(filename);
    buf->encoding = encoding;
    buf->use_crlf = 0;
    buf->trailing_newline = 1;
    buf->dirty = 0;

    size_t line_start = offset;
    int first_line = 1;

    for (size_t i = offset; i < total; i++) {
        if (data[i] != '\n') {
            continue;
        }
        size_t line_end = i;
        int crlf = 0;
        if (line_end > line_start && data[line_end - 1] == '\r') {
            line_end--;
            crlf = 1;
        }
        if (first_line) {
            buf->use_crlf = crlf;
            first_line = 0;
        }
        buffer_append_line(buf, (const char *)data + line_start, line_end - line_start);
        line_start = i + 1;
    }

    if (line_start < total) {
        buffer_append_line(buf, (const char *)data + line_start, total - line_start);
        buf->trailing_newline = 0;
    }

    if (buf->num_lines == 0) {
        buffer_append_line(buf, "", 0);
    }

    free(data);
    return 0;
}

int buffer_save(buffer_t *buf)
{
    if (buf->filename == NULL) {
        return -1;
    }
    return buffer_save_as(buf, buf->filename);
}

int buffer_save_as(buffer_t *buf, const char *filename)
{
    FILE *fp = fopen(filename, "wb");
    if (fp == NULL) {
        return -1;
    }

    if (buf->encoding == ENCODING_UTF8_BOM) {
        static const unsigned char bom[] = {0xEF, 0xBB, 0xBF};
        fwrite(bom, 1, sizeof(bom), fp);
    }

    const char *ending = buf->use_crlf ? "\r\n" : "\n";
    size_t ending_len = buf->use_crlf ? 2u : 1u;

    for (size_t i = 0; i < buf->num_lines; i++) {
        line_t *line = &buf->lines[i];
        fwrite(line->chars, 1, line->len, fp);
        if (i + 1 < buf->num_lines || buf->trailing_newline) {
            fwrite(ending, 1, ending_len, fp);
        }
    }

    int had_error = ferror(fp);
    fclose(fp);
    if (had_error) {
        return -1;
    }

    char *new_name = xstrdup(filename);
    free(buf->filename);
    buf->filename = new_name;
    buf->dirty = 0;
    return 0;
}

void buffer_insert_bytes(buffer_t *buf, size_t row, size_t col, const char *bytes, size_t n)
{
    line_t *line = &buf->lines[row];
    line_ensure_cap(line, line->len + n);
    memmove(line->chars + col + n, line->chars + col, line->len - col);
    memcpy(line->chars + col, bytes, n);
    line->len += n;
    buf->dirty = 1;
}

void buffer_delete_range(buffer_t *buf, size_t r1, size_t c1, size_t r2, size_t c2)
{
    if (r1 == r2) {
        line_t *line = &buf->lines[r1];
        size_t n = c2 - c1;
        memmove(line->chars + c1, line->chars + c1 + n, line->len - c1 - n);
        line->len -= n;
        buf->dirty = 1;
        return;
    }

    line_t *last = &buf->lines[r2];
    size_t first_keep = c1;
    size_t last_tail_len = last->len - c2;
    const char *tail_src = last->chars + c2;

    line_t *first = &buf->lines[r1];
    line_ensure_cap(first, first_keep + last_tail_len);
    memcpy(first->chars + first_keep, tail_src, last_tail_len);
    first->len = first_keep + last_tail_len;

    for (size_t i = r1 + 1; i <= r2; i++) {
        free(buf->lines[i].chars);
    }
    size_t remove_count = r2 - r1;
    memmove(&buf->lines[r1 + 1], &buf->lines[r2 + 1],
            (buf->num_lines - r2 - 1) * sizeof(line_t));
    buf->num_lines -= remove_count;
    buf->dirty = 1;
}

void buffer_split_line(buffer_t *buf, size_t row, size_t col)
{
    buffer_ensure_line_cap(buf, buf->num_lines + 1);

    line_t *line = &buf->lines[row];
    size_t tail_len = line->len - col;
    const char *tail_src = line->chars + col;

    memmove(&buf->lines[row + 2], &buf->lines[row + 1],
            (buf->num_lines - row - 1) * sizeof(line_t));

    line_t *new_line = &buf->lines[row + 1];
    new_line->chars = NULL;
    new_line->cap = 0;
    new_line->len = 0;
    line_ensure_cap(new_line, tail_len);
    memcpy(new_line->chars, tail_src, tail_len);
    new_line->len = tail_len;

    buf->lines[row].len = col;

    buf->num_lines += 1;
    buf->dirty = 1;
}

void buffer_join_lines(buffer_t *buf, size_t row)
{
    line_t *cur = &buf->lines[row];
    line_t *next = &buf->lines[row + 1];

    line_ensure_cap(cur, cur->len + next->len);
    memcpy(cur->chars + cur->len, next->chars, next->len);
    cur->len += next->len;
    free(next->chars);

    memmove(&buf->lines[row + 1], &buf->lines[row + 2],
            (buf->num_lines - row - 2) * sizeof(line_t));
    buf->num_lines -= 1;
    buf->dirty = 1;
}

char *buffer_extract_text(const buffer_t *buf, size_t r1, size_t c1, size_t r2, size_t c2,
                           size_t *out_len)
{
    if (r1 == r2) {
        const line_t *line = &buf->lines[r1];
        size_t len = c2 - c1;
        char *text = xmalloc(len > 0 ? len : 1);
        memcpy(text, line->chars + c1, len);
        *out_len = len;
        return text;
    }

    size_t total = (buf->lines[r1].len - c1) + 1;
    for (size_t row = r1 + 1; row < r2; row++) {
        total += buf->lines[row].len + 1;
    }
    total += c2;

    char *text = xmalloc(total > 0 ? total : 1);
    size_t pos = 0;

    memcpy(text + pos, buf->lines[r1].chars + c1, buf->lines[r1].len - c1);
    pos += buf->lines[r1].len - c1;
    text[pos++] = '\n';

    for (size_t row = r1 + 1; row < r2; row++) {
        memcpy(text + pos, buf->lines[row].chars, buf->lines[row].len);
        pos += buf->lines[row].len;
        text[pos++] = '\n';
    }

    memcpy(text + pos, buf->lines[r2].chars, c2);
    pos += c2;

    *out_len = pos;
    return text;
}

void buffer_insert_text(buffer_t *buf, size_t row, size_t col, const char *text, size_t len)
{
    size_t start = 0;
    for (size_t i = 0; i < len; i++) {
        if (text[i] != '\n') {
            continue;
        }
        buffer_insert_bytes(buf, row, col, text + start, i - start);
        col += i - start;
        buffer_split_line(buf, row, col);
        row += 1;
        col = 0;
        start = i + 1;
    }
    if (start < len) {
        buffer_insert_bytes(buf, row, col, text + start, len - start);
    }
}
