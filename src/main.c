/* SPDX-FileCopyrightText: AnmiTaliDev <anmitalidev@nuros.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "editor.h"
#include "input.h"
#include "render.h"
#include "terminal.h"

static int text_rows_for(int terminal_rows)
{
    int text_rows = terminal_rows - 2;
    return text_rows < 1 ? 1 : text_rows;
}

int main(int argc, char **argv)
{
    if (argc > 2) {
        fprintf(stderr, "usage: daf [file]\n");
        return EXIT_FAILURE;
    }

    terminal_enable_raw_mode();

    int rows;
    int cols;
    if (terminal_get_window_size(&rows, &cols) != 0) {
        rows = 24;
        cols = 80;
    }

    editor_t ed;
    editor_init(&ed, text_rows_for(rows), cols);

    if (argc == 2) {
        editor_open(&ed, argv[1]);
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
