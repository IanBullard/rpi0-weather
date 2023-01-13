// Copyright 2023 Ian Bullard
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

#include <array>
#include <stdlib.h>
#include <SDL2/SDL.h>

#include "emulate.h"
#include "log.h"

struct color_t {
    int r;
    int g;
    int b;
};

const static std::array<color_t, 8> s_palette{
    color_t{  57,  48,  57}, // 0 black 
    color_t{ 255, 255, 255}, // 1 white
    color_t{  58,  91,  70}, // 2 green
    color_t{  61,  59,  94}, // 3 blue
    color_t{ 156,  72,  75}, // 4 red
    color_t{ 208, 190,  71}, // 5 yellow
    color_t{ 177, 106,  73}, // 6 orange
    color_t{ 255, 255, 255}  // 7 clear
};

const static uint32_t s_width = 600;
const static uint32_t s_height = 448;

SDL_Window *s_window = nullptr;
SDL_Renderer *s_renderer = nullptr;

void inky_setup()
{
    if(SDL_Init(SDL_INIT_VIDEO) != 0) {
        log("Failed to initialize SDL2");
        exit(-1);
    }

    s_window = SDL_CreateWindow("RPi0 Weather Display Emulation", 0, 0, s_width*2, s_height*2, 0);
    if(!s_window) {
        log("Failed to create window");
        exit(-1);
    }
    s_renderer = SDL_CreateRenderer(s_window, -1, SDL_RENDERER_SOFTWARE);
    if(!s_renderer) {
        log("Failed to create renderer");
        exit(-1);
    }
    SDL_SetRenderDrawColor(s_renderer, 255, 255, 255, 255);
    SDL_RenderClear(s_renderer);
    SDL_RenderPresent(s_renderer);
}

void inky_set_pixel(int x, int y, uint8_t color)
{
    SDL_SetRenderDrawColor(s_renderer, s_palette[color].r, s_palette[color].g, s_palette[color].b, 255);
    SDL_RenderDrawPoint(s_renderer, x*2+0, y*2+0);
    SDL_RenderDrawPoint(s_renderer, x*2+0, y*2+1);
    SDL_RenderDrawPoint(s_renderer, x*2+1, y*2+0);
    SDL_RenderDrawPoint(s_renderer, x*2+1, y*2+1);
}

void inky_display()
{
    SDL_RenderPresent(s_renderer);
    SDL_SetRenderDrawColor(s_renderer, 255, 255, 255, 255);
    SDL_RenderClear(s_renderer);
}

void inky_shutdown()
{
    SDL_DestroyRenderer(s_renderer);
    SDL_DestroyWindow(s_window);
    SDL_Quit();
}