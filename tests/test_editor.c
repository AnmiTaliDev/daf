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

static void test_smart_home(void)
{
    editor_t ed;
    editor_init(&ed, 24, 80);
    set_line(&ed, 0, "    int x;");

    ed.cx = 10;
    editor_move_home(&ed, false);
    assert(ed.cx == 4);

    editor_move_home(&ed, false);
    assert(ed.cx == 0);

    editor_free(&ed);
}

static void test_selection_and_tab_indent(void)
{
    editor_t ed;
    editor_init(&ed, 24, 80);
    set_line(&ed, 0, "    int x;");

    ed.cy = 0;
    ed.cx = 4;
    editor_move_right(&ed, true);
    editor_move_right(&ed, true);
    editor_move_right(&ed, true);

    assert(editor_has_selection(&ed));
    size_t r1;
    size_t c1;
    size_t r2;
    size_t c2;
    editor_selection_range(&ed, &r1, &c1, &r2, &c2);
    assert(r1 == 0 && c1 == 4 && r2 == 0 && c2 == 7);

    editor_handle_tab(&ed, false);
    editor_selection_range(&ed, &r1, &c1, &r2, &c2);
    assert(c1 == 5 && c2 == 8);
    assert(memcmp(ed.buf.lines[0].chars + c1, "int", 3) == 0);

    editor_free(&ed);
}

int main(void)
{
    test_smart_home();
    test_selection_and_tab_indent();
    return 0;
}
