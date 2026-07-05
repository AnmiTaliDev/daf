/* SPDX-FileCopyrightText: AnmiTaliDev <anmitalidev@nuros.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef DAF_COMMON_H
#define DAF_COMMON_H

#include <stddef.h>

#define DAF_VERSION "0.2.0"
#define DAF_TAB_STOP 8

#define DAF_MIN(a, b) ((a) < (b) ? (a) : (b))
#define DAF_MAX(a, b) ((a) > (b) ? (a) : (b))

void *xmalloc(size_t size);
void *xrealloc(void *ptr, size_t size);
char *xstrdup(const char *s);

#endif /* DAF_COMMON_H */
