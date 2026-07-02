/* SPDX-FileCopyrightText: AnmiTaliDev <anmitalidev@nuros.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef DAF_SEARCH_H
#define DAF_SEARCH_H

#include <stdbool.h>
#include <stddef.h>

#include "buffer.h"

bool search_find_forward(const buffer_t *buf, size_t start_row, size_t start_col,
                          const char *query, size_t *out_row, size_t *out_col);

#endif /* DAF_SEARCH_H */
