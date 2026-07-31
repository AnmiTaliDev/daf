/* SPDX-FileCopyrightText: AnmiTaliDev <anmitalidev@nuros.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <assert.h>
#include <string.h>

#include "theme.h"

int main(void)
{
    const theme_t *dark = theme_find("dark");
    assert(dark != NULL);
    assert(strcmp(dark->name, "dark") == 0);

    const theme_t *light = theme_find("light");
    assert(light != NULL);
    assert(strcmp(light->name, "light") == 0);

    const theme_t *mono = theme_find("mono");
    assert(mono != NULL);
    assert(strcmp(mono->name, "mono") == 0);
    assert(strcmp(mono->normal, "") == 0);
    assert(strcmp(mono->gutter_current, "\x1b[7m") == 0);
    assert(strcmp(mono->search_match, "\x1b[4m") == 0);

    assert(theme_find("does-not-exist") == NULL);

    const char *default_name = theme_build_default_name();
    assert(theme_find(default_name) != NULL);

    return 0;
}
