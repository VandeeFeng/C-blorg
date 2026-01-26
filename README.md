# Org-Mode Static Site Generator (C Implementation)

A custom static site generator in C that parses org-mode documents and renders HTML, replicating the current blog design.

## Project Structure

```
orgblog/
├── build.c          # Build script using nob.h
├── nob.h            # Header-only build system
├── src/
│   ├── string.h/c   # Custom SDS-style dynamic string
│   ├── tokenizer.h/c # Org-mode tokenizer
│   ├── parser.h/c   # AST parser
│   ├── render.h/c   # HTML renderer
│   └── main.c       # Entry point
├── templates/       # HTML templates
├── test/
    └── test_string.c # String utilities tests
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

- **Tokenizer**: Reads org-mode files and produces tokens
- **Parser**: Builds AST from tokens
- **Renderer**: Generates HTML from AST
- **String Builder**: Custom SDS-style dynamic strings with exponential growth

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
- nob.h: https://github.com/tsoding/nob.h
- SDS (Redis strings): https://github.com/antirez/sds
