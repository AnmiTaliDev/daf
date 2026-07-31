/* SPDX-FileCopyrightText: AnmiTaliDev <anmitalidev@nuros.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef DAF_CONFIG_SUBPATH
#define DAF_CONFIG_SUBPATH "daf/config"
#endif

static void build_config_path(char *out, size_t out_size)
{
    const char *xdg = getenv("XDG_CONFIG_HOME");
    if (xdg != NULL && xdg[0] != '\0') {
        snprintf(out, out_size, "%s/%s", xdg, DAF_CONFIG_SUBPATH);
        return;
    }

    const char *home = getenv("HOME");
    if (home != NULL && home[0] != '\0') {
        snprintf(out, out_size, "%s/.config/%s", home, DAF_CONFIG_SUBPATH);
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

static void apply_line(daf_config_t *out, char *line)
{
    char *s = trim(line);
    if (s[0] == '\0' || s[0] == '#') {
        return;
    }

    char *eq = strchr(s, '=');
    if (eq == NULL) {
        return;
    }
    *eq = '\0';
    char *key = trim(s);
    char *value = trim(eq + 1);

    if (strcmp(key, "theme") == 0) {
        snprintf(out->theme_name, sizeof(out->theme_name), "%s", value);
    }
}

void config_load(daf_config_t *out)
{
    char path[512];
    build_config_path(path, sizeof(path));
    if (path[0] == '\0') {
        return;
    }

    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), fp) != NULL) {
        apply_line(out, line);
    }

    fclose(fp);
}

const char *config_subpath(void)
{
    return DAF_CONFIG_SUBPATH;
}
