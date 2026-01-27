#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "site-builder.h"

int main(int argc, char **argv) {
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    printf("Org-Mode Static Site Generator v1.0\n");
    printf("===================================\n\n");

    char *input_dir = "posts";
    char *output_dir = "blog";
    char *template_dir = "templates";
    char *site_title = "My Blog";

    int opt;
    while ((opt = getopt(argc, argv, "o:c:t:")) != -1) {
        switch (opt) {
        case 'o': output_dir = optarg; break;
        case 'c': input_dir = optarg; break;
        case 't': template_dir = optarg; break;
        default:
            fprintf(stderr, "Usage: %s [-o output_dir] [-c content_dir] [-t template_dir]\n", argv[0]);
            return 1;
        }
    }

    SiteBuilder builder = {
        .input_dir = input_dir,
        .output_dir = output_dir,
        .template_dir = template_dir,
        .site_title = site_title,
        .content = NULL,
        .posts = NULL,
        .post_count = 0,
        .post_capacity = 0
    };

    printf("Input directory:  %s\n", builder.input_dir);
    printf("Output directory: %s\n", builder.output_dir);
    printf("Template directory: %s\n", builder.template_dir);
    printf("Site title: %s\n", builder.site_title);

    mkdir_p(builder.output_dir);

    int errors = process_directory(&builder, builder.input_dir, builder.output_dir);

    if (errors > 0) {
        printf("\nWARNING: %d errors occurred during build\n", errors);
        return 1;
    }

    printf("\nGenerating index, tags, individual tag pages, and archive pages...\n");
    generate_index_page(&builder);
    generate_tags_page(&builder);
    generate_individual_tag_pages(&builder);
    generate_archive_page(&builder);

    printf("\nCopying template assets...\n");
    int copy_errors = copy_template_assets(&builder);
    if (copy_errors > 0) {
        printf("\nWARNING: %d errors occurred during asset copying\n", copy_errors);
    }

    printf("\nBuild complete!\n");
    return 0;
}
