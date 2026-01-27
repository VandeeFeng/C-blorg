#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "site-builder/site-builder.h"
#include "org-ffi.h"

static const char *WEEKDAYS[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
static const char *MONTHS[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

static char *format_rfc2822_date(int year, int month, int day, int hour, int minute) {
    struct tm tm = {
        .tm_year = year - 1900,
        .tm_mon = month - 1,
        .tm_mday = day,
        .tm_hour = hour,
        .tm_min = minute,
        .tm_sec = 0
    };

    time_t t = mktime(&tm);
    struct tm *gmt = gmtime(&t);

    char *rfc_date = malloc(64);
    if (!rfc_date) return NULL;

    snprintf(rfc_date, 64, "%s, %02d %s %d %02d:%02d:00 +0800",
             WEEKDAYS[gmt->tm_wday], day, MONTHS[month - 1], year, hour, minute);

    return rfc_date;
}

static char *convert_to_rfc2822(const char *org_date) {
    if (!org_date || strlen(org_date) < 11) {
        return strdup("");
    }

    int year = 0, month = 0, day = 0, hour = 0, minute = 0;

    if (sscanf(org_date, "<%d-%d-%d %d:%d>", &year, &month, &day, &hour, &minute) != 5) {
        return strdup("");
    }

    return format_rfc2822_date(year, month, day, hour, minute);
}

static char *xml_escape(const char *input) {
    if (!input) return strdup("");

    size_t len = strlen(input);
    size_t escaped_len = 0;

    for (size_t i = 0; i < len; i++) {
        switch (input[i]) {
            case '&': escaped_len += 5; break;
            case '<': escaped_len += 4; break;
            case '>': escaped_len += 4; break;
            case '\'': escaped_len += 6; break;
            case '"': escaped_len += 6; break;
            default: escaped_len += 1; break;
        }
    }

    char *escaped = malloc(escaped_len + 1);
    if (!escaped) return strdup("");

    size_t pos = 0;
    for (size_t i = 0; i < len; i++) {
        switch (input[i]) {
            case '&':
                strcpy(escaped + pos, "&amp;");
                pos += 5;
                break;
            case '<':
                strcpy(escaped + pos, "&lt;");
                pos += 4;
                break;
            case '>':
                strcpy(escaped + pos, "&gt;");
                pos += 4;
                break;
            case '\'':
                strcpy(escaped + pos, "&apos;");
                pos += 6;
                break;
            case '"':
                strcpy(escaped + pos, "&quot;");
                pos += 6;
                break;
            default:
                escaped[pos++] = input[i];
                break;
        }
    }
    escaped[pos] = '\0';
    return escaped;
}

static char *read_org_file(const char *path, size_t *out_size) {
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "ERROR: Failed to open %s\n", path);
        *out_size = 0;
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *content = malloc(file_size + 1);
    if (!content) {
        fprintf(stderr, "ERROR: Failed to allocate memory for %s\n", path);
        fclose(f);
        *out_size = 0;
        return NULL;
    }

    *out_size = fread(content, 1, file_size, f);
    content[*out_size] = '\0';
    fclose(f);
    return content;
}

typedef void (*TagCallback)(FILE *fp, const char *tag, const char *base_url, void *data);

static void process_tags(const char *tags_str, TagCallback callback, FILE *fp, const char *base_url, void *data) {
    if (!tags_str || strlen(tags_str) == 0) return;

    char *tags_copy = strdup(tags_str);
    if (!tags_copy) return;

    char *tag = strtok(tags_copy, " ");
    while (tag != NULL) {
        callback(fp, tag, base_url, data);
        tag = strtok(NULL, " ");
    }
    free(tags_copy);
}

static void write_description_tag(FILE *fp, const char *tag, const char *base_url, void *data) {
    int *first = (int *)data;
    if (!*first) {
        fprintf(fp, ", ");
    }
    *first = 0;
    fprintf(fp, "<a href=\"%stags/%s.html\">%s</a>", base_url, tag, tag);
}

static void write_category_element(FILE *fp, const char *tag, const char *base_url, void *data) {
    (void)base_url;
    (void)data;
    char *escaped = xml_escape(tag);
    fprintf(fp, "  <category><![CDATA[%s]]></category>\n", escaped);
    free(escaped);
}

int generate_rss_feed(SiteBuilder *builder) {
    if (builder->post_count == 0) {
        printf("Warning: No posts to generate RSS feed\n");
        return 0;
    }

    printf("\nGenerating RSS feed...\n");

    int max_items = builder->post_count < builder->max_rss_items ? builder->post_count : builder->max_rss_items;
    char *rss_path = join_path(builder->output_dir, "rss.xml");

    FILE *fp = fopen(rss_path, "w");
    if (!fp) {
        fprintf(stderr, "ERROR: Failed to create RSS file: %s\n", rss_path);
        free(rss_path);
        return 1;
    }

    fprintf(fp, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
    fprintf(fp, "<rss version=\"2.0\" xmlns:atom=\"http://www.w3.org/2005/Atom\">\n");
    fprintf(fp, "<channel>\n");

    char *escaped_title = xml_escape(builder->site_title);
    fprintf(fp, "<title><![CDATA[%s]]></title>\n", escaped_title);
    free(escaped_title);

    fprintf(fp, "<description><![CDATA[%s]]></description>\n", builder->site_title);
    fprintf(fp, "<link>%s</link>\n", builder->blog_base_url);

    time_t now = time(NULL);
    struct tm *tm_now = gmtime(&now);
    char last_build_date[64];
    snprintf(last_build_date, 64, "%s, %02d %s %d %02d:%02d:00 +0800",
             WEEKDAYS[tm_now->tm_wday], tm_now->tm_mday, MONTHS[tm_now->tm_mon], tm_now->tm_year + 1900,
             tm_now->tm_hour, tm_now->tm_min);
    fprintf(fp, "<lastBuildDate>%s</lastBuildDate>\n", last_build_date);

    for (int i = 0; i < max_items; i++) {
        PostInfo *post = &builder->posts[i];

        char *rfc_date = convert_to_rfc2822(post->raw_date);

        size_t url_len = strlen(builder->blog_base_url) + strlen(post->filename) + 6;
        char *post_url = malloc(url_len);
        snprintf(post_url, url_len, "%s%s.html", builder->blog_base_url, post->filename);

        fprintf(fp, "<item>\n");

        escaped_title = xml_escape(post->title);
        fprintf(fp, "  <title><![CDATA[%s]]></title>\n", escaped_title);
        free(escaped_title);

        fprintf(fp, "  <description><![CDATA[");

        size_t org_path_len = strlen(builder->input_dir) + strlen(post->filename) + 6;
        char *org_path = malloc(org_path_len);
        snprintf(org_path, org_path_len, "%s/%s.org", builder->input_dir, post->filename);

        size_t content_size = 0;
        char *content = read_org_file(org_path, &content_size);
        if (content) {
            char *html = org_parse_to_html(content, content_size);
            if (html) {
                fprintf(fp, "%s", html);
                org_free_string(html);
            }
            free(content);
        }
        free(org_path);

        if (post->tags && strlen(post->tags) > 0) {
            fprintf(fp, "<div class=\"taglist\"><a href=\"%stags.html\">Tags</a>: ", builder->blog_base_url);
            int first = 1;
            process_tags(post->tags, write_description_tag, fp, builder->blog_base_url, &first);
            fprintf(fp, " </div>");
        }

        fprintf(fp, "]]></description>\n");

        process_tags(post->tags, write_category_element, fp, builder->blog_base_url, NULL);

        fprintf(fp, "  <link>%s</link>\n", post_url);
        fprintf(fp, "  <guid>%s</guid>\n", post_url);
        fprintf(fp, "  <pubDate>%s</pubDate>\n", rfc_date);
        fprintf(fp, "</item>\n");

        free(rfc_date);
        free(post_url);
    }

    fprintf(fp, "</channel>\n");
    fprintf(fp, "</rss>\n");
    fclose(fp);

    printf("  RSS feed generated: %s\n", rss_path);
    printf("  Total items: %d (limit: %d)\n", max_items, builder->max_rss_items);

    free(rss_path);

    return 0;
}
