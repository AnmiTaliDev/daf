/* SPDX-FileCopyrightText: AnmiTaliDev <anmitalidev@nuros.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "filetype.h"

#include <string.h>
#include <strings.h>

typedef struct {
    const char *extension;
    const char *name;
} filetype_entry_t;

static const filetype_entry_t k_filetypes[] = {
    {".c", "C"},
    {".h", "C"},
    {".cpp", "C++"},
    {".cc", "C++"},
    {".cxx", "C++"},
    {".hpp", "C++"},
    {".py", "Python"},
    {".sh", "Shell"},
    {".bash", "Shell"},
    {".md", "Markdown"},
    {".markdown", "Markdown"},
    {".json", "JSON"},
    {".js", "JavaScript"},
    {".ts", "TypeScript"},
    {".rs", "Rust"},
    {".go", "Go"},
    {".java", "Java"},
    {".rb", "Ruby"},
    {".html", "HTML"},
    {".htm", "HTML"},
    {".css", "CSS"},
    {".xml", "XML"},
    {".yaml", "YAML"},
    {".yml", "YAML"},
    {".toml", "TOML"},
    {".ini", "INI"},
    {".sql", "SQL"},
    {".lua", "Lua"},
    {".pl", "Perl"},
    {".php", "PHP"},
};

static const char *find_extension(const char *filename)
{
    const char *slash = strrchr(filename, '/');
    const char *basename = slash != NULL ? slash + 1 : filename;
    const char *dot = strrchr(basename, '.');
    if (dot == NULL || dot == basename) {
        return NULL;
    }
    return dot;
}

const char *filetype_detect(const char *filename)
{
    if (filename == NULL) {
        return "Plain Text";
    }

    const char *extension = find_extension(filename);
    if (extension == NULL) {
        return "Plain Text";
    }

    for (size_t i = 0; i < sizeof(k_filetypes) / sizeof(k_filetypes[0]); i++) {
        if (strcasecmp(extension, k_filetypes[i].extension) == 0) {
            return k_filetypes[i].name;
        }
    }
    return "Plain Text";
}
