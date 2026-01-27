# C-blorg

## Purpose
Custom org-mode static site generator in C with a Rust FFI parser/exporter.

## Entry Points
- `nob.c` - Build script using nob.h. Run `./nob [command]`:
  - (no args) - Build project to `build/org-blog`
  - `clean` - Remove build/ and ffi/target/ directories
  - `test` - Build and run all test suites
  - `blog` - Build and run org-blog with args
- `src/main.c` - CLI entry and build orchestration
- `src/site-builder.c` - Directory processing and HTML generation
- `ffi/src/lib.rs` - Rust orgize wrapper and C ABI exports

## Intent Layer

**Before modifying code in a subdirectory, read its AGENTS.md first** to understand local patterns and invariants.

- **Rust FFI layer**: `ffi/AGENTS.md` - orgize-based parsing, HTML export, and C ABI bindings
- **Tests**: `test/AGENTS.md` - C test suites for parser, renderer, templates, strings, tokenizer, and FFI

### Global Invariants

- Any pointer returned by the Rust FFI must be released using the matching free function in `include/org-ffi.h`.
