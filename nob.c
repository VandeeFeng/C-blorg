#define NOB_IMPLEMENTATION
#include "nob.h"

typedef struct {
    const char *name;
    const char *src;
} Source;

bool compile_source(const char *src, const char *output)
{
    Nob_Cmd cmd = {0};
    nob_cmd_append(&cmd, "cc", "-Wall", "-Wextra", "-pedantic", "-std=c99");
    nob_cmd_append(&cmd, "-I", "src");
    nob_cmd_append(&cmd, "-c", src, "-o", output);
    return nob_cmd_run(&cmd);
}

bool link_program(const char **objects, size_t object_count, const char *output)
{
    Nob_Cmd cmd = {0};
    nob_cmd_append(&cmd, "cc");
    for (size_t i = 0; i < object_count; ++i) {
        nob_cmd_append(&cmd, objects[i]);
    }
    nob_cmd_append(&cmd, "-o", output);
    return nob_cmd_run(&cmd);
}

bool compile_sources(const char **sources, const char **outputs, size_t count)
{
    for (size_t i = 0; i < count; ++i) {
        if (!compile_source(sources[i], outputs[i])) return false;
    }
    return true;
}

bool build_project(void)
{
    if (!nob_mkdir_if_not_exists("build")) return false;

    Source sources[] = {
        {"org-string", "src/org-string.c"},
        {"tokenizer", "src/tokenizer.c"},
        {"parser", "src/parser.c"},
        {"render", "src/render.c"},
        {"template", "src/template.c"},
        {"site-builder", "src/site-builder.c"},
        {"main", "src/main.c"},
    };

    size_t source_count = sizeof(sources) / sizeof(sources[0]);
    const char *objects[source_count];
    for (size_t i = 0; i < source_count; ++i) {
        char *output = nob_temp_sprintf("build/%s.o", sources[i].name);
        if (!compile_source(sources[i].src, output)) return false;
        objects[i] = output;
    }

    return link_program(objects, source_count, "build/org-blog");
}

bool build_test(const char *test_name, const char *test_source, const char **objects, size_t object_count)
{
    char *output = nob_temp_sprintf("build/%s", test_name);
    Nob_Cmd cmd = {0};
    nob_cmd_append(&cmd, "cc", "-Wall", "-Wextra", "-pedantic", "-std=c99", "-I", "src");
    nob_cmd_append(&cmd, test_source);
    for (size_t i = 0; i < object_count; ++i) {
        nob_cmd_append(&cmd, objects[i]);
    }
    nob_cmd_append(&cmd, "-o", output);
    if (!nob_cmd_run(&cmd)) return false;

    cmd.count = 0;
    nob_cmd_append(&cmd, output);
    return nob_cmd_run(&cmd);
}

int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);

    const char *program = nob_shift_args(&argc, &argv);

    if (argc == 0) {
        nob_log(INFO, "Building default target: build");
        if (!build_project()) return 1;
        nob_log(INFO, "Build complete: build/org-blog");
        return 0;
    }

    if (strcmp(argv[0], "clean") == 0) {
        nob_log(INFO, "Cleaning build directory");
        Nob_Cmd cmd = {0};
        nob_cmd_append(&cmd, "rm", "-rf", "build/");
        return nob_cmd_run(&cmd) ? 0 : 1;
    }

    if (strcmp(argv[0], "test") == 0) {
        nob_log(INFO, "Running tests");
        if (!nob_mkdir_if_not_exists("build")) return 1;

        const char *objects[] = {"build/org-string.o", "build/tokenizer.o", "build/parser.o", "build/render.o", "build/template.o"};
        const char *sources[] = {"src/org-string.c", "src/tokenizer.c", "src/parser.c", "src/render.c", "src/template.c"};

        size_t count = sizeof(objects) / sizeof(objects[0]);
        if (!compile_sources(sources, objects, count)) return 1;

        const char *test_sources[] = {"test/test_string.c", "test/test_tokenizer.c", "test/test_parser.c", "test/test_render.c", "test/test_template.c"};
        const char *test_deps_string[] = {objects[0]};
        const char *test_deps_tokenizer[] = {objects[0], objects[1]};
        const char *test_deps_parser[] = {objects[0], objects[1], objects[2]};
        const char *test_deps_render[] = {objects[0], objects[1], objects[2], objects[3]};
        const char *test_deps_template[] = {objects[0], objects[4]};

        if (!build_test("test_string", test_sources[0], test_deps_string, 1)) return 1;
        if (!build_test("test_tokenizer", test_sources[1], test_deps_tokenizer, 2)) return 1;
        if (!build_test("test_parser", test_sources[2], test_deps_parser, 3)) return 1;
        if (!build_test("test_render", test_sources[3], test_deps_render, 4)) return 1;
        if (!build_test("test_template", test_sources[4], test_deps_template, 2)) return 1;

        nob_log(INFO, "All tests passed");
        return 0;
    }

    if (strcmp(argv[0], "run") == 0) {
        Nob_Cmd cmd = {0};
        nob_cmd_append(&cmd, "./build/org-blog");
        return nob_cmd_run(&cmd) ? 0 : 1;
    }

    nob_log(ERROR, "Unknown command: %s", argv[0]);
    nob_log(INFO, "Usage: %s [build|clean|test|run]", program);
    return 1;
}
