project_dir = os.scriptdir()
project_name = project_dir:match(".+[/\\]([%w_]+)")

csdk_root = "../../external/luatos-soc-2024/"
includes(csdk_root .. "csdk.lua")
description_common()

option("arduino_static_ctors", function()
    set_default(false)
    set_showmenu(true)
    set_description("Enable the Arduino static constructor bridge")
end)
add_options("arduino_static_ctors")

add_includedirs(project_dir .. "/inc")
add_cxflags("-include", "air780epm_luat_compat.h", {force = true})
add_includedirs(luatos_root .. "/components/u8g2", {public = true})
add_includedirs(luatos_root .. "/components/lcd", {public = true})

target(project_name, function()
    set_kind("static")
    set_targetdir("$(buildir)/" .. project_name .. "/")
    description_csdk()

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
    add_includedirs("../../core/air780epm/cores/air780epm", {public = true})
    add_includedirs("../../core/air780epm/variants/air780epm_dev", {public = true})

    if get_config("arduino_static_ctors") then
        add_defines("ARDUINO_ENABLE_STATIC_CONSTRUCTORS=1", {public = true})
    end

    add_files("./src/*.c", {public = true})
    add_files("./src/arduino_runtime.cpp", {public = true})
    add_files("./src/arduino_task.cpp", {public = true})
    add_files(luatos_root .. "/components/lcd/*.c", {public = true})
    remove_files(luatos_root .. "/components/lcd/luat_lib_*.c")

    local generated_sketch = path.join(project_dir, "generated", "arduino_sketch.cpp")
    if os.isfile(generated_sketch) then
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

    add_files("../../core/air780epm/cores/air780epm/*.cpp", {public = true})
end)
