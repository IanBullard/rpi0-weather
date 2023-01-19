set_languages("cxx17")

requires = {"freeimage", "freetype", "glm", "libsdl", "libzip", "fmt", "python", "sqlite3", "rapidjson", "utfcpp"}
add_requires(requires)

target("weather")
    set_kind("binary")
    add_files("src/*.cpp")
    add_files("src/utils/*.cpp")
    add_packages(requires)

    if is_plat("windows") then
        add_cxxflags("/Zc:__cplusplus")
    end
