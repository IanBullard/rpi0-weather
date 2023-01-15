requires = {"freeimage", "freetype", "glm", "libsdl", "libzip", "fmt", "python", "sqlite3"}
add_requires(requires)
set_languages("c++17")

target("weather")
    set_kind("binary")
    add_files("src/*.cpp")
    add_files("src/utils/*.cpp")
    add_packages(requires)
