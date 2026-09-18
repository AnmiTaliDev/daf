/* SPDX-FileCopyrightText: AnmiTaliDev <anmitalidev@nuros.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef DAF_SYNTAX_H
#define DAF_SYNTAX_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "buffer.h"

typedef enum {
    SYNTAX_ROLE_NONE = 0,
    SYNTAX_ROLE_KEYWORD,
    SYNTAX_ROLE_STRING,
    SYNTAX_ROLE_COMMENT,
    SYNTAX_ROLE_TYPE,
    SYNTAX_ROLE_FUNCTION,
    SYNTAX_ROLE_NUMBER,
    SYNTAX_ROLE_OPERATOR,
} syntax_role_t;

typedef struct syntax_highlighter_s syntax_highlighter_t;

syntax_highlighter_t *syntax_create(const char *filetype);
void syntax_destroy(syntax_highlighter_t *hl);

void syntax_update(syntax_highlighter_t *hl, const buffer_t *buf);
void syntax_get_line_roles(syntax_highlighter_t *hl, size_t row, const line_t *line, uint8_t *roles);

#endif
