#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include "site-builder/site-builder.h"

int main(int argc, char **argv) {
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    printf("Org-Mode Static Site Generator v1.0\n");
    printf("===================================\n\n");

    /* blog config */
    char *input_dir = "posts";
    char *output_dir = "blog";
    char *template_dir = "templates";
    char *site_title = "Vandee's Blog";
    char *blog_base_url = "https://www.vandee.art/blog/";
    bool show_index_description = true;

    int opt;
    while ((opt = getopt(argc, argv, "o:c:t:d:")) != -1) {
        switch (opt) {
        case 'o': output_dir = optarg; break;
        case 'c': input_dir = optarg; break;
        case 't': template_dir = optarg; break;
        case 'd': show_index_description = (strcmp(optarg, "true") == 0 || strcmp(optarg, "1") == 0); break;
        default:
            fprintf(stderr, "Usage: %s [-o output_dir] [-c content_dir] [-t template_dir] [-d show_index_description (true/false, default true)]\n", argv[0]);
            return 1;
        }
    }

    SiteBuilder builder = {
        .input_dir = input_dir,
        .output_dir = output_dir,
        .template_dir = template_dir,
        .site_title = site_title,
        .blog_base_url = blog_base_url,
        .content = NULL,
        .posts = NULL,
        .post_count = 0,
        .post_capacity = 0,
        .max_rss_items = 30
    };

    printf("Input directory:  %s\n", builder.input_dir);
    printf("Output directory: %s\n", builder.output_dir);
    printf("Template directory: %s\n", builder.template_dir);
    printf("Site title: %s\n", builder.site_title);
    printf("Blog base URL: %s\n", builder.blog_base_url);

    mkdir_p(builder.output_dir);

    printf("\nGenerating blog posts pages...\n");
    int errors = process_directory(&builder, builder.input_dir, builder.output_dir);

    if (errors > 0) {
        printf("\nWARNING: %d errors occurred during build\n", errors);
        return 1;
    }

    printf("\nGenerating index, tags, individual tag pages, and archive pages...\n");
    generate_index_page(&builder, show_index_description);
    generate_tags_page(&builder);
    generate_individual_tag_pages(&builder);
    generate_archive_page(&builder);
    generate_rss_feed(&builder);

    printf("\nCopying template assets...\n");
    int copy_errors = copy_template_assets(&builder);
    if (copy_errors > 0) {
        printf("\nWARNING: %d errors occurred during asset copying\n", copy_errors);
    }

    printf("\nBuild complete!\n");
    return 0;
}
