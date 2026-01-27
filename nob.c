#define NOB_IMPLEMENTATION
#include "nob.h"

static const char *rust_inputs[] = {
    "ffi/Cargo.toml",
    "ffi/Cargo.lock",
    "ffi/src/lib.rs",
};

static const char *c_headers[] = {
    "include/org-ffi.h",
    "src/org-string.h",
    "src/parser.h",
    "src/render.h",
    "src/site-builder.h",
    "src/template.h",
    "src/tokenizer.h",
};

static const char *core_sources[] = {
    "src/org-string.c",
    "src/template.c",
    "src/site-builder.c",
    "src/rss.c",
    "src/main.c",
};

static bool should_rebuild(const char *output, const char **inputs, size_t count, const char *label)
{
    int needs = nob_needs_rebuild(output, inputs, count);
    if (needs < 0) return false;
    if (needs == 0 && label) {
        nob_log(INFO, "Up to date: %s", label);
    }
    return needs > 0;
}

static bool build_rust_ffi(void)
{
    const char *output = "ffi/target/release/liborg_ffi.so";
    if (!should_rebuild(output, rust_inputs, NOB_ARRAY_LEN(rust_inputs), output)) return true;
    Nob_Cmd cmd = {0};
    nob_cmd_append(&cmd, "cargo", "build", "--release", "--manifest-path", "ffi/Cargo.toml");
    return nob_cmd_run(&cmd);
}

static bool compile_object(const char *src, const char *obj)
{
    const char *inputs[1 + NOB_ARRAY_LEN(c_headers)];
    inputs[0] = src;
    for (size_t i = 0; i < NOB_ARRAY_LEN(c_headers); ++i) {
        inputs[i + 1] = c_headers[i];
    }
    if (!should_rebuild(obj, inputs, NOB_ARRAY_LEN(inputs), obj)) return true;

    Nob_Cmd cmd = {0};
    nob_cc(&cmd);
    nob_cc_flags(&cmd);
    nob_cmd_append(&cmd, "-pedantic", "-std=c99", "-I", "src", "-I", "include", "-c", src);
    nob_cc_output(&cmd, obj);
    return nob_cmd_run(&cmd);
}

static bool link_program(const char **objects, size_t object_count, const char *output)
{
    const char *inputs[object_count + 1];
    for (size_t i = 0; i < object_count; ++i) {
        inputs[i] = objects[i];
    }
    inputs[object_count] = "ffi/target/release/liborg_ffi.so";
    if (!should_rebuild(output, inputs, NOB_ARRAY_LEN(inputs), output)) return true;

    Nob_Cmd cmd = {0};
    nob_cc(&cmd);
    nob_cmd_append(&cmd, "-L", "ffi/target/release", "-Wl,-rpath,$ORIGIN/../ffi/target/release");
    nob_cmd_append(&cmd, "-l", "org_ffi", "-l", "dl", "-lpthread");
    nob_da_append_many(&cmd, objects, object_count);
    nob_cc_output(&cmd, output);
    return nob_cmd_run(&cmd);
}

static bool build_project(void)
{
    if (!nob_mkdir_if_not_exists("build")) return false;
    nob_log(INFO, "Checking Rust FFI library");
    if (!build_rust_ffi()) return false;

    const char *objects[NOB_ARRAY_LEN(core_sources)];
    for (size_t i = 0; i < NOB_ARRAY_LEN(core_sources); ++i) {
        objects[i] = nob_temp_sprintf("build/%s.o", nob_path_name(core_sources[i]));
        if (!compile_object(core_sources[i], objects[i])) return false;
    }

    return link_program(objects, NOB_ARRAY_LEN(objects), "build/org-blog");
}

static bool build_and_run_test(const char *test_name, const char *test_source, const char **objects, size_t object_count)
{
    const char *output = nob_temp_sprintf("build/%s", test_name);
    Nob_Cmd cmd = {0};
    nob_cc(&cmd);
    nob_cc_flags(&cmd);
    nob_cmd_append(&cmd, "-pedantic", "-std=c99", "-I", "src", "-I", "include", test_source);
    nob_da_append_many(&cmd, objects, object_count);
    nob_cc_output(&cmd, output);
    if (!nob_cmd_run(&cmd)) return false;

    cmd.count = 0;
    nob_cmd_append(&cmd, output);
    return nob_cmd_run(&cmd);
}

static bool build_and_run_ffi_test(const char *test_source)
{
    Nob_Cmd cmd = {0};
    nob_cc(&cmd);
    nob_cc_flags(&cmd);
    nob_cmd_append(&cmd, "-pedantic", "-std=c99", "-I", "include", "-I", "src");
    nob_cmd_append(&cmd, "-L", "ffi/target/release", "-Wl,-rpath,ffi/target/release");
    nob_cmd_append(&cmd, "-l", "org_ffi", "-l", "dl", test_source);
    nob_cc_output(&cmd, "build/test_ffi");
    if (!nob_cmd_run(&cmd)) return false;

    cmd.count = 0;
    nob_cmd_append(&cmd, "./build/test_ffi");
    return nob_cmd_run(&cmd);
}

static bool build_and_run_page_structure_test(const char *test_source)
{
    Nob_Cmd cmd = {0};
    nob_cc(&cmd);
    nob_cc_flags(&cmd);
    nob_cmd_append(&cmd, "-pedantic", "-std=c99", test_source);
    nob_cc_output(&cmd, "build/test_page_structure");
    if (!nob_cmd_run(&cmd)) return false;

    cmd.count = 0;
    nob_cmd_append(&cmd, "./build/test_page_structure");
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
        if (!compile_object("src/org-string.c", objects[0])) return 1;
        if (!compile_object("src/template.c", objects[1])) return 1;

        if (!build_and_run_test("test_string", "test/test_string.c", objects, 1)) return 1;
        if (!build_and_run_test("test_template", "test/test_template.c", objects, 2)) return 1;

        nob_log(INFO, "Building FFI test");
        if (!build_and_run_ffi_test("test/test_ffi.c")) return 1;

        nob_log(INFO, "Building page structure test");
        if (!build_and_run_page_structure_test("test/test_page_structure.c")) return 1;

        nob_log(INFO, "All tests passed");
        return 0;
    }

    if (strcmp(argv[0], "blog") == 0) {
        nob_log(INFO, "Building blog before run");
        if (!build_project()) return 1;
        Nob_Cmd cmd = {0};
        nob_cmd_append(&cmd, "./build/org-blog");
        nob_da_append_many(&cmd, argv + 1, argc - 1);
        return nob_cmd_run(&cmd) ? 0 : 1;
    }

    nob_log(ERROR, "Unknown command: %s", argv[0]);
    nob_log(INFO, "Usage: %s [build|clean|test|blog]", program);
    return 1;
}
