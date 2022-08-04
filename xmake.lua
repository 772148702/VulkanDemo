set_project("VulkanDemo")
add_rules("mode.debug", "mode.release", "mode.releasedbg")

set_languages("c11", "cxx17")

include_dir_list = {
  "$(projectdir)/external/vulkan/windows/include"
}

links_list={}
target("Vulkan")
    add_files("./src/*.cpp")
    add_includedirs(include_dir_list)
    add_links(links_list,"$(projectdir)/external/vulkan/windows/lib/vulkan-1")
target_end()