/* SPDX-FileCopyrightText: AnmiTaliDev <anmitalidev@nuros.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <assert.h>

#include "buffer.h"
#include "search.h"

int main(void)
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
    return 0;
}
