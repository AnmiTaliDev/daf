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

static void test_replace_all_same_length(void)
{
    editor_t ed;
    editor_init(&ed, 24, 80);
    set_line(&ed, 0, "foo bar foo");
    buffer_split_line(&ed.buf, 0, 11);
    set_line(&ed, 1, "baz foo");

    size_t count = editor_replace_all(&ed, "foo", "qux");
    assert(count == 3);
    assert(memcmp(ed.buf.lines[0].chars, "qux bar qux", 11) == 0);
    assert(memcmp(ed.buf.lines[1].chars, "baz qux", 7) == 0);
    assert(ed.buf.dirty);

    editor_free(&ed);
}

static void test_replace_all_different_length_is_undoable(void)
{
    editor_t ed;
    editor_init(&ed, 24, 80);
    set_line(&ed, 0, "a cat and a cat");

    size_t count = editor_replace_all(&ed, "cat", "elephant");
    assert(count == 2);
    assert(ed.buf.lines[0].len == 25);
    assert(memcmp(ed.buf.lines[0].chars, "a elephant and a elephant", 25) == 0);

    editor_undo(&ed);
    assert(ed.buf.lines[0].len == 15);
    assert(memcmp(ed.buf.lines[0].chars, "a cat and a cat", 15) == 0);

    editor_redo(&ed);
    assert(ed.buf.lines[0].len == 25);
    assert(memcmp(ed.buf.lines[0].chars, "a elephant and a elephant", 25) == 0);

    editor_free(&ed);
}

static void test_replace_all_no_matches(void)
{
    editor_t ed;
    editor_init(&ed, 24, 80);
    set_line(&ed, 0, "hello world");

    size_t count = editor_replace_all(&ed, "missing", "x");
    assert(count == 0);
    assert(ed.buf.lines[0].len == 11);
    assert(memcmp(ed.buf.lines[0].chars, "hello world", 11) == 0);

    editor_free(&ed);
}

static void test_replace_all_multiline_replacement(void)
{
    editor_t ed;
    editor_init(&ed, 24, 80);
    set_line(&ed, 0, "one,two");

    size_t count = editor_replace_all(&ed, ",", "\n");
    assert(count == 1);
    assert(ed.buf.num_lines == 2);
    assert(memcmp(ed.buf.lines[0].chars, "one", 3) == 0);
    assert(memcmp(ed.buf.lines[1].chars, "two", 3) == 0);

    editor_free(&ed);
}

int main(void)
{
    test_replace_all_same_length();
    test_replace_all_different_length_is_undoable();
    test_replace_all_no_matches();
    test_replace_all_multiline_replacement();
    return 0;
}
