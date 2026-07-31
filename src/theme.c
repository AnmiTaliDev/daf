/* SPDX-FileCopyrightText: AnmiTaliDev <anmitalidev@nuros.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "theme.h"

#include <string.h>

#ifndef DAF_DEFAULT_THEME
#define DAF_DEFAULT_THEME "dark"
#endif

static const theme_t k_theme_dark = {
    .name = "dark",
    .normal = "\x1b[38;5;253;48;5;234m",
    .gutter = "\x1b[38;5;245;48;5;236m",
    .gutter_current = "\x1b[38;5;220;48;5;238m",
    .selection = "\x1b[38;5;234;48;5;75m",
    .search_match = "\x1b[38;5;234;48;5;221m",
    .status_bar = "\x1b[38;5;234;48;5;250m",
    .tilde = "\x1b[38;5;238;48;5;234m",
    .reset = "\x1b[0m",
};

static const theme_t k_theme_light = {
    .name = "light",
    .normal = "\x1b[38;5;236;48;5;255m",
    .gutter = "\x1b[38;5;249;48;5;253m",
    .gutter_current = "\x1b[38;5;130;48;5;251m",
    .selection = "\x1b[38;5;255;48;5;68m",
    .search_match = "\x1b[38;5;236;48;5;222m",
    .status_bar = "\x1b[38;5;255;48;5;60m",
    .tilde = "\x1b[38;5;250;48;5;255m",
    .reset = "\x1b[0m",
};

static const theme_t k_theme_mono = {
    .name = "mono",
    .normal = "",
    .gutter = "",
    .gutter_current = "\x1b[7m",
    .selection = "\x1b[7m",
    .search_match = "\x1b[4m",
    .status_bar = "\x1b[7m",
    .tilde = "",
    .reset = "\x1b[0m",
};

const theme_t *theme_find(const char *name)
{
    if (strcmp(name, "dark") == 0) {
        return &k_theme_dark;
    }
    if (strcmp(name, "light") == 0) {
        return &k_theme_light;
    }
    if (strcmp(name, "mono") == 0) {
        return &k_theme_mono;
    }
    return NULL;
}

const char *theme_build_default_name(void)
{
    return DAF_DEFAULT_THEME;
}
