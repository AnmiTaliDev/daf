/* SPDX-FileCopyrightText: AnmiTaliDev <anmitalidev@nuros.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <assert.h>
#include <string.h>

#include "filetype.h"

int main(void)
{
    assert(strcmp(filetype_detect("main.c"), "C") == 0);
    assert(strcmp(filetype_detect("app.py"), "Python") == 0);
    assert(strcmp(filetype_detect("path/to/file.MD"), "Markdown") == 0);
    assert(strcmp(filetype_detect("noext"), "Plain Text") == 0);
    assert(strcmp(filetype_detect(".hidden"), "Plain Text") == 0);
    return 0;
}
