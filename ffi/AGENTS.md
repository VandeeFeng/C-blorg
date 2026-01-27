# Rust FFI (orgize)

## Purpose
Expose orgize parsing and HTML/metadata export through a C ABI for the C site builder.

## Entry Points
- `ffi/src/lib.rs` - FFI exports and HTML exporter
- `../include/org-ffi.h` - C declarations used by C callers

## Contracts & Invariants
- `org_parse_to_html` returns a heap-allocated C string; caller frees with `org_free_string`.
- `org_extract_metadata` returns `OrgMetadata *`; caller frees with `org_free_metadata`.
- All FFI entry points return null on invalid input (null pointer or zero length).

## Patterns
- When adding a new export, declare `#[no_mangle] extern "C"` in `ffi/src/lib.rs`, mirror it in `../include/org-ffi.h`, and provide a free function for owned memory.

## Anti-patterns
- Do not return Rust-owned pointers without an explicit free function.

## Related Context
- C site builder usage: `../src/site-builder.c`
