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

static void select_range(editor_t *ed, size_t row, size_t from, size_t to)
{
    ed->cy = row;
    ed->cx = from;
    ed->sel.active = true;
    ed->sel.anchor_row = row;
    ed->sel.anchor_col = from;
    ed->cx = to;
}

static void test_copy_paste(void)
{
    editor_t ed;
    editor_init(&ed, 24, 80);
    set_line(&ed, 0, "hello world");

    select_range(&ed, 0, 0, 5); /* "hello" */
    editor_copy(&ed);
    assert(ed.clipboard_len == 5);
    assert(memcmp(ed.clipboard, "hello", 5) == 0);
    assert(ed.buf.lines[0].len == 11);

    ed.cy = 0;
    ed.cx = 11;
    ed.sel.active = false;
    editor_insert_codepoint(&ed, ' ');
    editor_paste(&ed);
    assert(ed.buf.lines[0].len == 17);
    assert(memcmp(ed.buf.lines[0].chars, "hello world hello", 17) == 0);
    assert(ed.cx == 17);

    editor_free(&ed);
}

static void test_cut_is_undoable(void)
{
    editor_t ed;
    editor_init(&ed, 24, 80);
    set_line(&ed, 0, "hello world");

    select_range(&ed, 0, 0, 6); /* "hello " */
    editor_cut(&ed);
    assert(ed.buf.lines[0].len == 5);
    assert(memcmp(ed.buf.lines[0].chars, "world", 5) == 0);
    assert(ed.clipboard_len == 6);
    assert(memcmp(ed.clipboard, "hello ", 6) == 0);

    editor_undo(&ed);
    assert(ed.buf.lines[0].len == 11);
    assert(memcmp(ed.buf.lines[0].chars, "hello world", 11) == 0);

    editor_free(&ed);
}

static void test_paste_replaces_selection(void)
{
    editor_t ed;
    editor_init(&ed, 24, 80);
    set_line(&ed, 0, "hello world");

    select_range(&ed, 0, 0, 5);
    editor_copy(&ed);

    select_range(&ed, 0, 6, 11); /* "world" */
    editor_paste(&ed);
    assert(memcmp(ed.buf.lines[0].chars, "hello hello", 11) == 0);

    editor_undo(&ed);
    assert(memcmp(ed.buf.lines[0].chars, "hello world", 11) == 0);

    editor_free(&ed);
}

static void test_paste_multiline(void)
{
    editor_t ed;
    editor_init(&ed, 24, 80);
    set_line(&ed, 0, "ab");
    buffer_split_line(&ed.buf, 0, 2);
    set_line(&ed, 1, "cd");

    ed.sel.active = true;
    ed.sel.anchor_row = 0;
    ed.sel.anchor_col = 0;
    ed.cy = 1;
    ed.cx = 2;
    editor_copy(&ed);
    assert(ed.clipboard_len == 5);
    assert(memcmp(ed.clipboard, "ab\ncd", 5) == 0);

    ed.sel.active = false;
    ed.cy = 1;
    ed.cx = 2;
    editor_paste(&ed);
    assert(ed.buf.num_lines == 3);
    assert(memcmp(ed.buf.lines[1].chars, "cdab", 4) == 0);
    assert(memcmp(ed.buf.lines[2].chars, "cd", 2) == 0);

    editor_free(&ed);
}

static void test_copy_without_selection_is_noop(void)
{
    editor_t ed;
    editor_init(&ed, 24, 80);
    set_line(&ed, 0, "text");

    editor_copy(&ed);
    assert(ed.clipboard == NULL);

    editor_paste(&ed);
    assert(ed.buf.lines[0].len == 4);

    editor_free(&ed);
}

int main(void)
{
    test_copy_paste();
    test_cut_is_undoable();
    test_paste_replaces_selection();
    test_paste_multiline();
    test_copy_without_selection_is_noop();
    return 0;
}
