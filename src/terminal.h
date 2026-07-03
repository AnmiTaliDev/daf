/* SPDX-FileCopyrightText: AnmiTaliDev <anmitalidev@nuros.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef DAF_TERMINAL_H
#define DAF_TERMINAL_H

enum key_type {
    KEY_NONE = 0,
    KEY_CHAR,
    KEY_UP,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_HOME,
    KEY_END,
    KEY_PAGE_UP,
    KEY_PAGE_DOWN,
    KEY_DELETE,
    KEY_BACKSPACE,
    KEY_ENTER,
    KEY_TAB,
    KEY_ESCAPE,
    KEY_MOUSE,
};

enum key_mod {
    MOD_NONE = 0,
    MOD_SHIFT = 1 << 0,
    MOD_ALT = 1 << 1,
    MOD_CTRL = 1 << 2,
};

enum mouse_button {
    MOUSE_NONE = 0,
    MOUSE_LEFT_PRESS,
    MOUSE_LEFT_DRAG,
    MOUSE_LEFT_RELEASE,
    MOUSE_SCROLL_UP,
    MOUSE_SCROLL_DOWN,
};

typedef struct {
    enum key_type type;
    unsigned int codepoint;
    int mods;

    enum mouse_button mouse_button;
    int mouse_row;
    int mouse_col;
} key_event_t;

void terminal_enable_raw_mode(void);
void terminal_disable_raw_mode(void);
int terminal_get_window_size(int *rows, int *cols);
int terminal_consume_resize(void);
key_event_t terminal_read_key(void);

#endif /* DAF_TERMINAL_H */
