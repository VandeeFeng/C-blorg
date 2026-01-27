#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "site-builder.h"

int mkdir_p(const char *path);
char *get_filename_without_ext(const char *filename);
char *join_path(const char *dir, const char *file);
int process_regular_file(SiteBuilder *builder, const char *input_path, const char *output_dir, const char *filename);
int process_directory(SiteBuilder *builder, const char *input_dir, const char *output_dir);
int copy_file(const char *src, const char *dst);
int copy_directory_recursive(const char *src_dir, const char *dst_dir);
int copy_template_assets(SiteBuilder *builder);

#endif
