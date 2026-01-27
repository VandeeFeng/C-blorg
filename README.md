# Org-Mode Static Site Generator (C + Rust)

A custom static site generator using C and Rust that parses org-mode documents and renders HTML, replicating the current blog design.

## Project Structure

```
C-blorg--rust/
 ├── nob.c                # Build script using nob.h
 ├── nob.h                # Header-only build system
 ├── src/
 │   ├── main.c           # Entry point
 │   ├── org-string.h/c   # Custom SDS-style dynamic string
 │   ├── template.h/c     # HTML template system with variable substitution
 │   ├── site-builder.h/c # Site building logic and file processing
 │   ├── tokenizer.h/c    # Org-mode tokenizer
 │   ├── parser.h/c       # Org-mode parser (AST generation)
 │   └── render.h/c       # HTML renderer from AST
 ├── include/
 │   └── org-ffi.h       # Generated Rust FFI header
 ├── ffi/
 │   ├── Cargo.toml
 │   └── src/lib.rs       # FFI wrapper around orgize library
 ├── templates/            # HTML templates
 ├── test/
```

## Building

```bash
# First, build the build script
cc nob.c -o nob

# Build project (default target)
./nob

# Clean build artifacts
./nob clean

# Run tests
./nob test

# Run blog builder
./nob blog
```

## Usage

```bash
./nob blog [-o output_dir] [-c content_dir] [-t template_dir]
```

Options:
- `-o` - Output directory for generated HTML (default: `blog`)
- `-c` - Content directory containing org-mode files (default: `posts`)
- `-t` - Directory containing HTML templates (default: `templates`)

Examples:
```bash
# Use all defaults
./nob blog

# Custom output directory
./nob blog -o public

# Custom content and output directories
./nob blog -c posts -o build

# Full customization
./nob blog -o public -c content -t templates
```

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

## Resources

- **nob.h**: https://github.com/tsoding/nob.h (Header-only C build system)
- **orgize**: https://github.com/PoiScript/orgize (Rust org-mode parser library)
- **SDS (Redis strings)**: https://github.com/antirez/sds (String implementation reference)

