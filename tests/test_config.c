/* SPDX-FileCopyrightText: AnmiTaliDev <anmitalidev@nuros.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "config.h"

static void write_file(const char *path, const char *content)
{
    FILE *fp = fopen(path, "w");
    assert(fp != NULL);
    fputs(content, fp);
    fclose(fp);
}

int main(void)
{
    char base[256];
    snprintf(base, sizeof(base), "/tmp/daf_config_test_%d", (int)getpid());
    mkdir(base, 0700);

    char full_path[600];
    snprintf(full_path, sizeof(full_path), "%s/%s", base, config_subpath());

    char dir_path[600];
    strcpy(dir_path, full_path);
    char *slash = strrchr(dir_path, '/');
    if (slash != NULL) {
        *slash = '\0';
        mkdir(dir_path, 0700);
    }

    write_file(full_path, "# a comment\n\n  theme = light  \nignored line\n");
    setenv("XDG_CONFIG_HOME", base, 1);

    daf_config_t config;
    memset(&config, 0, sizeof(config));
    config_load(&config);
    assert(strcmp(config.theme_name, "light") == 0);

    daf_config_t missing;
    memset(&missing, 0, sizeof(missing));
    setenv("XDG_CONFIG_HOME", "/tmp/daf_config_test_does_not_exist", 1);
    config_load(&missing);
    assert(missing.theme_name[0] == '\0');

    return 0;
}
