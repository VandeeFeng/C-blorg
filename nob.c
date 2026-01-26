#define NOB_IMPLEMENTATION
#include "nob.h"

typedef struct {
    const char *name;
    const char *src;
} Source;

bool build_rust_ffi(void)
{
    Nob_Cmd cmd = {0};
    nob_cmd_append(&cmd, "cargo", "build", "--release", "--manifest-path", "ffi/Cargo.toml");
    return nob_cmd_run(&cmd);
}

bool compile_source(const char *src, const char *output)
{
    Nob_Cmd cmd = {0};
    nob_cc(&cmd);
    nob_cc_flags(&cmd);
    nob_cmd_append(&cmd, "-pedantic", "-std=c99");
    nob_cmd_append(&cmd, "-I", "src", "-I", "include", "-c", src, "-o", output);
    return nob_cmd_run(&cmd);
}

bool link_program(const char **objects, size_t object_count, const char *output)
{
    Nob_Cmd cmd = {0};
    nob_cc(&cmd);
    nob_cmd_append(&cmd, "-L", "ffi/target/release", "-Wl,-rpath,$ORIGIN/../ffi/target/release");
    nob_cmd_append(&cmd, "-l", "org_ffi", "-l", "dl", "-lpthread");
    nob_da_append_many(&cmd, objects, object_count);
    nob_cc_output(&cmd, output);
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

    nob_log(INFO, "Building Rust FFI library");
    if (!build_rust_ffi()) return false;

    Source sources[] = {
        {"org-string", "src/org-string.c"},
        {"template", "src/template.c"},
        {"site-builder", "src/site-builder.c"},
        {"main", "src/main.c"},
    };

    size_t source_count = NOB_ARRAY_LEN(sources);
    const char *objects[source_count];
    for (size_t i = 0; i < source_count; ++i) {
        objects[i] = nob_temp_sprintf("build/%s.o", sources[i].name);
        if (!compile_source(sources[i].src, objects[i])) return false;
    }

    return link_program(objects, source_count, "build/org-blog");
}

bool build_test(const char *test_name, const char *test_source, const char **objects, size_t object_count)
{
    char *output = nob_temp_sprintf("build/%s", test_name);
    Nob_Cmd cmd = {0};
    nob_cc(&cmd);
    nob_cc_flags(&cmd);
    nob_cmd_append(&cmd, "-pedantic", "-std=c99", "-I", "src", test_source);
    nob_da_append_many(&cmd, objects, object_count);
    nob_cc_output(&cmd, output);
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
        nob_log(INFO, "Cleaning build and Rust target directories");
        Nob_Cmd cmd = {0};
        nob_cmd_append(&cmd, "rm", "-rf", "build/", "ffi/target/");
        return nob_cmd_run(&cmd) ? 0 : 1;
    }

    if (strcmp(argv[0], "test") == 0) {
        nob_log(INFO, "Running tests");
        if (!nob_mkdir_if_not_exists("build")) return 1;

        nob_log(INFO, "Building Rust FFI library");
        if (!build_rust_ffi()) return 1;

        const char *objects[] = {"build/org-string.o", "build/template.o"};
        const char *sources[] = {"src/org-string.c", "src/template.c"};

        size_t count = NOB_ARRAY_LEN(objects);
        if (!compile_sources(sources, objects, count)) return 1;

        const char *test_sources[] = {"test/test_string.c", "test/test_template.c", "test/test_ffi.c", "test/test_page_structure.c"};
        const char *test_deps_string[] = {objects[0]};
        const char *test_deps_template[] = {objects[0], objects[1]};
        const char *test_deps_ffi[] = {objects[0]};
        const char *test_deps_page_structure[] = {};

        if (!build_test("test_string", test_sources[0], test_deps_string, NOB_ARRAY_LEN(test_deps_string))) return 1;
        if (!build_test("test_template", test_sources[1], test_deps_template, NOB_ARRAY_LEN(test_deps_template))) return 1;

        nob_log(INFO, "Building FFI test");
        Nob_Cmd cmd = {0};
        nob_cc(&cmd);
        nob_cc_flags(&cmd);
        nob_cmd_append(&cmd, "-pedantic", "-std=c99", "-I", "include", "-I", "src");
        nob_cmd_append(&cmd, "-L", "ffi/target/release", "-Wl,-rpath,ffi/target/release");
        nob_cmd_append(&cmd, "-l", "org_ffi", "-l", "dl");
        nob_cmd_append(&cmd, test_sources[2]);
        nob_da_append_many(&cmd, test_deps_ffi, NOB_ARRAY_LEN(test_deps_ffi));
        nob_cc_output(&cmd, "build/test_ffi");
        if (!nob_cmd_run(&cmd)) return 1;

        cmd.count = 0;
        nob_cmd_append(&cmd, "./build/test_ffi");
        if (!nob_cmd_run(&cmd)) return 1;

        nob_log(INFO, "Building page structure test");
        cmd.count = 0;
        nob_cc(&cmd);
        nob_cc_flags(&cmd);
        nob_cmd_append(&cmd, "-pedantic", "-std=c99");
        nob_cmd_append(&cmd, test_sources[3]);
        nob_cc_output(&cmd, "build/test_page_structure");
        if (!nob_cmd_run(&cmd)) return 1;

        cmd.count = 0;
        nob_cmd_append(&cmd, "./build/test_page_structure");
        if (!nob_cmd_run(&cmd)) return 1;

        nob_log(INFO, "All tests passed");
        return 0;
    }

    if (strcmp(argv[0], "blog") == 0) {
        Nob_Cmd cmd = {0};
        nob_cmd_append(&cmd, "./build/org-blog");
        nob_da_append_many(&cmd, argv + 1, argc - 1);
        return nob_cmd_run(&cmd) ? 0 : 1;
    }

    nob_log(ERROR, "Unknown command: %s", argv[0]);
    nob_log(INFO, "Usage: %s [build|clean|test|blog]", program);
    return 1;
}
