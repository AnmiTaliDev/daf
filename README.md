# Daf

[![CI](https://github.com/AnmiTaliDev/daf/actions/workflows/ci.yml/badge.svg)](https://github.com/AnmiTaliDev/daf/actions/workflows/ci.yml)

Daf (Hebrew `דף` — "sheet/page") is a simple console text editor written in C.

## Planned Features

Theming, and similar features are planned for future versions.

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
| Ctrl-F                               | Find (Enter confirms, Ctrl-F finds next, Esc cancels) |
| Ctrl-G                               | Jump to a line number (Enter jumps, Esc cancels) |
| Ctrl-Z / Ctrl-Y                      | Undo / redo                                      |
| Ctrl-C / Ctrl-X                      | Copy / cut the selection (also pushed to the system clipboard) |
| Ctrl-V                               | Paste the internal clipboard                     |
| Backspace / Delete                   | Delete a character or the selection              |
| Esc                                  | Clear the selection                              |

## License

GNU GPL 3.0.
