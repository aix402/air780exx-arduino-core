project_dir = os.scriptdir()
project_name = project_dir:match(".+[/\\]([%w_]+)")

local function resolve_dependency_root(env_name, candidates)
    local env_value = os.getenv(env_name)
    if env_value and #env_value > 0 and os.isdir(env_value) then
        return path.normalize(env_value)
    end

    for _, candidate in ipairs(candidates) do
        local full_candidate = path.absolute(candidate, project_dir)
        if os.isdir(full_candidate) then
            return path.normalize(full_candidate)
        end
    end

    raise("Could not resolve " .. env_name .. " from configured dependency paths")
end

csdk_root = resolve_dependency_root("LUATOS_SOC_ROOT", {
    "../../../deps/luatos-soc-2024/"
})
includes(path.join(csdk_root, "csdk.lua"))
description_common()

option("arduino_static_ctors", function()
    set_default(false)
    set_showmenu(true)
    set_description("Enable the Arduino static constructor bridge")
end)
add_options("arduino_static_ctors")

option("arduino_external_build", function()
    set_default(false)
    set_showmenu(true)
    set_description("Link Arduino CLI-produced sketch/core objects instead of compiling them in xmake")
end)
add_options("arduino_external_build")

option("arduino_build_path", function()
    set_default("")
    set_showmenu(true)
    set_description("Arduino CLI build path containing sketch objects and core.a")
end)
add_options("arduino_build_path")

add_includedirs(project_dir .. "/inc")
add_cxflags("-include", "air780epm_luat_compat.h", {force = true})
add_includedirs(luatos_root .. "/components/u8g2", {public = true})
add_includedirs(luatos_root .. "/components/lcd", {public = true})
add_includedirs(luatos_root .. "/components/network/libhttp", {public = true})
add_includedirs(luatos_root .. "/components/network/http_parser", {public = true})

target(project_name, function()
    set_kind("static")
    set_targetdir("$(buildir)/" .. project_name .. "/")
    add_defines("ARDUINO=10819", "ARDUINO_ARCH_EC718PM=1", "ARDUINO_ARCH_AIR780EPM=1", {public = true})

    add_includedirs("./inc", {public = true})
    if os.isdir(path.join(project_dir, "generated")) then
        add_includedirs("./generated", {public = true})
    end
    if os.isdir(path.join(project_dir, "generated", "src")) then
        add_includedirs("./generated/src", {public = true})
    end
    local generated_libraries = path.join(project_dir, "generated", "libraries")
    if os.isdir(generated_libraries) then
        for _, header in ipairs(os.files(path.join(generated_libraries, "**.h"))) do
            add_includedirs(path.directory(header), {public = true})
        end
        for _, header in ipairs(os.files(path.join(generated_libraries, "**.hh"))) do
            add_includedirs(path.directory(header), {public = true})
        end
        for _, header in ipairs(os.files(path.join(generated_libraries, "**.hpp"))) do
            add_includedirs(path.directory(header), {public = true})
        end
    end

    description_csdk()

    add_includedirs("../../core/air780epm/cores/air780epm", {public = true})
    add_includedirs("../../core/air780epm/variants/air780epm_dev", {public = true})

    if get_config("arduino_static_ctors") then
        add_defines("ARDUINO_ENABLE_STATIC_CONSTRUCTORS=1", {public = true})
    end

    add_files("./src/*.c", {public = true})
    add_files("./src/arduino_runtime.cpp", {public = true})
    add_files("./src/arduino_task.cpp", {public = true})
    add_files(luatos_root .. "/components/lcd/*.c", {public = true})
    add_files(luatos_root .. "/components/network/http_parser/http_parser.c", {public = true})
    add_files(luatos_root .. "/components/network/libhttp/luat_http_ctrl.c", {public = true})
    add_files(luatos_root .. "/components/network/libhttp/luat_http_client_for_csdk.c", {public = true})
    remove_files(luatos_root .. "/components/lcd/luat_lib_*.c")

    local external_arduino_build = get_config("arduino_external_build")
    local generated_sketch = path.join(project_dir, "generated", "arduino_sketch.cpp")
    if external_arduino_build then
        -- setup()/loop() and Arduino core are linked from Arduino CLI output in the final ELF target.
    elseif os.isfile(generated_sketch) then
        add_files(generated_sketch, {public = true})
        for _, file in ipairs(os.files(path.join(project_dir, "generated", "*.c"))) do
            add_files(file, {public = true})
        end
        for _, file in ipairs(os.files(path.join(project_dir, "generated", "*.cpp"))) do
            if path.filename(file) ~= "arduino_sketch.cpp" then
                add_files(file, {public = true})
            end
        end
        for _, file in ipairs(os.files(path.join(project_dir, "generated", "src", "**.c"))) do
            add_files(file, {public = true})
        end
        for _, file in ipairs(os.files(path.join(project_dir, "generated", "src", "**.cpp"))) do
            add_files(file, {public = true})
        end
        if os.isdir(generated_libraries) then
            for _, file in ipairs(os.files(path.join(generated_libraries, "**.c"))) do
                add_files(file, {public = true})
            end
            for _, file in ipairs(os.files(path.join(generated_libraries, "**.cc"))) do
                add_files(file, {public = true})
            end
            for _, file in ipairs(os.files(path.join(generated_libraries, "**.cpp"))) do
                add_files(file, {public = true})
            end
            for _, file in ipairs(os.files(path.join(generated_libraries, "**.cxx"))) do
                add_files(file, {public = true})
            end
            for _, file in ipairs(os.files(path.join(generated_libraries, "**.S"))) do
                add_files(file, {public = true})
            end
            for _, file in ipairs(os.files(path.join(generated_libraries, "**.s"))) do
                add_files(file, {public = true})
            end
        end
    else
        add_files("./src/arduino_entry.cpp", {public = true})
    end

    if not external_arduino_build then
        add_files("../../core/air780epm/cores/air780epm/*.cpp", {public = true})
    end
end)

target(project_name .. ".elf", function()
    -- The CSDK linker script is preprocessed from this final ELF target.
    -- Keep the OS selector visible here so FreeRTOS sections are placed in .load_apos.
    add_defines("FEATURE_OS_ENABLE", "FEATURE_FREERTOS_ENABLE")

    if get_config("arduino_external_build") then
        local arduino_build_path = get_config("arduino_build_path")
        if not arduino_build_path or arduino_build_path == "" then
            raise("arduino_build_path is required when arduino_external_build is enabled")
        end

        local sketch_objects = os.files(path.join(arduino_build_path, "sketch", "*.o"))
        if #sketch_objects == 0 then
            raise("No Arduino CLI sketch objects found in " .. path.join(arduino_build_path, "sketch"))
        end
        for _, object_file in ipairs(sketch_objects) do
            add_ldflags(object_file, {force = true})
        end

        local library_objects = os.files(path.join(arduino_build_path, "libraries", "**.o"))
        for _, object_file in ipairs(library_objects) do
            add_ldflags(object_file, {force = true})
        end

        local library_archives = os.files(path.join(arduino_build_path, "libraries", "**.a"))
        for _, archive_file in ipairs(library_archives) do
            add_ldflags("-Wl,--whole-archive", archive_file, "-Wl,--no-whole-archive", {force = true})
        end

        local core_archive = path.join(arduino_build_path, "core", "core.a")
        if not os.isfile(core_archive) then
            raise("Arduino CLI core archive not found: " .. core_archive)
        end
        add_ldflags("-Wl,--whole-archive", core_archive, "-Wl,--no-whole-archive", {force = true})
    end
end)
