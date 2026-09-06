-- Add the minhook dependency definition to Xmake's manager
add_requires("minhook")
add_requires("miniz")
add_syslinks("user32", "gdi32", "d3d9")

set_xmakever("2.5.1")
set_project("T5SP-MOD")
set_version("1.0.0")

set_arch("x86")
set_plat("windows")

target("d3d9")
    set_kind("shared")
    -- Force C++20 standard to satisfy the std::ranges dependencies
    set_languages("c++20", "c11")

    -- Dynamic Includes
    add_includedirs(".") 
    add_includedirs("T5SP-MOD/src/client")
    add_includedirs("Project-utils/utils")

    -- Dynamic Sources (Compiles your proxy client and hook utilities)
    add_files("T5SP-MOD/src/client/**.cpp")
    add_files("Project-utils/utils/utils/**.cpp")

    -- FIX 1: Automatically inject and download MinHook to resolve the MH_ errors
    add_packages("minhook")

    -- FIX 2: Explicitly link User32.lib to resolve MessageBoxA and Clipboard errors
    add_syslinks("user32")

    add_defines("WIN32", "_WINDOWS", "_USRDLL", "_MBCS") 

    if is_mode("debug") then
        set_symbols("debug")
        set_optimize("none")
    else
        set_symbols("hidden")
        set_optimize("fastest")
        add_cxflags("/O2")
    end

    add_ldflags("/DLL")
