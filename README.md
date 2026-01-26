# Org-Mode Static Site Generator (C + Rust)

A custom static site generator using C and Rust that parses org-mode documents and renders HTML, replicating the current blog design.

## Project Structure

```
C-blorg--rust/
 ├── nob.c                # Build script using nob.h
 ├── nob.h                # Header-only build system
 ├── src/
 │   ├── org-string.h/c   # Custom SDS-style dynamic string
 │   ├── template.h/c     # HTML template system with variable substitution
 │   ├── site-builder.h/c # Site building logic and file processing
 │   ├── main.c           # Entry point
 ├── include/
 │   └── org-ffi.h       # Generated Rust FFI header
 ├── ffi/
 │   ├── Cargo.toml        # Rust package definition with orgize dependency
 │   └── src/lib.rs       # FFI wrapper around orgize library
 ├── templates/            # HTML templates
 ├── test/
 │   ├── test_string.c     # String utilities tests
 │   ├── test_template.c   # Template system tests
 │   ├── test_ffi.c       # Rust FFI tests
 │   └── test_issues.org   # Test org-mode files
 └── Cargo.lock           # Rust dependency lock file
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

# Run executable
./nob run
```

## Usage

```bash
./orgblog [-o output_dir] [-c content_dir] [-t template_dir]
```

Options:
- `-o` - Output directory for generated HTML (default: `output`)
- `-c` - Content directory containing org-mode files (default: `content`)
- `-t` - Directory containing HTML templates (default: `templates`)

Examples:
```bash
# Use all defaults
./orgblog

# Custom output directory
./orgblog -o public

# Custom content and output directories
./orgblog -c posts -o build

# Full customization
./orgblog -o public -c content -t templates
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
  - Extracts metadata and HTML from Rust FFI
  - Renders HTML using templates
  - Generates index, archive, and tag pages

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

