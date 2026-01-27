# Site Builder

Modular static site generator with org-mode support. Owns directory scanning, org parsing, HTML rendering, post management, and tag generation.

## Module Structure

- **site-builder.h** - Core types (`SiteBuilder`, `PostInfo`) and public API
- **filesystem.c/h** - File I/O, directory traversal, and asset copying
- **org-parser.c/h** - Org file reading, metadata extraction, HTML conversion
- **page-renderer.c/h** - Template loading, variable injection, HTML output
- **post-management.c/h** - Post collection, sorting, index/archive pages
- **tag-pages.c/h** - Tag grouping, tag index, individual tag pages

## Contracts & Invariants

- All file paths use `join_path()` - never concatenate strings manually
- String buffers allocated with `string_create()` - free with `string_free()`
- Template vars set with `template_set_var()` - must free char* after use
- Post info stored in `builder->posts[]` - grow capacity via `add_post_to_builder()`

## Patterns

**Processing flow:**
1. `process_directory()` scans input dir recursively
2. `process_org_file()` reads org, parses HTML, extracts metadata
3. `render_post_page()` applies post template, then base template
4. After all posts: `generate_index_page()`, `generate_archive_page()`, `generate_tags_page()`, `generate_individual_tag_pages()`

**Memory management:**
- `free_org_file_resources()` cleans up after each org file
- `copy_template_assets()` copies template/custom/ to output dir
- RSS generation in `src/rss.c` (separate file, uses SiteBuilder)

## Anti-patterns

- Never bypass `process_directory()` for file scanning
- Don't call org FFI directly - use org-parser layer
- Don't inline template code - use template system
- Don't hardcode paths - use `join_path()`

## Related Context

- Root: `../AGENTS.md` (global invariants)
- Rust FFI: `../../ffi/AGENTS.md` (org parsing via orgize)
- Template system: `../template.h` (variable substitution)
