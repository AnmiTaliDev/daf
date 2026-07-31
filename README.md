# Daf

[![CI](https://github.com/AnmiTaliDev/daf/actions/workflows/ci.yml/badge.svg)](https://github.com/AnmiTaliDev/daf/actions/workflows/ci.yml)

Daf (Hebrew `דף` — "sheet/page") is a simple console text editor written in C.

## Themes

Three built-in 256-color themes: `dark` (default), `light`, and `mono` (the
original reverse-video/underline look, for terminals or tastes that don't
want real colors). Resolved in this order:

1. `--theme=NAME` on the command line
2. the `DAF_THEME` environment variable
3. `theme=NAME` in the config file (`$XDG_CONFIG_HOME/daf/config`, or
   `~/.config/daf/config`) — `#` starts a comment
4. the build-time default (`meson configure -Ddefault_theme=NAME`)

The config file's own location can be changed at build time too, with
`meson configure -Dconfig_subpath=...`.

## Building

Built with [Meson](https://mesonbuild.com/):

```sh
meson setup build
ninja -C build
```

The binary is produced at `build/src/daf`.

Run the test suite with:

```sh
meson test -C build
```

## Usage

```sh
build/src/daf [file]
```

If no file is given, the editor opens an empty, unnamed buffer.

## Keybindings

| Keys                                | Action                                          |
|--------------------------------------|--------------------------------------------------|
| Arrows                               | Move the cursor                                  |
| Shift + arrows/Home/End/PgUp/PgDn    | Select text                                      |
| Home                                 | Smart jump to start of line                      |
| End                                  | Jump to end of line                              |
| Tab / Shift-Tab                      | Insert tab / indent or dedent the selection      |
| Ctrl-S                               | Save (prompts for a name if the file is new)     |
| Ctrl-Q                               | Quit (press again to discard unsaved changes)    |
| Ctrl-F                               | Find: all matches are highlighted with a count in the prompt; Ctrl-F again jumps to the next match (wraps around), Enter confirms, Esc cancels |
| Ctrl-G                               | Jump to a line number (Enter jumps, Esc cancels) |
| Ctrl-R                               | Search & replace: type the search text, Enter, then the replacement text, Enter to replace every match at once (Esc cancels either step) |
| Ctrl-Z / Ctrl-Y                      | Undo / redo                                      |
| Ctrl-C / Ctrl-X                      | Copy / cut the selection (also pushed to the system clipboard) |
| Ctrl-V                               | Paste the internal clipboard                     |
| Backspace / Delete                   | Delete a character or the selection              |
| Esc                                  | Clear the selection                              |

Enter also auto-indents: a new line inherits the current line's leading
whitespace.

## License

GNU GPL 3.0.
