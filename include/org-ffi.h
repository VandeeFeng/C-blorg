#ifndef ORG_FFI_H
#define ORG_FFI_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Opaque types */
typedef struct OrgDocument OrgDocument;
typedef struct OrgMetadata OrgMetadata;

/* Parse org-mode content and return HTML string.
 * input - Null-terminated org-mode content string
 * len - Length of input string (excluding null terminator)
 *
 * Returns heap-allocated null-terminated HTML string, or NULL on error.
 * The returned string must be freed using org_free_string(). */
char* org_parse_to_html(const char* input, size_t len);

/* Extract metadata from org-mode content.
 * input - Null-terminated org-mode content string
 * len - Length of input string (excluding null terminator)
 *
 * Returns pointer to OrgMetadata struct, or NULL on error.
 * The returned metadata must be freed using org_free_metadata(). */
OrgMetadata* org_extract_metadata(const char* input, size_t len);

/* Free a string allocated by org_parse_to_html() or metadata fields.
 * s - Pointer to string to free (can be NULL) */
void org_free_string(char* s);

/* Free metadata struct allocated by org_extract_metadata().
 * This also frees all strings within the metadata struct.
 * meta - Pointer to metadata to free (can be NULL) */
void org_free_metadata(OrgMetadata* meta);

/* Metadata getters */

/* Get title from metadata.
 * meta - Metadata struct pointer
 *
 * Returns null-terminated string, or NULL if not found.
 * The string is owned by metadata, do not free. */
const char* org_meta_get_title(const OrgMetadata* meta);

/* Get date from metadata.
 * meta - Metadata struct pointer
 *
 * Returns null-terminated string, or NULL if not found.
 * The string is owned by metadata, do not free. */
const char* org_meta_get_date(const OrgMetadata* meta);

/* Get description from metadata.
 * meta - Metadata struct pointer
 *
 * Returns null-terminated string, or NULL if not found.
 * The string is owned by metadata, do not free. */
const char* org_meta_get_description(const OrgMetadata* meta);

/* Get tags from metadata.
 * meta - Metadata struct pointer
 *
 * Returns null-terminated string (space-separated tags), or NULL if not found.
 * The string is owned by metadata, do not free. */
const char* org_meta_get_tags(const OrgMetadata* meta);

/* Get tags as array of strings.
 * meta - Metadata struct pointer
 * out_count - Output parameter for number of tags
 *
 * Returns array of null-terminated strings, or NULL if no tags.
 * The array is owned by metadata, do not free.
 * Individual strings are owned by metadata, do not free. */
const char** org_meta_get_tags_array(const OrgMetadata* meta, size_t* out_count);

/* Extract Table of Contents from org-mode content.
 * input - Null-terminated org-mode content string
 * len - Length of input string (excluding null terminator)
 *
 * Returns heap-allocated null-terminated HTML TOC string, or NULL on error.
 * The returned string must be freed using org_free_string(). */
char* org_extract_toc(const char* input, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* ORG_FFI_H */
