/* SPDX-FileCopyrightText: AnmiTaliDev <anmitalidev@nuros.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "theme.h"

static void write_file(const char *path, const char *content)
{
    FILE *fp = fopen(path, "w");
    assert(fp != NULL);
    fputs(content, fp);
    fclose(fp);
}

static void test_builtin_themes(void)
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
}

static void test_load_prefers_builtin(void)
{
    assert(theme_load("dark") == theme_find("dark"));
}

static void test_load_missing_returns_null(void)
{
    setenv("XDG_CONFIG_HOME", "/tmp/daf_theme_test_does_not_exist", 1);
    assert(theme_load("no-such-custom-theme") == NULL);
}

static const char *setup_theme_dir(void)
{
    static char base[256];
    snprintf(base, sizeof(base), "/tmp/daf_theme_test_%d", (int)getpid());
    mkdir(base, 0700);

    char subpath_copy[256];
    snprintf(subpath_copy, sizeof(subpath_copy), "%s", theme_dir_subpath());

    char path[600];
    snprintf(path, sizeof(path), "%s", base);

    char *segment = strtok(subpath_copy, "/");
    while (segment != NULL) {
        size_t len = strlen(path);
        snprintf(path + len, sizeof(path) - len, "/%s", segment);
        mkdir(path, 0700);
        segment = strtok(NULL, "/");
    }

    setenv("XDG_CONFIG_HOME", base, 1);
    return base;
}

static void test_load_custom_theme_with_base(void)
{
    const char *base = setup_theme_dir();
    char path[700];
    snprintf(path, sizeof(path), "%s/%s/mytheme.conf", base, theme_dir_subpath());
    write_file(path, "# custom theme\nbase=light\nselection=196,231\n");

    const theme_t *custom = theme_load("mytheme");
    assert(custom != NULL);
    assert(strcmp(custom->name, "mytheme") == 0);
    assert(strcmp(custom->selection, "\x1b[38;5;196;48;5;231m") == 0);
    /* Roles not overridden inherit from the requested base (light). */
    const theme_t *light = theme_find("light");
    assert(strcmp(custom->normal, light->normal) == 0);
    assert(strcmp(custom->status_bar, light->status_bar) == 0);
}

static void test_load_custom_theme_defaults_to_dark_base(void)
{
    const char *base = setup_theme_dir();
    char path[700];
    snprintf(path, sizeof(path), "%s/%s/nobase.conf", base, theme_dir_subpath());
    write_file(path, "tilde=250\n");

    const theme_t *custom = theme_load("nobase");
    assert(custom != NULL);
    assert(strcmp(custom->tilde, "\x1b[38;5;250m") == 0);
    const theme_t *dark = theme_find("dark");
    assert(strcmp(custom->normal, dark->normal) == 0);
}

int main(void)
{
    test_builtin_themes();
    test_load_prefers_builtin();
    test_load_missing_returns_null();
    test_load_custom_theme_with_base();
    test_load_custom_theme_defaults_to_dark_base();
    return 0;
}
