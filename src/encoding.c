/* SPDX-FileCopyrightText: AnmiTaliDev <anmitalidev@nuros.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "encoding.h"

static int utf8_sequence_len(unsigned char lead)
{
    if ((lead & 0x80) == 0x00) {
        return 1;
    }
    if ((lead & 0xE0) == 0xC0) {
        return 2;
    }
    if ((lead & 0xF0) == 0xE0) {
        return 3;
    }
    if ((lead & 0xF8) == 0xF0) {
        return 4;
    }
    return 0;
}

encoding_t encoding_detect(const unsigned char *data, size_t len)
{
    size_t i = 0;
    int has_multibyte = 0;

    if (len >= 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF) {
        return ENCODING_UTF8_BOM;
    }

    while (i < len) {
        int seq_len = utf8_sequence_len(data[i]);
        if (seq_len == 0) {
            return ENCODING_UNKNOWN;
        }
        if (seq_len > 1) {
            has_multibyte = 1;
        }
        if (i + (size_t)seq_len > len) {
            return ENCODING_UNKNOWN;
        }
        for (int j = 1; j < seq_len; j++) {
            if ((data[i + (size_t)j] & 0xC0) != 0x80) {
                return ENCODING_UNKNOWN;
            }
        }
        i += (size_t)seq_len;
    }

    return has_multibyte ? ENCODING_UTF8 : ENCODING_ASCII;
}

const char *encoding_name(encoding_t encoding)
{
    switch (encoding) {
    case ENCODING_ASCII:
        return "ASCII";
    case ENCODING_UTF8:
        return "UTF-8";
    case ENCODING_UTF8_BOM:
        return "UTF-8 BOM";
    case ENCODING_UNKNOWN:
    default:
        return "Unknown";
    }
}
