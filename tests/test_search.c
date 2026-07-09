/* SPDX-FileCopyrightText: AnmiTaliDev <anmitalidev@nuros.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <assert.h>
#include <stdlib.h>

#include "buffer.h"
#include "search.h"

static void test_find_forward(void)
{
    buffer_t buf;
    buffer_init(&buf);
    buffer_insert_bytes(&buf, 0, 0, "foo bar", 7);
    buffer_split_line(&buf, 0, 7);
    buffer_insert_bytes(&buf, 1, 0, "baz foo", 7);

    size_t row = 0;
    size_t col = 0;

    assert(search_find_forward(&buf, 0, 0, "foo", &row, &col));
    assert(row == 0 && col == 0);

    assert(search_find_forward(&buf, 0, 1, "foo", &row, &col));
    assert(row == 1 && col == 4);

    assert(search_find_forward(&buf, 1, 5, "foo", &row, &col));
    assert(row == 0 && col == 0);

    assert(!search_find_forward(&buf, 0, 0, "missing", &row, &col));
    assert(!search_find_forward(&buf, 0, 0, "", &row, &col));

    buffer_free(&buf);
}

static void test_find_all(void)
{
    buffer_t buf;
    buffer_init(&buf);
    buffer_insert_bytes(&buf, 0, 0, "foo bar", 7);
    buffer_split_line(&buf, 0, 7);
    buffer_insert_bytes(&buf, 1, 0, "baz foo", 7);

    size_t count = 0;
    search_match_t *matches = search_find_all(&buf, "foo", &count);
    assert(count == 2);
    assert(matches[0].row == 0 && matches[0].col == 0);
    assert(matches[1].row == 1 && matches[1].col == 4);
    free(matches);

    matches = search_find_all(&buf, "missing", &count);
    assert(count == 0);
    assert(matches == NULL);

    matches = search_find_all(&buf, "", &count);
    assert(count == 0);
    assert(matches == NULL);

    buffer_free(&buf);
}

static void test_find_all_non_overlapping(void)
{
    buffer_t buf;
    buffer_init(&buf);
    buffer_insert_bytes(&buf, 0, 0, "aaaa", 4);

    size_t count = 0;
    search_match_t *matches = search_find_all(&buf, "aa", &count);
    assert(count == 2);
    assert(matches[0].col == 0);
    assert(matches[1].col == 2);
    free(matches);

    buffer_free(&buf);
}

int main(void)
{
    test_find_forward();
    test_find_all();
    test_find_all_non_overlapping();
    return 0;
}
