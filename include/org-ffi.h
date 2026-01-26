#ifndef ORG_FFI_H
#define ORG_FFI_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Opaque types */
typedef struct OrgDocument OrgDocument;
typedef struct OrgMetadata OrgMetadata;

/**
 * Parse org-mode content and return HTML string.
 *
 * @param input Null-terminated org-mode content string
 * @param len Length of input string (excluding null terminator)
 * @return Heap-allocated null-terminated HTML string, or NULL on error
 *
 * The returned string must be freed using org_free_string().
 */
char* org_parse_to_html(const char* input, size_t len);

/**
 * Extract metadata from org-mode content.
 *
 * @param input Null-terminated org-mode content string
 * @param len Length of input string (excluding null terminator)
 * @return Pointer to OrgMetadata struct, or NULL on error
 *
 * The returned metadata must be freed using org_free_metadata().
 */
OrgMetadata* org_extract_metadata(const char* input, size_t len);

/**
 * Free a string allocated by org_parse_to_html() or metadata fields.
 *
 * @param s Pointer to string to free (can be NULL)
 */
void org_free_string(char* s);

/**
 * Free metadata struct allocated by org_extract_metadata().
 *
 * This also frees all strings within the metadata struct.
 *
 * @param meta Pointer to metadata to free (can be NULL)
 */
void org_free_metadata(OrgMetadata* meta);

/* Metadata getters */

/**
 * Get title from metadata.
 *
 * @param meta Metadata struct pointer
 * @return Null-terminated string, or NULL if not found.
 *         The string is owned by metadata, do not free.
 */
const char* org_meta_get_title(const OrgMetadata* meta);

/**
 * Get date from metadata.
 *
 * @param meta Metadata struct pointer
 * @return Null-terminated string, or NULL if not found.
 *         The string is owned by metadata, do not free.
 */
const char* org_meta_get_date(const OrgMetadata* meta);

/**
 * Get description from metadata.
 *
 * @param meta Metadata struct pointer
 * @return Null-terminated string, or NULL if not found.
 *         The string is owned by metadata, do not free.
 */
const char* org_meta_get_description(const OrgMetadata* meta);

/**
 * Get tags from metadata.
 *
 * @param meta Metadata struct pointer
 * @return Null-terminated string (space-separated tags), or NULL if not found.
 *         The string is owned by metadata, do not free.
 */
const char* org_meta_get_tags(const OrgMetadata* meta);

/**
 * Get tags as array of strings.
 *
 * @param meta Metadata struct pointer
 * @param out_count Output parameter for number of tags
 * @return Array of null-terminated strings, or NULL if no tags.
 *         The array is owned by metadata, do not free.
 *         Individual strings are owned by metadata, do not free.
 */
const char** org_meta_get_tags_array(const OrgMetadata* meta, size_t* out_count);

#ifdef __cplusplus
}
#endif

#endif /* ORG_FFI_H */
