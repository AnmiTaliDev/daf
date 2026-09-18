/* SPDX-FileCopyrightText: AnmiTaliDev <anmitalidev@nuros.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "editor.h"
#include "syntax.h"

static void test_syntax_unsupported_or_null(void)
{
    assert(syntax_create(NULL) == NULL);
    assert(syntax_create("UnknownType") == NULL);
}

static void test_syntax_c_highlight(void)
{
    syntax_highlighter_t *hl = syntax_create("C");
    if (hl == NULL) {
        return;
    }

    buffer_t buf;
    buffer_init(&buf);

    const char *line_text = "int count = 42;";
    buffer_insert_bytes(&buf, 0, 0, line_text, strlen(line_text));

    syntax_update(hl, &buf);

    uint8_t roles[64];
    memset(roles, 0, sizeof(roles));
    syntax_get_line_roles(hl, 0, &buf.lines[0], roles);

    assert(roles[0] == SYNTAX_ROLE_TYPE);
    assert(roles[1] == SYNTAX_ROLE_TYPE);
    assert(roles[2] == SYNTAX_ROLE_TYPE);

    assert(roles[12] == SYNTAX_ROLE_NUMBER);
    assert(roles[13] == SYNTAX_ROLE_NUMBER);

    buffer_free(&buf);
    syntax_destroy(hl);
}

static void test_editor_syntax_lifecycle(void)
{
    editor_t ed;
    editor_init(&ed, 24, 80);

    ed.filetype = "C";
    editor_setup_syntax(&ed);

    const char *code = "return 10;";
    buffer_insert_bytes(&ed.buf, 0, 0, code, strlen(code));

    if (ed.syntax != NULL) {
        syntax_update(ed.syntax, &ed.buf);

        uint8_t roles[32];
        memset(roles, 0, sizeof(roles));
        syntax_get_line_roles(ed.syntax, 0, &ed.buf.lines[0], roles);

        assert(roles[0] == SYNTAX_ROLE_KEYWORD);
        assert(roles[5] == SYNTAX_ROLE_KEYWORD);
    }

    editor_free(&ed);
}

int main(void)
{
    test_syntax_unsupported_or_null();
    test_syntax_c_highlight();
    test_editor_syntax_lifecycle();
    return 0;
}
