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

static void test_undo_redo_typing(void)
{
    editor_t ed;
    editor_init(&ed, 24, 80);

    editor_insert_codepoint(&ed, 'a');
    editor_insert_codepoint(&ed, 'b');
    editor_insert_codepoint(&ed, 'c');
    assert(ed.buf.lines[0].len == 3);
    assert(memcmp(ed.buf.lines[0].chars, "abc", 3) == 0);

    editor_undo(&ed);
    assert(ed.buf.lines[0].len == 2);
    assert(memcmp(ed.buf.lines[0].chars, "ab", 2) == 0);
    assert(ed.cx == 2);

    editor_undo(&ed);
    editor_undo(&ed);
    assert(ed.buf.lines[0].len == 0);

    editor_undo(&ed);
    assert(ed.buf.lines[0].len == 0);

    editor_redo(&ed);
    editor_redo(&ed);
    editor_redo(&ed);
    assert(ed.buf.lines[0].len == 3);
    assert(memcmp(ed.buf.lines[0].chars, "abc", 3) == 0);
    assert(ed.cx == 3);

    editor_free(&ed);
}

static void test_new_edit_clears_redo(void)
{
    editor_t ed;
    editor_init(&ed, 24, 80);

    editor_insert_codepoint(&ed, 'a');
    editor_insert_codepoint(&ed, 'b');
    editor_undo(&ed);
    assert(ed.redo_count == 1);

    editor_insert_codepoint(&ed, 'x');
    assert(ed.redo_count == 0);
    assert(memcmp(ed.buf.lines[0].chars, "ax", 2) == 0);

    editor_free(&ed);
}

static void test_undo_backspace_and_newline(void)
{
    editor_t ed;
    editor_init(&ed, 24, 80);
    set_line(&ed, 0, "hello");
    ed.cx = 5;

    editor_backspace(&ed);
    assert(ed.buf.lines[0].len == 4);
    editor_undo(&ed);
    assert(ed.buf.lines[0].len == 5);
    assert(memcmp(ed.buf.lines[0].chars, "hello", 5) == 0);
    assert(ed.cx == 5);

    editor_insert_newline(&ed);
    assert(ed.buf.num_lines == 2);
    editor_undo(&ed);
    assert(ed.buf.num_lines == 1);
    assert(ed.cx == 5);

    editor_free(&ed);
}

static void test_undo_tab_indent_selection(void)
{
    editor_t ed;
    editor_init(&ed, 24, 80);
    set_line(&ed, 0, "int x;");

    ed.cy = 0;
    ed.cx = 0;
    editor_move_right(&ed, true);
    editor_move_right(&ed, true);
    editor_move_right(&ed, true);

    editor_handle_tab(&ed, false);
    assert(ed.buf.lines[0].len == 7);
    assert(ed.buf.lines[0].chars[0] == '\t');

    editor_undo(&ed);
    assert(ed.buf.lines[0].len == 6);
    assert(memcmp(ed.buf.lines[0].chars, "int x;", 6) == 0);

    editor_free(&ed);
}

int main(void)
{
    test_undo_redo_typing();
    test_new_edit_clears_redo();
    test_undo_backspace_and_newline();
    test_undo_tab_indent_selection();
    return 0;
}
