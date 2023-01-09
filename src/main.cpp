#include <fmt/core.h>
#include <fmt/chrono.h>
#include <chrono>
#include <ctime>
#include <string>
#include <SDL2/SDL.h>

void log(const std::string& msg)
{
    auto time = fmt::localtime(std::time(nullptr));
    fmt::print("{:%H:%M:%S} - {}\n", time, msg);
}

int main(int arc, char** argv)
{
    log("Startup");
    return 0;
}