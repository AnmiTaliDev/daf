/* SPDX-FileCopyrightText: AnmiTaliDev <anmitalidev@nuros.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "input.h"

#include <stdlib.h>

#include "filetype.h"
#include "search.h"

#define CTRL_KEY(c) ((c) - 'a' + 1)

static void handle_quit(editor_t *ed)
{
    if (ed->buf.dirty && !ed->quit_confirm_pending) {
        ed->quit_confirm_pending = true;
        editor_set_status(ed, "Unsaved changes. Press Ctrl-Q again to quit without saving.");
        return;
    }
    ed->should_quit = true;
}

static void handle_save(editor_t *ed)
{
    if (ed->buf.filename == NULL) {
        ed->mode = MODE_PROMPT_SAVE_AS;
        ed->prompt_len = 0;
        ed->prompt_input[0] = '\0';
        return;
    }
    if (buffer_save(&ed->buf) == 0) {
        editor_set_status(ed, "Saved \"%s\"", ed->buf.filename);
    } else {
        editor_set_status(ed, "Error: could not save \"%s\"", ed->buf.filename);
    }
}

static void enter_find_mode(editor_t *ed)
{
    ed->mode = MODE_PROMPT_FIND;
    ed->prompt_len = 0;
    ed->prompt_input[0] = '\0';
    ed->search_start_cy = ed->cy;
    ed->search_start_cx = ed->cx;
}

static void enter_goto_mode(editor_t *ed)
{
    ed->mode = MODE_PROMPT_GOTO;
    ed->prompt_len = 0;
    ed->prompt_input[0] = '\0';
}

static void prompt_append_char(editor_t *ed, unsigned int codepoint)
{
    if (codepoint >= 128) {
        return;
    }
    if (ed->prompt_len + 1 >= sizeof(ed->prompt_input)) {
        return;
    }
    ed->prompt_input[ed->prompt_len++] = (char)codepoint;
    ed->prompt_input[ed->prompt_len] = '\0';
}

static void prompt_backspace(editor_t *ed)
{
    if (ed->prompt_len > 0) {
        ed->prompt_len--;
        ed->prompt_input[ed->prompt_len] = '\0';
    }
}

static void do_incremental_search(editor_t *ed)
{
    if (ed->prompt_len == 0) {
        ed->cy = ed->search_start_cy;
        ed->cx = ed->search_start_cx;
        editor_clear_search_matches(ed);
        return;
    }

    size_t row, col;
    if (search_find_forward(&ed->buf, ed->search_start_cy, ed->search_start_cx, ed->prompt_input,
                             &row, &col)) {
        ed->cy = row;
        ed->cx = col;
    }

    size_t count;
    search_match_t *matches = search_find_all(&ed->buf, ed->prompt_input, &count);
    editor_set_search_matches(ed, matches, count);
}

static void confirm_prompt(editor_t *ed)
{
    switch (ed->mode) {
    case MODE_PROMPT_FIND:
        ed->mode = MODE_EDIT;
        editor_clear_search_matches(ed);
        editor_set_status(ed, "Ln %zu, Col %zu", ed->cy + 1, ed->cx + 1);
        break;

    case MODE_PROMPT_GOTO: {
        long line_number = strtol(ed->prompt_input, NULL, 10);
        ed->mode = MODE_EDIT;
        if (line_number > 0) {
            editor_goto_line(ed, line_number);
            editor_set_status(ed, "Ln %zu", ed->cy + 1);
        } else {
            editor_set_status(ed, "Cancelled");
        }
        break;
    }

    case MODE_PROMPT_SAVE_AS:
        ed->mode = MODE_EDIT;
        if (ed->prompt_len > 0) {
            if (buffer_save_as(&ed->buf, ed->prompt_input) == 0) {
                ed->filetype = filetype_detect(ed->buf.filename);
                editor_set_status(ed, "Saved \"%s\"", ed->buf.filename);
            } else {
                editor_set_status(ed, "Error: could not save \"%s\"", ed->prompt_input);
            }
        } else {
            editor_set_status(ed, "Cancelled");
        }
        break;

    case MODE_EDIT:
    default:
        break;
    }
}

static void process_prompt_key(editor_t *ed, key_event_t key)
{
    if (key.type == KEY_ESCAPE) {
        if (ed->mode == MODE_PROMPT_FIND) {
            ed->cy = ed->search_start_cy;
            ed->cx = ed->search_start_cx;
            editor_clear_search_matches(ed);
        }
        ed->mode = MODE_EDIT;
        editor_set_status(ed, "Cancelled");
        return;
    }

    if (key.type == KEY_BACKSPACE) {
        prompt_backspace(ed);
        if (ed->mode == MODE_PROMPT_FIND) {
            do_incremental_search(ed);
        }
        return;
    }

    if (key.type == KEY_ENTER) {
        confirm_prompt(ed);
        return;
    }

    if (key.type == KEY_CHAR && (key.mods & MOD_CTRL) && ed->mode == MODE_PROMPT_FIND &&
        key.codepoint == CTRL_KEY('f')) {
        size_t row, col;
        if (search_find_forward(&ed->buf, ed->cy, ed->cx + 1, ed->prompt_input, &row, &col)) {
            ed->cy = row;
            ed->cx = col;
        }
        return;
    }

    if (key.type == KEY_CHAR && !(key.mods & MOD_CTRL)) {
        prompt_append_char(ed, key.codepoint);
        if (ed->mode == MODE_PROMPT_FIND) {
            do_incremental_search(ed);
        }
    }
}

void input_process_key(editor_t *ed, key_event_t key)
{
    if (ed->mode != MODE_EDIT) {
        process_prompt_key(ed, key);
        return;
    }

    if (key.type == KEY_MOUSE) {
        switch (key.mouse_button) {
        case MOUSE_LEFT_PRESS:
            editor_mouse_press(ed, key.mouse_row, key.mouse_col);
            break;
        case MOUSE_LEFT_DRAG:
            editor_mouse_drag(ed, key.mouse_row, key.mouse_col);
            break;
        case MOUSE_LEFT_RELEASE:
            editor_mouse_release(ed);
            break;
        case MOUSE_SCROLL_UP:
            editor_scroll_view(ed, -3);
            break;
        case MOUSE_SCROLL_DOWN:
            editor_scroll_view(ed, 3);
            break;
        case MOUSE_NONE:
        default:
            break;
        }
        return;
    }

    if (key.type == KEY_PASTE) {
        editor_paste_text(ed, key.paste_text, key.paste_len);
        editor_set_status(ed, "Pasted from terminal");
        return;
    }

    bool is_ctrl_key = key.type == KEY_CHAR && (key.mods & MOD_CTRL) != 0;
    if (!(is_ctrl_key && key.codepoint == CTRL_KEY('q'))) {
        ed->quit_confirm_pending = false;
    }

    if (is_ctrl_key) {
        if (key.codepoint == CTRL_KEY('q')) {
            handle_quit(ed);
        } else if (key.codepoint == CTRL_KEY('s')) {
            handle_save(ed);
        } else if (key.codepoint == CTRL_KEY('f')) {
            enter_find_mode(ed);
        } else if (key.codepoint == CTRL_KEY('g')) {
            enter_goto_mode(ed);
        } else if (key.codepoint == CTRL_KEY('z')) {
            editor_undo(ed);
        } else if (key.codepoint == CTRL_KEY('y')) {
            editor_redo(ed);
        } else if (key.codepoint == CTRL_KEY('c')) {
            if (editor_has_selection(ed)) {
                editor_copy(ed);
                terminal_set_clipboard(ed->clipboard, ed->clipboard_len);
                editor_set_status(ed, "Copied");
            } else {
                editor_set_status(ed, "Nothing selected");
            }
        } else if (key.codepoint == CTRL_KEY('x')) {
            if (editor_has_selection(ed)) {
                editor_cut(ed);
                terminal_set_clipboard(ed->clipboard, ed->clipboard_len);
                editor_set_status(ed, "Cut");
            } else {
                editor_set_status(ed, "Nothing selected");
            }
        } else if (key.codepoint == CTRL_KEY('v')) {
            if (ed->clipboard != NULL) {
                editor_paste(ed);
                editor_set_status(ed, "Pasted");
            }
        }
        return;
    }

    bool shift = (key.mods & MOD_SHIFT) != 0;

    switch (key.type) {
    case KEY_UP:
        editor_move_up(ed, shift);
        break;
    case KEY_DOWN:
        editor_move_down(ed, shift);
        break;
    case KEY_LEFT:
        editor_move_left(ed, shift);
        break;
    case KEY_RIGHT:
        editor_move_right(ed, shift);
        break;
    case KEY_HOME:
        editor_move_home(ed, shift);
        break;
    case KEY_END:
        editor_move_end(ed, shift);
        break;
    case KEY_PAGE_UP:
        editor_move_page(ed, true, shift);
        break;
    case KEY_PAGE_DOWN:
        editor_move_page(ed, false, shift);
        break;
    case KEY_DELETE:
        editor_delete_forward(ed);
        break;
    case KEY_BACKSPACE:
        editor_backspace(ed);
        break;
    case KEY_ENTER:
        editor_insert_newline(ed);
        break;
    case KEY_TAB:
        editor_handle_tab(ed, shift);
        break;
    case KEY_ESCAPE:
        editor_clear_selection(ed);
        break;
    case KEY_CHAR:
        editor_insert_codepoint(ed, key.codepoint);
        break;
    default:
        break;
    }
}
