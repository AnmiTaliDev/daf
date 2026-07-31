/* SPDX-FileCopyrightText: AnmiTaliDev <anmitalidev@nuros.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef DAF_CONFIG_H
#define DAF_CONFIG_H

typedef struct {
    char theme_name[64]; /* empty if the config file didn't set one */
} daf_config_t;

/* Reads the user config file (key=value lines, '#' comments) if it exists.
 * `out` must be zero-initialized by the caller; a missing file, or one that
 * doesn't set a given key, simply leaves the matching field untouched. */
void config_load(daf_config_t *out);

/* Path to the config file, relative to $XDG_CONFIG_HOME (or $HOME/.config),
 * baked in at build time via `meson configure -Dconfig_subpath=...`. */
const char *config_subpath(void);

#endif /* DAF_CONFIG_H */
