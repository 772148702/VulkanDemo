set_project("VulkanDemo")
add_rules("mode.debug", "mode.release", "mode.releasedbg")

set_languages("c11", "cxx17")

include_dir_list = {
  "$(projectdir)/external/vulkan/windows/include",
   "$(projectdir)/external/imgui",
   "$(projectdir)/external/SPIRV-Cross",
   "$(projectdir)/external/assimp/include",
}


links_list={"User32","Gdi32","shell32","Advapi32"}
add_defines("PLATFORM_WINDOWS","NOMINMAX","MONKEY_DEBUG","_WINDOWS")

target("imgui")
     set_kind("static")
    add_includedirs(include_dir_list)
 
    add_files("./external/imgui/**.cpp")
target_end()


package("myassimp")
    add_deps("cmake")
    set_sourcedir(path.join("$(projectdir)","external", "assimp"))
    on_install(function (package)
        local configs = {}
        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
        table.insert(configs, "-DBUILD_SHARED_LIBS=" .. (package:config("shared") and "ON" or "OFF"))
        import("package.tools.cmake").install(package, configs)
    end)
package_end()


add_requires("myassimp")

target("Vulkan")
    set_kind("static")
    add_files("./src/Engine/**.cpp")
    add_includedirs(include_dir_list, "$(projectdir)/src/Engine")
    add_deps("imgui")
    add_packages("myassimp")
    add_links(links_list,"$(projectdir)/external/vulkan/windows/lib/vulkan-1")
        --copy resource file to build directory
target_end()

target("Shader")
   set_kind("static")
    on_load(function(target)
        --os.cd("./src")
        --os.execv("python", {path.join("Example","compile.py")})
        -- local temp = vformat(path.join("$(buildir)","$(os)","$(arch)","$(mode)"))
        -- local srcResource = vformat(path.join("$(projectdir)","src","Example","assets"));
        -- os.rm(path.join(temp,"assets"))
        -- os.cp(srcResource,temp)
       
        --print(vformat("cp Resource from "..srcResource.." to "..temp));
    end)
target_end()

includes("src/Example/**/xmake.lua")