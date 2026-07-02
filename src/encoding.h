/* SPDX-FileCopyrightText: AnmiTaliDev <anmitalidev@nuros.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef DAF_ENCODING_H
#define DAF_ENCODING_H

#include <stddef.h>

typedef enum {
    ENCODING_ASCII,
    ENCODING_UTF8,
    ENCODING_UTF8_BOM,
    ENCODING_UNKNOWN,
} encoding_t;

encoding_t encoding_detect(const unsigned char *data, size_t len);
const char *encoding_name(encoding_t encoding);

#endif /* DAF_ENCODING_H */
