target("10_Pipelines")
set_kind("binary")
add_files("/*.cpp","../LaunchWindows.cpp")
add_links(links_list)
add_includedirs(include_dir_list, "$(projectdir)/src/Engine")
add_ldflags("-subsystem:windows")
add_deps("Vulkan")
