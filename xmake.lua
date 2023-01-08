add_requires("fmt")
set_languages("c++17")

target("console")
    set_kind("binary")
    add_files("src/*.cpp")
    add_packages("libsdl", "fmt")
