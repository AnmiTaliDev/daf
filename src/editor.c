/* SPDX-FileCopyrightText: AnmiTaliDev <anmitalidev@nuros.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "editor.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "filetype.h"

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

static size_t utf8_encode(unsigned int cp, char out[4])
{
    if (cp < 0x80) {
        out[0] = (char)cp;
        return 1;
    }
    if (cp < 0x800) {
        out[0] = (char)(0xC0 | (cp >> 6));
        out[1] = (char)(0x80 | (cp & 0x3F));
        return 2;
    }
    if (cp < 0x10000) {
        out[0] = (char)(0xE0 | (cp >> 12));
        out[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
        out[2] = (char)(0x80 | (cp & 0x3F));
        return 3;
    }
    out[0] = (char)(0xF0 | (cp >> 18));
    out[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
    out[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
    out[3] = (char)(0x80 | (cp & 0x3F));
    return 4;
}

static size_t prev_char_start(const line_t *line, size_t cx)
{
    if (cx == 0) {
        return 0;
    }
    size_t i = cx - 1;
    while (i > 0 && ((unsigned char)line->chars[i] & 0xC0) == 0x80) {
        i--;
    }
    return i;
}

static size_t next_char_start(const line_t *line, size_t cx)
{
    if (cx >= line->len) {
        return line->len;
    }
    size_t next = cx + utf8_char_len((unsigned char)line->chars[cx]);
    return next > line->len ? line->len : next;
}

static size_t first_non_blank(const line_t *line)
{
    size_t i = 0;
    while (i < line->len && (line->chars[i] == ' ' || line->chars[i] == '\t')) {
        i++;
    }
    return i;
}

size_t editor_cx_to_rx(const line_t *line, size_t cx)
{
    size_t rx = 0;
    for (size_t i = 0; i < cx && i < line->len; i++) {
        unsigned char c = (unsigned char)line->chars[i];
        if ((c & 0xC0) == 0x80) {
            continue;
        }
        if (c == '\t') {
            rx += DAF_TAB_STOP - (rx % DAF_TAB_STOP);
        } else {
            rx++;
        }
    }
    return rx;
}

size_t editor_rx_to_cx(const line_t *line, size_t target_rx)
{
    size_t rx = 0;
    size_t cx = 0;
    while (cx < line->len) {
        unsigned char c = (unsigned char)line->chars[cx];
        if ((c & 0xC0) != 0x80) {
            if (rx >= target_rx) {
                return cx;
            }
            if (c == '\t') {
                rx += DAF_TAB_STOP - (rx % DAF_TAB_STOP);
            } else {
                rx++;
            }
        }
        cx++;
    }
    return line->len;
}

void editor_init(editor_t *ed, int screen_rows, int screen_cols)
{
    memset(ed, 0, sizeof(*ed));
    buffer_init(&ed->buf);
    ed->screen_rows = screen_rows;
    ed->screen_cols = screen_cols;
    ed->mode = MODE_EDIT;
    ed->filetype = "Plain Text";
    editor_set_status(ed, "daf %s  |  Ctrl-S save  Ctrl-Q quit  Ctrl-F find  Ctrl-G go to line", DAF_VERSION);
}

void editor_free(editor_t *ed)
{
    buffer_free(&ed->buf);
}

void editor_open(editor_t *ed, const char *filename)
{
    if (buffer_load(&ed->buf, filename) == 0) {
        editor_set_status(ed, "Opened \"%s\"", filename);
    } else {
        free(ed->buf.filename);
        ed->buf.filename = xstrdup(filename);
        editor_set_status(ed, "New file \"%s\"", filename);
    }
    ed->filetype = filetype_detect(filename);
    ed->cy = 0;
    ed->cx = 0;
    ed->rowoff = 0;
    ed->coloff = 0;
    ed->sel.active = false;
}

void editor_update_screen_size(editor_t *ed, int screen_rows, int screen_cols)
{
    ed->screen_rows = screen_rows;
    ed->screen_cols = screen_cols;
}

void editor_set_status(editor_t *ed, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(ed->status_msg, sizeof(ed->status_msg), fmt, ap);
    va_end(ap);
}

int editor_gutter_width(const editor_t *ed)
{
    int digits = 1;
    size_t n = ed->buf.num_lines;
    while (n >= 10) {
        n /= 10;
        digits++;
    }
    int width = digits + 1;
    return width < 4 ? 4 : width;
}

int editor_text_cols(const editor_t *ed)
{
    int cols = ed->screen_cols - editor_gutter_width(ed);
    return cols < 1 ? 1 : cols;
}

void editor_scroll(editor_t *ed)
{
    line_t *line = &ed->buf.lines[ed->cy];
    size_t rx = editor_cx_to_rx(line, ed->cx);
    int text_cols = editor_text_cols(ed);

    if (ed->cy < ed->rowoff) {
        ed->rowoff = ed->cy;
    }
    if (ed->screen_rows > 0 && ed->cy >= ed->rowoff + (size_t)ed->screen_rows) {
        ed->rowoff = ed->cy - (size_t)ed->screen_rows + 1;
    }
    if (rx < ed->coloff) {
        ed->coloff = rx;
    }
    if (rx >= ed->coloff + (size_t)text_cols) {
        ed->coloff = rx - (size_t)text_cols + 1;
    }
}

bool editor_has_selection(const editor_t *ed)
{
    return ed->sel.active && !(ed->sel.anchor_row == ed->cy && ed->sel.anchor_col == ed->cx);
}

void editor_selection_range(const editor_t *ed, size_t *r1, size_t *c1, size_t *r2, size_t *c2)
{
    size_t ar = ed->sel.anchor_row;
    size_t ac = ed->sel.anchor_col;
    size_t cr = ed->cy;
    size_t cc = ed->cx;

    if (ar < cr || (ar == cr && ac <= cc)) {
        *r1 = ar;
        *c1 = ac;
        *r2 = cr;
        *c2 = cc;
    } else {
        *r1 = cr;
        *c1 = cc;
        *r2 = ar;
        *c2 = ac;
    }
}

void editor_clear_selection(editor_t *ed)
{
    ed->sel.active = false;
}

static void begin_or_extend_selection(editor_t *ed, bool extend)
{
    if (extend) {
        if (!ed->sel.active) {
            ed->sel.active = true;
            ed->sel.anchor_row = ed->cy;
            ed->sel.anchor_col = ed->cx;
        }
    } else {
        ed->sel.active = false;
    }
}

static void delete_selection(editor_t *ed)
{
    size_t r1, c1, r2, c2;
    editor_selection_range(ed, &r1, &c1, &r2, &c2);
    buffer_delete_range(&ed->buf, r1, c1, r2, c2);
    ed->cy = r1;
    ed->cx = c1;
    ed->sel.active = false;
}

void editor_move_left(editor_t *ed, bool extend)
{
    begin_or_extend_selection(ed, extend);
    line_t *line = &ed->buf.lines[ed->cy];
    if (ed->cx > 0) {
        ed->cx = prev_char_start(line, ed->cx);
    } else if (ed->cy > 0) {
        ed->cy--;
        ed->cx = ed->buf.lines[ed->cy].len;
    }
    ed->goal_rx = editor_cx_to_rx(&ed->buf.lines[ed->cy], ed->cx);
}

void editor_move_right(editor_t *ed, bool extend)
{
    begin_or_extend_selection(ed, extend);
    line_t *line = &ed->buf.lines[ed->cy];
    if (ed->cx < line->len) {
        ed->cx = next_char_start(line, ed->cx);
    } else if (ed->cy + 1 < ed->buf.num_lines) {
        ed->cy++;
        ed->cx = 0;
    }
    ed->goal_rx = editor_cx_to_rx(&ed->buf.lines[ed->cy], ed->cx);
}

void editor_move_up(editor_t *ed, bool extend)
{
    begin_or_extend_selection(ed, extend);
    if (ed->cy > 0) {
        ed->cy--;
        ed->cx = editor_rx_to_cx(&ed->buf.lines[ed->cy], ed->goal_rx);
    } else {
        ed->cx = 0;
    }
}

void editor_move_down(editor_t *ed, bool extend)
{
    begin_or_extend_selection(ed, extend);
    if (ed->cy + 1 < ed->buf.num_lines) {
        ed->cy++;
        ed->cx = editor_rx_to_cx(&ed->buf.lines[ed->cy], ed->goal_rx);
    } else {
        ed->cx = ed->buf.lines[ed->cy].len;
    }
}

void editor_move_home(editor_t *ed, bool extend)
{
    begin_or_extend_selection(ed, extend);
    line_t *line = &ed->buf.lines[ed->cy];
    size_t fnb = first_non_blank(line);
    ed->cx = (ed->cx == fnb) ? 0 : fnb;
    ed->goal_rx = editor_cx_to_rx(line, ed->cx);
}

void editor_move_end(editor_t *ed, bool extend)
{
    begin_or_extend_selection(ed, extend);
    ed->cx = ed->buf.lines[ed->cy].len;
    ed->goal_rx = editor_cx_to_rx(&ed->buf.lines[ed->cy], ed->cx);
}

void editor_move_page(editor_t *ed, bool up, bool extend)
{
    begin_or_extend_selection(ed, extend);
    size_t amount = (size_t)DAF_MAX(ed->screen_rows, 1);
    if (up) {
        ed->cy = amount > ed->cy ? 0 : ed->cy - amount;
    } else {
        size_t last = ed->buf.num_lines - 1;
        ed->cy = DAF_MIN(ed->cy + amount, last);
    }
    ed->cx = editor_rx_to_cx(&ed->buf.lines[ed->cy], ed->goal_rx);
}

void editor_goto_line(editor_t *ed, long line_number)
{
    if (line_number < 1) {
        line_number = 1;
    }
    size_t target = (size_t)(line_number - 1);
    if (target >= ed->buf.num_lines) {
        target = ed->buf.num_lines - 1;
    }
    ed->cy = target;
    ed->cx = first_non_blank(&ed->buf.lines[ed->cy]);
    ed->goal_rx = editor_cx_to_rx(&ed->buf.lines[ed->cy], ed->cx);
    ed->sel.active = false;
}

void editor_insert_codepoint(editor_t *ed, unsigned int codepoint)
{
    if (editor_has_selection(ed)) {
        delete_selection(ed);
    }
    char bytes[4];
    size_t n = utf8_encode(codepoint, bytes);
    buffer_insert_bytes(&ed->buf, ed->cy, ed->cx, bytes, n);
    ed->cx += n;
    ed->goal_rx = editor_cx_to_rx(&ed->buf.lines[ed->cy], ed->cx);
}

void editor_insert_newline(editor_t *ed)
{
    if (editor_has_selection(ed)) {
        delete_selection(ed);
    }
    buffer_split_line(&ed->buf, ed->cy, ed->cx);
    ed->cy += 1;
    ed->cx = 0;
    ed->goal_rx = 0;
}

void editor_backspace(editor_t *ed)
{
    if (editor_has_selection(ed)) {
        delete_selection(ed);
        return;
    }
    if (ed->cx > 0) {
        line_t *line = &ed->buf.lines[ed->cy];
        size_t start = prev_char_start(line, ed->cx);
        buffer_delete_range(&ed->buf, ed->cy, start, ed->cy, ed->cx);
        ed->cx = start;
    } else if (ed->cy > 0) {
        size_t prev_len = ed->buf.lines[ed->cy - 1].len;
        buffer_join_lines(&ed->buf, ed->cy - 1);
        ed->cy -= 1;
        ed->cx = prev_len;
    }
    ed->goal_rx = editor_cx_to_rx(&ed->buf.lines[ed->cy], ed->cx);
}

void editor_delete_forward(editor_t *ed)
{
    if (editor_has_selection(ed)) {
        delete_selection(ed);
        return;
    }
    line_t *line = &ed->buf.lines[ed->cy];
    if (ed->cx < line->len) {
        size_t end = next_char_start(line, ed->cx);
        buffer_delete_range(&ed->buf, ed->cy, ed->cx, ed->cy, end);
    } else if (ed->cy + 1 < ed->buf.num_lines) {
        buffer_join_lines(&ed->buf, ed->cy);
    }
}

static void indent_line(buffer_t *buf, size_t row)
{
    buffer_insert_bytes(buf, row, 0, "\t", 1);
}

static size_t dedent_line(buffer_t *buf, size_t row)
{
    line_t *line = &buf->lines[row];
    if (line->len == 0) {
        return 0;
    }
    if (line->chars[0] == '\t') {
        buffer_delete_range(buf, row, 0, row, 1);
        return 1;
    }
    size_t n = 0;
    while (n < line->len && n < DAF_TAB_STOP && line->chars[n] == ' ') {
        n++;
    }
    if (n > 0) {
        buffer_delete_range(buf, row, 0, row, n);
    }
    return n;
}

static void shift_column_on_row(size_t *col, size_t row, size_t target_row, long delta,
                                 size_t new_len)
{
    if (row != target_row) {
        return;
    }
    long shifted = (long)*col + delta;
    *col = shifted < 0 ? 0 : (size_t)shifted;
    *col = DAF_MIN(*col, new_len);
}

void editor_handle_tab(editor_t *ed, bool shift)
{
    if (editor_has_selection(ed)) {
        size_t r1, c1, r2, c2;
        editor_selection_range(ed, &r1, &c1, &r2, &c2);
        (void)c1;
        (void)c2;
        for (size_t row = r1; row <= r2; row++) {
            long delta;
            if (shift) {
                delta = -(long)dedent_line(&ed->buf, row);
            } else {
                indent_line(&ed->buf, row);
                delta = 1;
            }
            size_t new_len = ed->buf.lines[row].len;
            shift_column_on_row(&ed->cx, ed->cy, row, delta, new_len);
            shift_column_on_row(&ed->sel.anchor_col, ed->sel.anchor_row, row, delta, new_len);
        }
    } else if (!shift) {
        editor_insert_codepoint(ed, '\t');
    }
}

static bool editor_screen_to_buffer_pos(const editor_t *ed, int screen_row, int screen_col,
                                         size_t *out_row, size_t *out_col)
{
    if (screen_row < 0 || screen_row >= ed->screen_rows || ed->buf.num_lines == 0) {
        return false;
    }

    size_t file_row = ed->rowoff + (size_t)screen_row;
    if (file_row >= ed->buf.num_lines) {
        file_row = ed->buf.num_lines - 1;
    }

    int gutter = editor_gutter_width(ed);
    int text_col = screen_col - gutter;
    if (text_col < 0) {
        text_col = 0;
    }
    size_t rx = ed->coloff + (size_t)text_col;

    *out_row = file_row;
    *out_col = editor_rx_to_cx(&ed->buf.lines[file_row], rx);
    return true;
}

void editor_mouse_press(editor_t *ed, int screen_row, int screen_col)
{
    size_t row, col;
    if (!editor_screen_to_buffer_pos(ed, screen_row, screen_col, &row, &col)) {
        return;
    }
    ed->cy = row;
    ed->cx = col;
    ed->sel.active = true;
    ed->sel.anchor_row = row;
    ed->sel.anchor_col = col;
    ed->goal_rx = editor_cx_to_rx(&ed->buf.lines[ed->cy], ed->cx);
}

void editor_mouse_drag(editor_t *ed, int screen_row, int screen_col)
{
    size_t row, col;
    if (!editor_screen_to_buffer_pos(ed, screen_row, screen_col, &row, &col)) {
        return;
    }
    if (!ed->sel.active) {
        ed->sel.active = true;
        ed->sel.anchor_row = ed->cy;
        ed->sel.anchor_col = ed->cx;
    }
    ed->cy = row;
    ed->cx = col;
    ed->goal_rx = editor_cx_to_rx(&ed->buf.lines[ed->cy], ed->cx);
}

void editor_mouse_release(editor_t *ed)
{
    if (ed->sel.active && ed->sel.anchor_row == ed->cy && ed->sel.anchor_col == ed->cx) {
        ed->sel.active = false;
    }
}

void editor_scroll_view(editor_t *ed, int delta_lines)
{
    long max_off = (long)ed->buf.num_lines - (long)DAF_MAX(ed->screen_rows, 1);
    if (max_off < 0) {
        max_off = 0;
    }
    long new_off = (long)ed->rowoff + delta_lines;
    if (new_off < 0) {
        new_off = 0;
    }
    if (new_off > max_off) {
        new_off = max_off;
    }
    ed->rowoff = (size_t)new_off;
}
