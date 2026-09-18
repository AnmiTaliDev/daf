/* SPDX-FileCopyrightText: AnmiTaliDev <anmitalidev@nuros.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "syntax.h"

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tree_sitter/api.h>

#include "common.h"

struct syntax_highlighter_s {
    void *dl_handle;
    TSParser *parser;
    TSTree *tree;
    TSQuery *query;
    TSQueryCursor *cursor;
    syntax_role_t *capture_roles;
    uint32_t capture_count;
    size_t parsed_version;
};

static const char *filetype_to_lang_name(const char *filetype)
{
    if (filetype == NULL) {
        return NULL;
    }
    if (strcmp(filetype, "C") == 0) return "c";
    if (strcmp(filetype, "C++") == 0) return "cpp";
    if (strcmp(filetype, "Python") == 0) return "python";
    if (strcmp(filetype, "Shell") == 0) return "bash";
    if (strcmp(filetype, "Markdown") == 0) return "markdown";
    if (strcmp(filetype, "JSON") == 0) return "json";
    if (strcmp(filetype, "JavaScript") == 0) return "javascript";
    if (strcmp(filetype, "TypeScript") == 0) return "typescript";
    if (strcmp(filetype, "Rust") == 0) return "rust";
    if (strcmp(filetype, "Go") == 0) return "go";
    if (strcmp(filetype, "Java") == 0) return "java";
    if (strcmp(filetype, "Ruby") == 0) return "ruby";
    if (strcmp(filetype, "HTML") == 0) return "html";
    if (strcmp(filetype, "CSS") == 0) return "css";
    if (strcmp(filetype, "XML") == 0) return "xml";
    if (strcmp(filetype, "YAML") == 0) return "yaml";
    if (strcmp(filetype, "TOML") == 0) return "toml";
    if (strcmp(filetype, "Lua") == 0) return "lua";
    if (strcmp(filetype, "PHP") == 0) return "php";
    return NULL;
}

static void *load_grammar_library(const char *lang, const TSLanguage **out_lang)
{
    char libname[128];
    void *handle = NULL;

    snprintf(libname, sizeof(libname), "libtree-sitter-%s.so", lang);
    handle = dlopen(libname, RTLD_NOW | RTLD_LOCAL);
    if (handle == NULL) {
        snprintf(libname, sizeof(libname), "libtree-sitter-%s.so.0", lang);
        handle = dlopen(libname, RTLD_NOW | RTLD_LOCAL);
    }
    if (handle == NULL) {
        snprintf(libname, sizeof(libname), "tree-sitter-%s.so", lang);
        handle = dlopen(libname, RTLD_NOW | RTLD_LOCAL);
    }
    if (handle == NULL) {
        return NULL;
    }

    char symname[128];
    snprintf(symname, sizeof(symname), "tree_sitter_%s", lang);
    for (size_t i = 0; symname[i] != '\0'; i++) {
        if (symname[i] == '-') {
            symname[i] = '_';
        }
    }

    typedef const TSLanguage *(*lang_fn_t)(void);
    union {
        void *obj;
        lang_fn_t func;
    } sym;
    sym.obj = dlsym(handle, symname);
    if (sym.func == NULL) {
        dlclose(handle);
        return NULL;
    }

    *out_lang = sym.func();
    return handle;
}

static char *read_file_content(const char *path, uint32_t *out_len)
{
    FILE *fp = fopen(path, "rb");
    if (fp == NULL) {
        return NULL;
    }
    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return NULL;
    }
    long sz = ftell(fp);
    if (sz <= 0) {
        fclose(fp);
        return NULL;
    }
    rewind(fp);

    char *buf = xmalloc((size_t)sz + 1);
    size_t read_bytes = fread(buf, 1, (size_t)sz, fp);
    fclose(fp);
    buf[read_bytes] = '\0';
    if (out_len != NULL) {
        *out_len = (uint32_t)read_bytes;
    }
    return buf;
}

static char *find_and_read_query(const char *lang, uint32_t *out_len)
{
    char path[512];

    const char *xdg = getenv("XDG_CONFIG_HOME");
    if (xdg != NULL && xdg[0] != '\0') {
        snprintf(path, sizeof(path), "%s/daf/queries/%s/highlights.scm", xdg, lang);
        char *content = read_file_content(path, out_len);
        if (content != NULL) {
            return content;
        }
    }

    const char *home = getenv("HOME");
    if (home != NULL && home[0] != '\0') {
        snprintf(path, sizeof(path), "%s/.config/daf/queries/%s/highlights.scm", home, lang);
        char *content = read_file_content(path, out_len);
        if (content != NULL) {
            return content;
        }
    }

    snprintf(path, sizeof(path), "/usr/share/tree-sitter/queries/%s/highlights.scm", lang);
    char *content = read_file_content(path, out_len);
    if (content != NULL) {
        return content;
    }

    snprintf(path, sizeof(path), "/usr/share/nvim/runtime/queries/%s/highlights.scm", lang);
    return read_file_content(path, out_len);
}

static syntax_role_t capture_name_to_role(const char *name)
{
    if (strncmp(name, "keyword", 7) == 0 || strcmp(name, "repeat") == 0 ||
        strcmp(name, "conditional") == 0 || strcmp(name, "include") == 0) {
        return SYNTAX_ROLE_KEYWORD;
    }
    if (strncmp(name, "string", 6) == 0 || strcmp(name, "character") == 0) {
        return SYNTAX_ROLE_STRING;
    }
    if (strncmp(name, "comment", 7) == 0) {
        return SYNTAX_ROLE_COMMENT;
    }
    if (strncmp(name, "type", 4) == 0 || strcmp(name, "storageclass") == 0 ||
        strcmp(name, "structure") == 0) {
        return SYNTAX_ROLE_TYPE;
    }
    if (strncmp(name, "function", 8) == 0 || strncmp(name, "method", 6) == 0 ||
        strcmp(name, "constructor") == 0) {
        return SYNTAX_ROLE_FUNCTION;
    }
    if (strncmp(name, "number", 6) == 0 || strcmp(name, "float") == 0 ||
        strcmp(name, "boolean") == 0) {
        return SYNTAX_ROLE_NUMBER;
    }
    if (strncmp(name, "operator", 8) == 0) {
        return SYNTAX_ROLE_OPERATOR;
    }
    return SYNTAX_ROLE_NONE;
}

syntax_highlighter_t *syntax_create(const char *filetype)
{
    const char *lang = filetype_to_lang_name(filetype);
    if (lang == NULL) {
        return NULL;
    }

    const TSLanguage *ts_lang = NULL;
    void *handle = load_grammar_library(lang, &ts_lang);
    if (handle == NULL || ts_lang == NULL) {
        return NULL;
    }

    TSParser *parser = ts_parser_new();
    if (parser == NULL || !ts_parser_set_language(parser, ts_lang)) {
        if (parser != NULL) {
            ts_parser_delete(parser);
        }
        dlclose(handle);
        return NULL;
    }

    uint32_t query_len = 0;
    char *query_src = find_and_read_query(lang, &query_len);
    TSQuery *query = NULL;
    TSQueryCursor *cursor = NULL;
    syntax_role_t *roles = NULL;
    uint32_t capture_count = 0;

    if (query_src != NULL) {
        uint32_t err_offset = 0;
        TSQueryError err_type = TSQueryErrorNone;
        query = ts_query_new(ts_lang, query_src, query_len, &err_offset, &err_type);
        free(query_src);
        if (query != NULL) {
            cursor = ts_query_cursor_new();
            capture_count = ts_query_capture_count(query);
            roles = xmalloc(capture_count * sizeof(syntax_role_t));
            for (uint32_t i = 0; i < capture_count; i++) {
                uint32_t nlen = 0;
                const char *name = ts_query_capture_name_for_id(query, i, &nlen);
                roles[i] = capture_name_to_role(name);
            }
        }
    }

    syntax_highlighter_t *hl = xmalloc(sizeof(syntax_highlighter_t));
    hl->dl_handle = handle;
    hl->parser = parser;
    hl->tree = NULL;
    hl->query = query;
    hl->cursor = cursor;
    hl->capture_roles = roles;
    hl->capture_count = capture_count;
    hl->parsed_version = (size_t)-1;
    return hl;
}

void syntax_destroy(syntax_highlighter_t *hl)
{
    if (hl == NULL) {
        return;
    }
    if (hl->cursor != NULL) {
        ts_query_cursor_delete(hl->cursor);
    }
    if (hl->query != NULL) {
        ts_query_delete(hl->query);
    }
    if (hl->tree != NULL) {
        ts_tree_delete(hl->tree);
    }
    if (hl->parser != NULL) {
        ts_parser_delete(hl->parser);
    }
    free(hl->capture_roles);
    if (hl->dl_handle != NULL) {
        dlclose(hl->dl_handle);
    }
    free(hl);
}

static char *buffer_to_string(const buffer_t *buf, size_t *out_len)
{
    size_t total = 0;
    for (size_t i = 0; i < buf->num_lines; i++) {
        total += buf->lines[i].len + 1;
    }
    char *text = xmalloc(total + 1);
    size_t offset = 0;
    for (size_t i = 0; i < buf->num_lines; i++) {
        memcpy(text + offset, buf->lines[i].chars, buf->lines[i].len);
        offset += buf->lines[i].len;
        text[offset++] = '\n';
    }
    text[offset] = '\0';
    if (out_len != NULL) {
        *out_len = offset;
    }
    return text;
}

void syntax_update(syntax_highlighter_t *hl, const buffer_t *buf)
{
    if (hl == NULL || hl->parser == NULL) {
        return;
    }
    if (hl->parsed_version == buf->version && hl->tree != NULL) {
        return;
    }

    size_t text_len = 0;
    char *text = buffer_to_string(buf, &text_len);
    TSTree *new_tree = ts_parser_parse_string(hl->parser, NULL, text, (uint32_t)text_len);
    free(text);
    if (new_tree != NULL) {
        if (hl->tree != NULL) {
            ts_tree_delete(hl->tree);
        }
        hl->tree = new_tree;
        hl->parsed_version = buf->version;
    }
}

void syntax_get_line_roles(syntax_highlighter_t *hl, size_t row, const line_t *line, uint8_t *roles)
{
    if (line->len == 0) {
        return;
    }
    memset(roles, SYNTAX_ROLE_NONE, line->len);
    if (hl == NULL || hl->tree == NULL || hl->query == NULL || hl->cursor == NULL) {
        return;
    }

    TSNode root = ts_tree_root_node(hl->tree);
    TSPoint start_point = {(uint32_t)row, 0};
    TSPoint end_point = {(uint32_t)(row + 1), 0};
    ts_query_cursor_set_point_range(hl->cursor, start_point, end_point);
    ts_query_cursor_exec(hl->cursor, hl->query, root);

    TSQueryMatch match;
    uint32_t capture_index;
    while (ts_query_cursor_next_capture(hl->cursor, &match, &capture_index)) {
        const TSQueryCapture *capture = &match.captures[capture_index];
        if (capture->index >= hl->capture_count) {
            continue;
        }
        syntax_role_t role = hl->capture_roles[capture->index];
        if (role == SYNTAX_ROLE_NONE) {
            continue;
        }
        TSPoint p_start = ts_node_start_point(capture->node);
        TSPoint p_end = ts_node_end_point(capture->node);
        if (p_start.row > row || p_end.row < row) {
            continue;
        }
        size_t c_start = (p_start.row == row) ? p_start.column : 0;
        size_t c_end = (p_end.row == row) ? p_end.column : line->len;
        if (c_start > line->len) {
            c_start = line->len;
        }
        if (c_end > line->len) {
            c_end = line->len;
        }
        for (size_t c = c_start; c < c_end; c++) {
            roles[c] = (uint8_t)role;
        }
    }
}
