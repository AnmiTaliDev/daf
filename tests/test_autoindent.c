/* SPDX-FileCopyrightText: AnmiTaliDev <anmitalidev@nuros.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <assert.h>
#include <string.h>

#include "editor.h"

static void set_line(editor_t *ed, size_t row, const char *text)
{
    buffer_insert_bytes(&ed->buf, row, 0, text, strlen(text));
}

static void test_indent_carried_from_end_of_line(void)
{
    editor_t ed;
    editor_init(&ed, 24, 80);
    set_line(&ed, 0, "    foo();");
    ed.cy = 0;
    ed.cx = ed.buf.lines[0].len;

    editor_insert_newline(&ed);

    assert(ed.buf.num_lines == 2);
    assert(memcmp(ed.buf.lines[0].chars, "    foo();", 10) == 0);
    assert(ed.buf.lines[1].len == 4);
    assert(memcmp(ed.buf.lines[1].chars, "    ", 4) == 0);
    assert(ed.cy == 1 && ed.cx == 4);

    editor_undo(&ed);
    assert(ed.buf.num_lines == 1);
    assert(ed.buf.lines[0].len == 10);

    editor_free(&ed);
}

static void test_indent_carried_mid_line(void)
{
    editor_t ed;
    editor_init(&ed, 24, 80);
    set_line(&ed, 0, "  if (x) {y}");
    ed.cy = 0;
    ed.cx = 9; /* right after "  if (x) " (before '{') */

    editor_insert_newline(&ed);

    assert(ed.buf.num_lines == 2);
    assert(memcmp(ed.buf.lines[0].chars, "  if (x) ", 9) == 0);
    assert(memcmp(ed.buf.lines[1].chars, "  {y}", 5) == 0);
    assert(ed.cx == 2);

    editor_free(&ed);
}

static void test_no_indent_duplicated_when_splitting_inside_whitespace(void)
{
    editor_t ed;
    editor_init(&ed, 24, 80);
    set_line(&ed, 0, "    x");
    ed.cy = 0;
    ed.cx = 2; /* inside the leading whitespace */

    editor_insert_newline(&ed);

    assert(ed.buf.num_lines == 2);
    assert(memcmp(ed.buf.lines[0].chars, "  ", 2) == 0);
    assert(ed.buf.lines[1].len == 3);
    assert(memcmp(ed.buf.lines[1].chars, "  x", 3) == 0);

    editor_free(&ed);
}

static void test_no_indent_on_flush_left_line(void)
{
    editor_t ed;
    editor_init(&ed, 24, 80);
    set_line(&ed, 0, "hello");
    ed.cy = 0;
    ed.cx = 5;

    editor_insert_newline(&ed);

    assert(ed.buf.num_lines == 2);
    assert(ed.buf.lines[1].len == 0);
    assert(ed.cx == 0);

    editor_free(&ed);
}

int main(void)
{
    test_indent_carried_from_end_of_line();
    test_indent_carried_mid_line();
    test_no_indent_duplicated_when_splitting_inside_whitespace();
    test_no_indent_on_flush_left_line();
    return 0;
}
