/* SPDX-FileCopyrightText: AnmiTaliDev <anmitalidev@nuros.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "render.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "encoding.h"

typedef struct {
    char *data;
    size_t len;
    size_t cap;
} abuf_t;

static void abuf_append(abuf_t *ab, const char *s, size_t len)
{
    if (ab->len + len > ab->cap) {
        size_t new_cap = ab->cap == 0 ? 256 : ab->cap;
        while (new_cap < ab->len + len) {
            new_cap *= 2;
        }
        ab->data = xrealloc(ab->data, new_cap);
        ab->cap = new_cap;
    }
    memcpy(ab->data + ab->len, s, len);
    ab->len += len;
}

static void abuf_append_str(abuf_t *ab, const char *s)
{
    abuf_append(ab, s, strlen(s));
}

static void abuf_free(abuf_t *ab)
{
    free(ab->data);
}

static size_t utf8_char_len(unsigned char lead)
{
    if ((lead & 0x80) == 0x00) {
        return 1;
    }
    if ((lead & 0xE0) == 0xC0) {
        return 2;
    }
    if ((lead & 0xF0) == 0xE0) {
        return 3;
    }
    if ((lead & 0xF8) == 0xF0) {
        return 4;
    }
    return 1;
}

static const char *prompt_label(editor_mode_t mode)
{
    switch (mode) {
    case MODE_PROMPT_FIND:
        return "Find: ";
    case MODE_PROMPT_GOTO:
        return "Go to line: ";
    case MODE_PROMPT_SAVE_AS:
        return "Save as: ";
    case MODE_EDIT:
    default:
        return "";
    }
}

static void render_gutter(abuf_t *ab, const editor_t *ed, size_t file_row, bool is_current)
{
    int width = editor_gutter_width(ed);
    char digits[32];
    int digit_len = snprintf(digits, sizeof(digits), "%zu", file_row + 1);
    if (digit_len < 0) {
        digit_len = 0;
    }

    int pad = width - 1 - digit_len;
    for (int i = 0; i < pad; i++) {
        abuf_append(ab, " ", 1);
    }

    if (is_current) {
        abuf_append_str(ab, "\x1b[7m");
    }
    abuf_append(ab, digits, (size_t)digit_len);
    if (is_current) {
        abuf_append_str(ab, "\x1b[27m");
    }

    abuf_append(ab, " ", 1);
}

static void render_empty_gutter(abuf_t *ab, const editor_t *ed)
{
    int width = editor_gutter_width(ed);
    for (int i = 0; i < width; i++) {
        abuf_append(ab, " ", 1);
    }
    abuf_append_str(ab, "~");
}

static bool byte_in_search_match(const editor_t *ed, size_t row, size_t byte_col)
{
    for (size_t i = 0; i < ed->search_match_count; i++) {
        const search_match_t *m = &ed->search_matches[i];
        if (m->row == row && byte_col >= m->col && byte_col < m->col + ed->prompt_len) {
            return true;
        }
    }
    return false;
}

static void render_text_row(abuf_t *ab, const editor_t *ed, size_t file_row)
{
    const line_t *line = &ed->buf.lines[file_row];
    int text_cols = editor_text_cols(ed);
    size_t coloff = ed->coloff;

    bool row_has_hl = false;
    size_t hl_start = 0;
    size_t hl_end = 0;
    if (editor_has_selection(ed)) {
        size_t r1, c1, r2, c2;
        editor_selection_range(ed, &r1, &c1, &r2, &c2);
        if (file_row >= r1 && file_row <= r2) {
            row_has_hl = true;
            hl_start = (file_row == r1) ? c1 : 0;
            hl_end = (file_row == r2) ? c2 : line->len;
        }
    }
    bool show_matches = ed->mode == MODE_PROMPT_FIND && ed->search_match_count > 0;

    size_t i = 0;
    size_t rx = 0;
    int emitted = 0;
    bool hl_active = false;
    bool match_active = false;

    while (i < line->len && emitted < text_cols) {
        unsigned char c = (unsigned char)line->chars[i];
        bool should_hl = row_has_hl && i >= hl_start && i < hl_end;
        bool should_match = show_matches && byte_in_search_match(ed, file_row, i);
        if (should_hl != hl_active) {
            abuf_append_str(ab, should_hl ? "\x1b[7m" : "\x1b[27m");
            hl_active = should_hl;
        }
        if (should_match != match_active) {
            abuf_append_str(ab, should_match ? "\x1b[4m" : "\x1b[24m");
            match_active = should_match;
        }

        if (c == '\t') {
            size_t width = DAF_TAB_STOP - (rx % DAF_TAB_STOP);
            for (size_t s = 0; s < width; s++) {
                if (rx >= coloff && emitted < text_cols) {
                    abuf_append(ab, " ", 1);
                    emitted++;
                }
                rx++;
            }
            i++;
        } else {
            size_t clen = utf8_char_len(c);
            if (i + clen > line->len) {
                clen = 1;
            }
            if (rx >= coloff && emitted < text_cols) {
                abuf_append(ab, line->chars + i, clen);
                emitted++;
            }
            rx++;
            i += clen;
        }
    }

    if (hl_active) {
        abuf_append_str(ab, "\x1b[27m");
    }
    if (match_active) {
        abuf_append_str(ab, "\x1b[24m");
    }
}

static void render_status_bar(abuf_t *ab, const editor_t *ed)
{
    abuf_append_str(ab, "\x1b[7m");

    char left[256];
    const char *name = ed->buf.filename != NULL ? ed->buf.filename : "[No Name]";
    int left_len = snprintf(left, sizeof(left), " %s%s", name, ed->buf.dirty ? " [+]" : "");
    if (left_len < 0) {
        left_len = 0;
    }
    if (left_len > ed->screen_cols) {
        left_len = ed->screen_cols;
    }

    size_t rx = editor_cx_to_rx(&ed->buf.lines[ed->cy], ed->cx);
    char right[128];
    int right_len = snprintf(right, sizeof(right), "%s | %s | Ln %zu, Col %zu ", ed->filetype,
                              encoding_name(ed->buf.encoding), ed->cy + 1, rx + 1);
    if (right_len < 0) {
        right_len = 0;
    }

    abuf_append(ab, left, (size_t)left_len);

    int padding = ed->screen_cols - left_len - right_len;
    while (padding > 0) {
        abuf_append(ab, " ", 1);
        padding--;
    }
    if (right_len > 0 && ed->screen_cols - left_len >= right_len) {
        abuf_append(ab, right, (size_t)right_len);
    }

    abuf_append_str(ab, "\x1b[0m");
    abuf_append_str(ab, "\r\n");
}

static void render_message_bar(abuf_t *ab, const editor_t *ed)
{
    abuf_append_str(ab, "\x1b[K");
    if (ed->mode != MODE_EDIT) {
        abuf_append_str(ab, prompt_label(ed->mode));
        abuf_append(ab, ed->prompt_input, ed->prompt_len);
        if (ed->mode == MODE_PROMPT_FIND && ed->prompt_len > 0) {
            char count_buf[32];
            int n = snprintf(count_buf, sizeof(count_buf), "  (%zu found)", ed->search_match_count);
            if (n > 0) {
                abuf_append(ab, count_buf, (size_t)n);
            }
        }
    } else {
        size_t len = strlen(ed->status_msg);
        size_t max_len = ed->screen_cols > 0 ? (size_t)ed->screen_cols : 0;
        if (len > max_len) {
            len = max_len;
        }
        abuf_append(ab, ed->status_msg, len);
    }
}

void render_screen(editor_t *ed)
{
    editor_scroll(ed);

    abuf_t ab = {0};
    abuf_append_str(&ab, "\x1b[?25l");
    abuf_append_str(&ab, "\x1b[H");

    for (int row = 0; row < ed->screen_rows; row++) {
        size_t file_row = ed->rowoff + (size_t)row;
        if (file_row < ed->buf.num_lines) {
            render_gutter(&ab, ed, file_row, file_row == ed->cy);
            render_text_row(&ab, ed, file_row);
        } else {
            render_empty_gutter(&ab, ed);
        }
        abuf_append_str(&ab, "\x1b[K");
        abuf_append_str(&ab, "\r\n");
    }

    render_status_bar(&ab, ed);
    render_message_bar(&ab, ed);

    int cursor_row;
    int cursor_col;
    if (ed->mode != MODE_EDIT) {
        cursor_row = ed->screen_rows + 2;
        size_t label_len = strlen(prompt_label(ed->mode));
        cursor_col = (int)(label_len + ed->prompt_len) + 1;
    } else {
        size_t rx = editor_cx_to_rx(&ed->buf.lines[ed->cy], ed->cx);
        cursor_row = (int)(ed->cy - ed->rowoff) + 1;
        cursor_col = editor_gutter_width(ed) + (int)(rx - ed->coloff) + 1;
    }

    char pos_buf[32];
    int n = snprintf(pos_buf, sizeof(pos_buf), "\x1b[%d;%dH", cursor_row, cursor_col);
    if (n > 0) {
        abuf_append(&ab, pos_buf, (size_t)n);
    }

    abuf_append_str(&ab, "\x1b[?25h");

    ssize_t written = write(STDOUT_FILENO, ab.data, ab.len);
    (void)written;
    abuf_free(&ab);
}
