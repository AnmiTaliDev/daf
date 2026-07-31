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
} theme_t;

const theme_t *theme_find(const char *name);

const char *theme_build_default_name(void);

#endif /* DAF_THEME_H */
