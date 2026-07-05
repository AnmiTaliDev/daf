/* SPDX-FileCopyrightText: AnmiTaliDev <anmitalidev@nuros.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef DAF_EDITOR_H
#define DAF_EDITOR_H

#include <stdbool.h>
#include <stddef.h>

#include "buffer.h"

typedef enum {
    MODE_EDIT,
    MODE_PROMPT_FIND,
    MODE_PROMPT_GOTO,
    MODE_PROMPT_SAVE_AS,
} editor_mode_t;

typedef struct {
    bool active;
    size_t anchor_row;
    size_t anchor_col;
} selection_t;

typedef struct {
    size_t row;
    size_t col;
    char *old_text;
    size_t old_len;
    char *new_text;
    size_t new_len;
} edit_record_t;

typedef struct {
    buffer_t buf;

    size_t cy;
    size_t cx;
    size_t goal_rx;

    size_t rowoff;
    size_t coloff;
    int screen_rows;
    int screen_cols;

    selection_t sel;

    editor_mode_t mode;
    char prompt_input[256];
    size_t prompt_len;
    size_t search_start_cy;
    size_t search_start_cx;

    char status_msg[256];
    const char *filetype;

    edit_record_t *undo_stack;
    size_t undo_count;
    size_t undo_cap;
    edit_record_t *redo_stack;
    size_t redo_count;
    size_t redo_cap;

    char *clipboard;
    size_t clipboard_len;

    bool quit_confirm_pending;
    bool should_quit;
} editor_t;

void editor_init(editor_t *ed, int screen_rows, int screen_cols);
void editor_free(editor_t *ed);
void editor_open(editor_t *ed, const char *filename);
void editor_update_screen_size(editor_t *ed, int screen_rows, int screen_cols);

void editor_set_status(editor_t *ed, const char *fmt, ...);

int editor_gutter_width(const editor_t *ed);
int editor_text_cols(const editor_t *ed);
void editor_scroll(editor_t *ed);

size_t editor_cx_to_rx(const line_t *line, size_t cx);
size_t editor_rx_to_cx(const line_t *line, size_t target_rx);

bool editor_has_selection(const editor_t *ed);
void editor_selection_range(const editor_t *ed, size_t *r1, size_t *c1, size_t *r2, size_t *c2);
void editor_clear_selection(editor_t *ed);

void editor_move_left(editor_t *ed, bool extend);
void editor_move_right(editor_t *ed, bool extend);
void editor_move_up(editor_t *ed, bool extend);
void editor_move_down(editor_t *ed, bool extend);
void editor_move_home(editor_t *ed, bool extend);
void editor_move_end(editor_t *ed, bool extend);
void editor_move_page(editor_t *ed, bool up, bool extend);
void editor_goto_line(editor_t *ed, long line_number);

void editor_insert_codepoint(editor_t *ed, unsigned int codepoint);
void editor_insert_newline(editor_t *ed);
void editor_backspace(editor_t *ed);
void editor_delete_forward(editor_t *ed);
void editor_handle_tab(editor_t *ed, bool shift);

void editor_mouse_press(editor_t *ed, int screen_row, int screen_col);
void editor_mouse_drag(editor_t *ed, int screen_row, int screen_col);
void editor_mouse_release(editor_t *ed);
void editor_scroll_view(editor_t *ed, int delta_lines);

void editor_undo(editor_t *ed);
void editor_redo(editor_t *ed);

void editor_copy(editor_t *ed);
void editor_cut(editor_t *ed);
void editor_paste(editor_t *ed);
void editor_paste_text(editor_t *ed, const char *text, size_t len);

#endif /* DAF_EDITOR_H */
