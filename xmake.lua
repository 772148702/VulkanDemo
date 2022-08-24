set_project("VulkanDemo")
add_rules("mode.debug", "mode.release", "mode.releasedbg")

set_languages("c11", "cxx17")

include_dir_list = {
  "$(projectdir)/external/vulkan/windows/include"
}

links_list={"User32","Gdi32"}
add_defines("PLATFORM_WINDOWS","NOMINMAX","MONKEY_DEBUG")
target("Vulkan")
    set_kind("static")
    add_files("./src/Engine/**.cpp")
    add_includedirs(include_dir_list, "$(projectdir)/src/Engine")
    add_links(links_list,"$(projectdir)/external/vulkan/windows/lib/vulkan-1")
target_end()

includes("src/Example/**/xmake.lua")