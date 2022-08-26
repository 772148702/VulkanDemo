set_project("VulkanDemo")
add_rules("mode.debug", "mode.release", "mode.releasedbg")

set_languages("c11", "cxx17")

include_dir_list = {
  "$(projectdir)/external/vulkan/windows/include",
   "$(projectdir)/external/imgui",
   "$(projectdir)/external/SPIRV-Cross",
   "$(projectdir)/external/assimp/include"
}

links_list={"User32","Gdi32","shell32"}
add_defines("PLATFORM_WINDOWS","NOMINMAX","MONKEY_DEBUG","_WINDOWS")
target("Vulkan")
    set_kind("static")
    add_files("./src/Engine/**.cpp")
    add_includedirs(include_dir_list, "$(projectdir)/src/Engine")
    add_links(links_list,"$(projectdir)/external/vulkan/windows/lib/vulkan-1")
        --copy resource file to build directory

target_end()

target("Shader")
   set_kind("static")
    on_load(function(target)
        --os.cd("./src")
        --os.execv("python", {path.join("Example","compile.py")})
        local temp = vformat(path.join("$(buildir)","$(os)","$(arch)","$(mode)"))
        local srcResource = vformat(path.join("$(projectdir)","src","Example","assets"));
        os.rm(path.join(temp,"assets"))
        os.cp(srcResource,temp)
       
        --print(vformat("cp Resource from "..srcResource.." to "..temp));
    end)
target_end()

includes("src/Example/**/xmake.lua")