/* SPDX-FileCopyrightText: AnmiTaliDev <anmitalidev@nuros.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef DAF_THEME_H
#define DAF_THEME_H

typedef struct {
    const char *name;
    const char *normal;
    const char *gutter;
    const char *gutter_current;
    const char *selection;
    const char *search_match;
    const char *status_bar;
    const char *tilde;
    const char *reset;
    const char *syntax_keyword;
    const char *syntax_string;
    const char *syntax_comment;
    const char *syntax_type;
    const char *syntax_function;
    const char *syntax_number;
    const char *syntax_operator;
} theme_t;

const theme_t *theme_find(const char *name);

const theme_t *theme_load(const char *name);

const char *theme_build_default_name(void);

const char *theme_dir_subpath(void);

#endif /* DAF_THEME_H */
