/* SPDX-FileCopyrightText: AnmiTaliDev <anmitalidev@nuros.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config.h"
#include "editor.h"
#include "input.h"
#include "render.h"
#include "terminal.h"
#include "theme.h"

static int text_rows_for(int terminal_rows)
{
    int text_rows = terminal_rows - 2;
    return text_rows < 1 ? 1 : text_rows;
}

static const theme_t *resolve_theme(const char *cli_theme_name)
{
    daf_config_t config;
    memset(&config, 0, sizeof(config));
    config_load(&config);

    const char *name = cli_theme_name;
    if (name == NULL) {
        name = getenv("DAF_THEME");
    }
    if (name == NULL && config.theme_name[0] != '\0') {
        name = config.theme_name;
    }
    if (name == NULL) {
        name = theme_build_default_name();
    }

    const theme_t *theme = theme_find(name);
    if (theme == NULL) {
        fprintf(stderr, "daf: unknown theme \"%s\", falling back to \"dark\"\n", name);
        theme = theme_find("dark");
    }
    return theme;
}

int main(int argc, char **argv)
{
    const char *cli_theme_name = NULL;
    const char *filename = NULL;

    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "--theme=", 8) == 0) {
            cli_theme_name = argv[i] + 8;
        } else if (filename == NULL) {
            filename = argv[i];
        } else {
            fprintf(stderr, "usage: daf [--theme=name] [file]\n");
            return EXIT_FAILURE;
        }
    }

    const theme_t *theme = resolve_theme(cli_theme_name);

    terminal_enable_raw_mode();

    int rows;
    int cols;
    if (terminal_get_window_size(&rows, &cols) != 0) {
        rows = 24;
        cols = 80;
    }

    editor_t ed;
    editor_init(&ed, text_rows_for(rows), cols);
    ed.theme = theme;

    if (filename != NULL) {
        editor_open(&ed, filename);
    }

    while (!ed.should_quit) {
        if (terminal_consume_resize()) {
            if (terminal_get_window_size(&rows, &cols) == 0) {
                editor_update_screen_size(&ed, text_rows_for(rows), cols);
            }
        }
        render_screen(&ed);
        key_event_t key = terminal_read_key();
        input_process_key(&ed, key);
    }

    ssize_t written = write(STDOUT_FILENO, "\x1b[2J\x1b[H", 7);
    (void)written;

    editor_free(&ed);
    return EXIT_SUCCESS;
}
