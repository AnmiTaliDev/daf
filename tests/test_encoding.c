/* SPDX-FileCopyrightText: AnmiTaliDev <anmitalidev@nuros.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <assert.h>

#include "encoding.h"

int main(void)
{
    const unsigned char ascii[] = "hello world";
    assert(encoding_detect(ascii, sizeof(ascii) - 1) == ENCODING_ASCII);

    const unsigned char utf8[] = {'h', 'i', ' ', 0xD0, 0xBF, 0xD1, 0x80};
    assert(encoding_detect(utf8, sizeof(utf8)) == ENCODING_UTF8);

    const unsigned char bom[] = {0xEF, 0xBB, 0xBF, 'h', 'i'};
    assert(encoding_detect(bom, sizeof(bom)) == ENCODING_UTF8_BOM);

    const unsigned char invalid[] = {0xFF, 0xFE};
    assert(encoding_detect(invalid, sizeof(invalid)) == ENCODING_UNKNOWN);

    return 0;
}
