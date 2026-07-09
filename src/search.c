/* SPDX-FileCopyrightText: AnmiTaliDev <anmitalidev@nuros.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "search.h"

#include <string.h>

#include "common.h"

static long find_substring(const char *hay, size_t hay_len, const char *needle, size_t needle_len,
                            size_t from)
{
    if (needle_len == 0 || hay_len < needle_len || from > hay_len - needle_len) {
        return -1;
    }
    for (size_t i = from; i + needle_len <= hay_len; i++) {
        if (memcmp(hay + i, needle, needle_len) == 0) {
            return (long)i;
        }
    }
    return -1;
}

bool search_find_forward(const buffer_t *buf, size_t start_row, size_t start_col,
                          const char *query, size_t *out_row, size_t *out_col)
{
    size_t query_len = strlen(query);
    if (query_len == 0) {
        return false;
    }

    for (size_t row = start_row; row < buf->num_lines; row++) {
        const line_t *line = &buf->lines[row];
        size_t from = (row == start_row) ? start_col : 0;
        if (from > line->len) {
            continue;
        }
        long pos = find_substring(line->chars, line->len, query, query_len, from);
        if (pos >= 0) {
            *out_row = row;
            *out_col = (size_t)pos;
            return true;
        }
    }

    for (size_t row = 0; row <= start_row; row++) {
        const line_t *line = &buf->lines[row];
        long pos = find_substring(line->chars, line->len, query, query_len, 0);
        if (pos >= 0) {
            *out_row = row;
            *out_col = (size_t)pos;
            return true;
        }
    }

    return false;
}

search_match_t *search_find_all(const buffer_t *buf, const char *query, size_t *out_count)
{
    size_t query_len = strlen(query);
    search_match_t *matches = NULL;
    size_t count = 0;
    size_t cap = 0;

    if (query_len > 0) {
        for (size_t row = 0; row < buf->num_lines; row++) {
            const line_t *line = &buf->lines[row];
            size_t from = 0;
            for (;;) {
                long pos = find_substring(line->chars, line->len, query, query_len, from);
                if (pos < 0) {
                    break;
                }
                if (count == cap) {
                    size_t new_cap = cap == 0 ? 16 : cap * 2;
                    matches = xrealloc(matches, new_cap * sizeof(search_match_t));
                    cap = new_cap;
                }
                matches[count].row = row;
                matches[count].col = (size_t)pos;
                count++;
                from = (size_t)pos + query_len;
            }
        }
    }

    *out_count = count;
    return matches;
}
