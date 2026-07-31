/* SPDX-FileCopyrightText: AnmiTaliDev <anmitalidev@nuros.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "theme.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

#ifndef DAF_DEFAULT_THEME
#define DAF_DEFAULT_THEME "dark"
#endif

#ifndef DAF_THEME_DIR_SUBPATH
#define DAF_THEME_DIR_SUBPATH "daf/themes"
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

const char *theme_dir_subpath(void)
{
    return DAF_THEME_DIR_SUBPATH;
}

static void build_theme_path(char *out, size_t out_size, const char *name)
{
    const char *xdg = getenv("XDG_CONFIG_HOME");
    if (xdg != NULL && xdg[0] != '\0') {
        snprintf(out, out_size, "%s/%s/%s.conf", xdg, DAF_THEME_DIR_SUBPATH, name);
        return;
    }

    const char *home = getenv("HOME");
    if (home != NULL && home[0] != '\0') {
        snprintf(out, out_size, "%s/.config/%s/%s.conf", home, DAF_THEME_DIR_SUBPATH, name);
        return;
    }

    out[0] = '\0';
}

static char *trim(char *s)
{
    while (*s == ' ' || *s == '\t') {
        s++;
    }
    size_t len = strlen(s);
    while (len > 0 &&
           (s[len - 1] == ' ' || s[len - 1] == '\t' || s[len - 1] == '\n' || s[len - 1] == '\r')) {
        s[--len] = '\0';
    }
    return s;
}

static char *build_sgr(const char *spec)
{
    int fg = -1;
    int bg = -1;
    sscanf(spec, "%d,%d", &fg, &bg);
    if (fg < 0) {
        return NULL;
    }

    char buf[64];
    if (bg >= 0) {
        snprintf(buf, sizeof(buf), "\x1b[38;5;%d;48;5;%dm", fg, bg);
    } else {
        snprintf(buf, sizeof(buf), "\x1b[38;5;%dm", fg);
    }
    return xstrdup(buf);
}

static void apply_role(theme_t *custom, const char *key, char *sgr)
{
    if (strcmp(key, "normal") == 0) {
        custom->normal = sgr;
    } else if (strcmp(key, "gutter") == 0) {
        custom->gutter = sgr;
    } else if (strcmp(key, "gutter_current") == 0) {
        custom->gutter_current = sgr;
    } else if (strcmp(key, "selection") == 0) {
        custom->selection = sgr;
    } else if (strcmp(key, "search_match") == 0) {
        custom->search_match = sgr;
    } else if (strcmp(key, "status_bar") == 0) {
        custom->status_bar = sgr;
    } else if (strcmp(key, "tilde") == 0) {
        custom->tilde = sgr;
    } else {
        free(sgr);
    }
}

const theme_t *theme_load(const char *name)
{
    const theme_t *builtin = theme_find(name);
    if (builtin != NULL) {
        return builtin;
    }

    char path[512];
    build_theme_path(path, sizeof(path), name);
    if (path[0] == '\0') {
        return NULL;
    }

    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        return NULL;
    }

    const theme_t *base = &k_theme_dark;
    char line[256];
    while (fgets(line, sizeof(line), fp) != NULL) {
        char *s = trim(line);
        if (s[0] == '\0' || s[0] == '#') {
            continue;
        }
        char *eq = strchr(s, '=');
        if (eq == NULL) {
            continue;
        }
        *eq = '\0';
        char *key = trim(s);
        char *value = trim(eq + 1);
        if (strcmp(key, "base") == 0) {
            const theme_t *found = theme_find(value);
            if (found != NULL) {
                base = found;
            }
        }
    }

    theme_t *custom = xmalloc(sizeof(theme_t));
    *custom = *base;
    custom->name = xstrdup(name);

    rewind(fp);
    while (fgets(line, sizeof(line), fp) != NULL) {
        char *s = trim(line);
        if (s[0] == '\0' || s[0] == '#') {
            continue;
        }
        char *eq = strchr(s, '=');
        if (eq == NULL) {
            continue;
        }
        *eq = '\0';
        char *key = trim(s);
        char *value = trim(eq + 1);
        if (strcmp(key, "base") == 0) {
            continue;
        }

        char *sgr = build_sgr(value);
        if (sgr != NULL) {
            apply_role(custom, key, sgr);
        }
    }

    fclose(fp);
    return custom;
}
