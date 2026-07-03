/* SPDX-FileCopyrightText: AnmiTaliDev <anmitalidev@nuros.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "buffer.h"

static void test_insert_and_delete(void)
{
    buffer_t buf;
    buffer_init(&buf);

    buffer_insert_bytes(&buf, 0, 0, "hello", 5);
    assert(buf.lines[0].len == 5);
    assert(memcmp(buf.lines[0].chars, "hello", 5) == 0);

    buffer_delete_range(&buf, 0, 1, 0, 3);
    assert(buf.lines[0].len == 3);
    assert(memcmp(buf.lines[0].chars, "hlo", 3) == 0);

    buffer_free(&buf);
}

static void test_split_and_join(void)
{
    buffer_t buf;
    buffer_init(&buf);

    buffer_insert_bytes(&buf, 0, 0, "abcdef", 6);
    buffer_split_line(&buf, 0, 3);
    assert(buf.num_lines == 2);
    assert(buf.lines[0].len == 3);
    assert(memcmp(buf.lines[0].chars, "abc", 3) == 0);
    assert(buf.lines[1].len == 3);
    assert(memcmp(buf.lines[1].chars, "def", 3) == 0);

    buffer_join_lines(&buf, 0);
    assert(buf.num_lines == 1);
    assert(buf.lines[0].len == 6);
    assert(memcmp(buf.lines[0].chars, "abcdef", 6) == 0);

    buffer_free(&buf);
}

static void test_load_save_roundtrip(void)
{
    const char *path = "test_buffer_roundtrip.tmp";

    buffer_t buf;
    buffer_init(&buf);
    buffer_insert_bytes(&buf, 0, 0, "line one", 8);
    buffer_split_line(&buf, 0, 8);
    buffer_insert_bytes(&buf, 1, 0, "line two", 8);

    assert(buffer_save_as(&buf, path) == 0);
    buffer_free(&buf);

    buffer_t loaded;
    buffer_init(&loaded);
    assert(buffer_load(&loaded, path) == 0);
    assert(loaded.num_lines == 2);
    assert(memcmp(loaded.lines[0].chars, "line one", 8) == 0);
    assert(memcmp(loaded.lines[1].chars, "line two", 8) == 0);
    assert(loaded.dirty == 0);
    assert(loaded.trailing_newline == 1);

    buffer_free(&loaded);
    remove(path);
}

int main(void)
{
    test_insert_and_delete();
    test_split_and_join();
    test_load_save_roundtrip();
    return 0;
}
