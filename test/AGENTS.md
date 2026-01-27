# Tests

## Purpose
Validate the C org parser, renderer, templates, tokenizer, and FFI integration.

## Entry Points
- `test/test_parser.c`
- `test/test_render.c`
- `test/test_template.c`
- `test/test_tokenizer.c`
- `test/test_string.c`
- `test/test_page_structure.c`
- `test/test_ffi.c`
- Fixtures: `test/test_issues.org`

## Contracts & Invariants
- Tests must be deterministic and leave no artifacts outside `/tmp/`.
- FFI allocations returned to C must be freed using the matching free functions.

## Patterns
- Prefer narrow, single-purpose test cases per file.
