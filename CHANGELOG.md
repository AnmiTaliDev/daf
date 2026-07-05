# Changelog

All notable changes to this project are documented in this file.

## [0.2.0]

### Added

- Undo and redo (Ctrl-Z / Ctrl-Y)
- Copy, cut, and paste (Ctrl-C / Ctrl-X / Ctrl-V) with an internal clipboard
- Best-effort system clipboard integration via OSC 52 on copy/cut
- Bracketed paste support, so a real terminal paste (Ctrl+Shift+V, middle
  click, terminal paste menu) inserts system clipboard text as a single
  undoable edit
- Status bar messages for copy/cut/paste actions

## [0.1.0]

### Added

- Micro-like keybindings (not vi-style)
- Text selection via Shift + arrows/Home/End/PageUp/PageDown
- Working Tab: insert a tab, or indent/dedent the selection
- Smart Home, like in micro
- Text search (Ctrl-F) and jump to line (Ctrl-G)
- Status bar: file name, modified indicator, cursor position, file type,
  encoding
- Line numbers and current line highlighting
- Mouse support: click to place the cursor, drag to select, wheel to scroll
- Meson build system and a GitHub Actions CI workflow with unit tests
