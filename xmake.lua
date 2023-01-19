set_languages("cxx17")
add_rules("mode.debug", "mode.release")
if is_plat("windows") then
    set_config("cxxflags", "/Zc:__cplusplus")
end

requires = {"freeimage", "freetype", "glm", "libsdl", "libzip", "fmt", "python", "sqlite3", "rapidjson", "utfcpp"}
add_requires(requires)

target("weather")
    set_kind("binary")
    add_files("src/*.cpp")
    add_files("src/utils/*.cpp")
    add_packages(requires)
    add_defines("-std:c++17")
