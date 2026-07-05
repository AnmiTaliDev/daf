/* SPDX-FileCopyrightText: AnmiTaliDev <anmitalidev@nuros.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "terminal.h"

#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "common.h"

static struct termios g_orig_termios;
static int g_raw_mode_active = 0;
static volatile sig_atomic_t g_resized = 0;

static char *g_paste_buf = NULL;
static size_t g_paste_len = 0;
static size_t g_paste_cap = 0;

static void handle_sigwinch(int signum)
{
    (void)signum;
    g_resized = 1;
}

static void write_str(const char *s)
{
    ssize_t written = write(STDOUT_FILENO, s, strlen(s));
    (void)written;
}

void terminal_disable_raw_mode(void)
{
    if (g_raw_mode_active) {
        write_str("\x1b[?2004l\x1b[?1006l\x1b[?1002l");
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_orig_termios);
        g_raw_mode_active = 0;
    }
}

void terminal_enable_raw_mode(void)
{
    if (tcgetattr(STDIN_FILENO, &g_orig_termios) == -1) {
        fprintf(stderr, "daf: tcgetattr failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    atexit(terminal_disable_raw_mode);

    struct termios raw = g_orig_termios;
    raw.c_iflag &= (tcflag_t)~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= (tcflag_t)~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= (tcflag_t)~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        fprintf(stderr, "daf: tcsetattr failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    g_raw_mode_active = 1;
    write_str("\x1b[?1002h\x1b[?1006h\x1b[?2004h");

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sigwinch;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGWINCH, &sa, NULL);
}

int terminal_consume_resize(void)
{
    if (g_resized) {
        g_resized = 0;
        return 1;
    }
    return 0;
}

int terminal_get_window_size(int *rows, int *cols)
{
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        return -1;
    }
    *rows = ws.ws_row;
    *cols = ws.ws_col;
    return 0;
}

static int read_raw_byte(char *c)
{
    ssize_t n;
    for (;;) {
        n = read(STDIN_FILENO, c, 1);
        if (n == 1) {
            return 0;
        }
        if (n == -1 && errno != EINTR) {
            return -1;
        }
        if (n == 0) {
            return -1;
        }
    }
}

static int try_read_byte(char *c, int timeout_ms)
{
    struct pollfd pfd;
    pfd.fd = STDIN_FILENO;
    pfd.events = POLLIN;
    int ret = poll(&pfd, 1, timeout_ms);
    if (ret <= 0) {
        return -1;
    }
    return read_raw_byte(c);
}

static key_event_t decode_csi_body(const char *body)
{
    key_event_t ev = {.type = KEY_NONE, .codepoint = 0, .mods = MOD_NONE};
    size_t len = strlen(body);
    if (len == 0) {
        ev.type = KEY_ESCAPE;
        return ev;
    }

    char final = body[len - 1];
    int p1 = 0;
    int p2 = 0;
    sscanf(body, "%d;%d", &p1, &p2);

    if (p2 > 0) {
        int m = p2 - 1;
        if (m & 1) {
            ev.mods |= MOD_SHIFT;
        }
        if (m & 2) {
            ev.mods |= MOD_ALT;
        }
        if (m & 4) {
            ev.mods |= MOD_CTRL;
        }
    }

    switch (final) {
    case 'A':
        ev.type = KEY_UP;
        break;
    case 'B':
        ev.type = KEY_DOWN;
        break;
    case 'C':
        ev.type = KEY_RIGHT;
        break;
    case 'D':
        ev.type = KEY_LEFT;
        break;
    case 'H':
        ev.type = KEY_HOME;
        break;
    case 'F':
        ev.type = KEY_END;
        break;
    case 'Z':
        ev.type = KEY_TAB;
        ev.mods |= MOD_SHIFT;
        break;
    case '~':
        switch (p1) {
        case 1:
            ev.type = KEY_HOME;
            break;
        case 3:
            ev.type = KEY_DELETE;
            break;
        case 4:
            ev.type = KEY_END;
            break;
        case 5:
            ev.type = KEY_PAGE_UP;
            break;
        case 6:
            ev.type = KEY_PAGE_DOWN;
            break;
        default:
            ev.type = KEY_NONE;
            break;
        }
        break;
    default:
        ev.type = KEY_NONE;
        break;
    }
    return ev;
}

static key_event_t read_sgr_mouse_sequence(void)
{
    char body[32];
    size_t idx = 0;
    for (;;) {
        char c;
        if (try_read_byte(&c, 30) != 0) {
            break;
        }
        if (idx + 1 >= sizeof(body)) {
            break;
        }
        body[idx++] = c;
        if (c == 'M' || c == 'm') {
            break;
        }
    }
    body[idx] = '\0';

    if (idx == 0) {
        return (key_event_t){.type = KEY_ESCAPE};
    }

    char final = body[idx - 1];
    int cb = 0;
    int cx = 0;
    int cy = 0;
    sscanf(body, "%d;%d;%d", &cb, &cx, &cy);

    key_event_t ev = {.type = KEY_MOUSE};
    ev.mouse_col = cx - 1;
    ev.mouse_row = cy - 1;

    int button_code = cb & 0x3;
    int is_motion = cb & 32;
    int is_wheel = cb & 64;

    if (is_wheel) {
        ev.mouse_button = (button_code == 0) ? MOUSE_SCROLL_UP : MOUSE_SCROLL_DOWN;
    } else if (button_code == 0) {
        if (final == 'm') {
            ev.mouse_button = MOUSE_LEFT_RELEASE;
        } else {
            ev.mouse_button = is_motion ? MOUSE_LEFT_DRAG : MOUSE_LEFT_PRESS;
        }
    } else {
        ev.mouse_button = MOUSE_NONE;
    }

    return ev;
}

static void paste_buf_append(char c)
{
    if (g_paste_len + 1 > g_paste_cap) {
        size_t new_cap = g_paste_cap == 0 ? 1024 : g_paste_cap * 2;
        g_paste_buf = xrealloc(g_paste_buf, new_cap);
        g_paste_cap = new_cap;
    }
    g_paste_buf[g_paste_len++] = c;
}

static key_event_t read_bracketed_paste(void)
{
    static const char terminator[5] = {'[', '2', '0', '1', '~'};
    g_paste_len = 0;

    for (;;) {
        char c;
        if (read_raw_byte(&c) != 0) {
            break;
        }
        if ((unsigned char)c != 0x1b) {
            paste_buf_append(c);
            continue;
        }

        char seq[5];
        size_t got = 0;
        for (; got < sizeof(seq); got++) {
            if (try_read_byte(&seq[got], 30) != 0) {
                break;
            }
        }
        if (got == sizeof(seq) && memcmp(seq, terminator, sizeof(terminator)) == 0) {
            break;
        }

        paste_buf_append(0x1b);
        for (size_t i = 0; i < got; i++) {
            paste_buf_append(seq[i]);
        }
    }

    return (key_event_t){.type = KEY_PASTE, .paste_text = g_paste_buf, .paste_len = g_paste_len};
}

static key_event_t read_csi_rest(char first_byte)
{
    char body[16];
    size_t idx = 0;
    char c = first_byte;
    for (;;) {
        if (idx + 1 >= sizeof(body)) {
            break;
        }
        body[idx++] = c;
        if (!((c >= '0' && c <= '9') || c == ';')) {
            break;
        }
        if (try_read_byte(&c, 30) != 0) {
            break;
        }
    }
    body[idx] = '\0';

    if (strcmp(body, "200~") == 0) {
        return read_bracketed_paste();
    }

    return decode_csi_body(body);
}

static key_event_t read_escape_sequence(void)
{
    char first;
    if (try_read_byte(&first, 30) != 0) {
        return (key_event_t){.type = KEY_ESCAPE};
    }

    if (first != '[' && first != 'O') {
        return (key_event_t){.type = KEY_ESCAPE};
    }

    char second;
    if (try_read_byte(&second, 30) != 0) {
        return (key_event_t){.type = KEY_ESCAPE};
    }

    if (first == '[' && second == '<') {
        return read_sgr_mouse_sequence();
    }

    return read_csi_rest(second);
}

static key_event_t decode_utf8_char(unsigned char lead)
{
    int extra;
    unsigned int cp;

    if ((lead & 0xE0) == 0xC0) {
        extra = 1;
        cp = lead & 0x1Fu;
    } else if ((lead & 0xF0) == 0xE0) {
        extra = 2;
        cp = lead & 0x0Fu;
    } else if ((lead & 0xF8) == 0xF0) {
        extra = 3;
        cp = lead & 0x07u;
    } else {
        return (key_event_t){.type = KEY_CHAR, .codepoint = lead};
    }

    for (int i = 0; i < extra; i++) {
        char cont;
        if (read_raw_byte(&cont) != 0) {
            return (key_event_t){.type = KEY_CHAR, .codepoint = lead};
        }
        cp = (cp << 6) | ((unsigned int)(unsigned char)cont & 0x3Fu);
    }

    return (key_event_t){.type = KEY_CHAR, .codepoint = cp};
}

key_event_t terminal_read_key(void)
{
    char c;
    if (read_raw_byte(&c) != 0) {
        return (key_event_t){.type = KEY_NONE};
    }
    unsigned char uc = (unsigned char)c;

    if (uc == 0x1b) {
        return read_escape_sequence();
    }
    if (uc == 0x7f || uc == 0x08) {
        return (key_event_t){.type = KEY_BACKSPACE};
    }
    if (uc == '\r' || uc == '\n') {
        return (key_event_t){.type = KEY_ENTER};
    }
    if (uc == '\t') {
        return (key_event_t){.type = KEY_TAB};
    }
    if (uc < 0x20) {
        return (key_event_t){.type = KEY_CHAR, .codepoint = uc, .mods = MOD_CTRL};
    }
    if (uc < 0x80) {
        return (key_event_t){.type = KEY_CHAR, .codepoint = uc};
    }
    return decode_utf8_char(uc);
}

static const char k_base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                      "abcdefghijklmnopqrstuvwxyz"
                                      "0123456789+/";

static size_t base64_encode(const unsigned char *data, size_t len, char *out)
{
    size_t out_len = 0;
    size_t i = 0;

    for (; i + 3 <= len; i += 3) {
        unsigned int n = ((unsigned int)data[i] << 16) | ((unsigned int)data[i + 1] << 8) |
                          (unsigned int)data[i + 2];
        out[out_len++] = k_base64_chars[(n >> 18) & 0x3F];
        out[out_len++] = k_base64_chars[(n >> 12) & 0x3F];
        out[out_len++] = k_base64_chars[(n >> 6) & 0x3F];
        out[out_len++] = k_base64_chars[n & 0x3F];
    }

    size_t rem = len - i;
    if (rem == 1) {
        unsigned int n = (unsigned int)data[i] << 16;
        out[out_len++] = k_base64_chars[(n >> 18) & 0x3F];
        out[out_len++] = k_base64_chars[(n >> 12) & 0x3F];
        out[out_len++] = '=';
        out[out_len++] = '=';
    } else if (rem == 2) {
        unsigned int n = ((unsigned int)data[i] << 16) | ((unsigned int)data[i + 1] << 8);
        out[out_len++] = k_base64_chars[(n >> 18) & 0x3F];
        out[out_len++] = k_base64_chars[(n >> 12) & 0x3F];
        out[out_len++] = k_base64_chars[(n >> 6) & 0x3F];
        out[out_len++] = '=';
    }

    return out_len;
}

void terminal_set_clipboard(const char *text, size_t len)
{
    size_t cap = ((len + 2) / 3) * 4 + 1;
    char *encoded = xmalloc(cap);
    size_t encoded_len = base64_encode((const unsigned char *)text, len, encoded);

    write_str("\x1b]52;c;");
    ssize_t written = write(STDOUT_FILENO, encoded, encoded_len);
    (void)written;
    write_str("\x07");

    free(encoded);
}
