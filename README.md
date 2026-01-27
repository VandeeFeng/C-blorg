# Org-Mode Static Site Generator (C + Rust)

A custom static site generator using C and Rust that parses org-mode documents and renders HTML, replicating the current blog design.

[bastibe/org-static-blog: A static site generator using org-mode](https://github.com/bastibe/org-static-blog) is convenient enough, but the generated page structure is fixed and it depends on `org export`.

I want to build a more flexible template system that doesn't depend on Emacs.

This project uses [orgize](https://github.com/PoiScript/orgize) to replace `org export`.

Like Hugo, this project includes a [templates/](templates/) directory, making it easy to customize templates. Just HTML and JavaScript，no more framework.

This project using [nob.h](https://github.com/tsoding/nob.h) to build.

## Quick start

```bash
cc nob.c -o nob
./nob blog
```

Options:
- `-o` - Output directory for generated HTML (default: `blog`)
- `-c` - Content directory containing org-mode files (default: `posts`)
- `-t` - Directory containing HTML templates (default: `templates`)

```bash
./nob blog [-o output_dir] [-c content_dir] [-t template_dir]
```

Other commands:

```bash
# Build project (default target)
./nob

# Clean build artifacts
./nob clean

# Run tests
./nob test

# Run blog builder
./nob blog
```

## Project Structure

```
C-blorg--rust/
 ├── nob.c                # Build script using nob.h
 ├── nob.h                # Header-only build system
 ├── src/
 │   ├── main.c           # Entry point
 │   ├── org-string.h/c   # Custom SDS-style dynamic string
 │   ├── template.h/c     # HTML template system with variable substitution
 │   ├── site-builder/    # Site building logic and file processing
 │   ├── tokenizer.h/c    # Org-mode tokenizer
 │   ├── parser.h/c       # Org-mode parser (AST generation)
 │   └── render.h/c       # HTML renderer from AST
 ├── include/
 │   └── org-ffi.h        # Generated Rust FFI header
 ├── ffi/
 │   ├── Cargo.toml
 │   └── src/lib.rs       # FFI wrapper around orgize library
 ├── templates/           # HTML templates
 ├── test/
```

## Features Supported

- Org-mode metadata (title, date, description, tags)
- Headings (up to 6 levels)
- Text formatting (bold, italic, code, strikethrough)
- Code blocks with language specification
- Blockquotes
- Lists (ordered and unordered)
- Links
- Images with attributes
- HTML template rendering

## Architecture

The project uses a hybrid C + Rust architecture:
- **Rust FFI Layer** (`ffi/src/lib.rs`):
  - Wraps the [orgize](https://github.com/PoiScript/orgize) library
  - Exposes C-compatible functions via `extern "C"`
  - Handles org-mode parsing and HTML generation
  - Extracts metadata (title, date, tags, description)

- **C Site Builder** (`src/site-builder.c`):
  - Orchestrates the build process
  - Processes org-mode files recursively
  - Extracts metadata and HTML from parser/renderer
  - Renders HTML using templates
  - Generates index, archive, and tag pages
  - Copies template assets (404, projects, static files)

- **Template System** (`src/template.c`):
  - Simple variable substitution `{{variable}}`
  - Partial inclusion `{{include filename}}`
  - Reusable header, footer, head components

- **String Utilities** (`src/org-string.c`):
  - Custom SDS-style dynamic strings with exponential growth
  - Memory-efficient string operations

## Related projects

- **nob.h**: https://github.com/tsoding/nob.h (Header-only C build system)
- **orgize**: https://github.com/PoiScript/orgize (Rust org-mode parser library)
- **SDS (Redis strings)**: https://github.com/antirez/sds (String implementation reference)

