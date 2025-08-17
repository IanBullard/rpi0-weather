#include "sdl_emulator.h"
#include <iostream>
#include <cstring>

#ifdef HAVE_SDL3
#include <SDL3/SDL.h>
#endif

SDL3Emulator::SDL3Emulator() 
    : window_(nullptr)
    , renderer_(nullptr)
    , display_texture_(nullptr)
    , pixel_buffer_(nullptr)
    , initialized_(false)
    , quit_requested_(false)
{
    // Allocate pixel buffer
    pixel_buffer_ = new uint8_t[DISPLAY_WIDTH * DISPLAY_HEIGHT];
    std::memset(pixel_buffer_, 1, DISPLAY_WIDTH * DISPLAY_HEIGHT); // Start with white
}

SDL3Emulator::~SDL3Emulator() {
    shutdown();
    delete[] pixel_buffer_;
}

bool SDL3Emulator::initialize() {
    if (initialized_) {
        return true;
    }
    
#ifdef HAVE_SDL3
    // Initialize SDL3
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "Failed to initialize SDL3: " << SDL_GetError() << std::endl;
        return false;
    }
    
    // Create window
    window_ = SDL_CreateWindow(
        "Inky Display Emulator - rpi0-weather",
        DISPLAY_WIDTH * WINDOW_SCALE,
        DISPLAY_HEIGHT * WINDOW_SCALE,
        0
    );
    
    if (!window_) {
        std::cerr << "Failed to create SDL3 window: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return false;
    }
    
    // Create renderer
    renderer_ = SDL_CreateRenderer(window_, nullptr);
    if (!renderer_) {
        std::cerr << "Failed to create SDL3 renderer: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window_);
        SDL_Quit();
        return false;
    }
    
    // Create texture for display buffer
    display_texture_ = SDL_CreateTexture(
        renderer_,
        SDL_PIXELFORMAT_RGB24,
        SDL_TEXTUREACCESS_STREAMING,
        DISPLAY_WIDTH,
        DISPLAY_HEIGHT
    );
    
    if (!display_texture_) {
        std::cerr << "Failed to create SDL3 texture: " << SDL_GetError() << std::endl;
        SDL_DestroyRenderer(renderer_);
        SDL_DestroyWindow(window_);
        SDL_Quit();
        return false;
    }
    
    initialized_ = true;
    std::cout << "SDL3 emulator initialized successfully" << std::endl;
    std::cout << "Controls: Keys 1-4 = Buttons A-D, ESC = Quit" << std::endl;
    
    return true;
#else
    std::cout << "SDL3 emulator not available (compiled without SDL3 support)" << std::endl;
    std::cout << "Using console mode instead" << std::endl;
    initialized_ = true;
    return true;
#endif
}

void SDL3Emulator::shutdown() {
    if (!initialized_) {
        return;
    }
    
#ifdef HAVE_SDL3
    if (display_texture_) {
        SDL_DestroyTexture(display_texture_);
        display_texture_ = nullptr;
    }
    
    if (renderer_) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }
    
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    
    SDL_Quit();
#endif
    
    initialized_ = false;
    std::cout << "SDL3 emulator shutdown" << std::endl;
}

void SDL3Emulator::clear(uint8_t color) {
    if (!initialized_ || color >= 8) {
        return;
    }
    
    std::memset(pixel_buffer_, color, DISPLAY_WIDTH * DISPLAY_HEIGHT);
}

void SDL3Emulator::set_pixel(int x, int y, uint8_t color) {
    if (!initialized_ || x < 0 || x >= DISPLAY_WIDTH || 
        y < 0 || y >= DISPLAY_HEIGHT || color >= 8) {
        return;
    }
    
    pixel_buffer_[y * DISPLAY_WIDTH + x] = color;
}

void SDL3Emulator::update() {
    if (!initialized_) {
        return;
    }
    
#ifdef HAVE_SDL3
    render_display();
    std::cout << "SDL3 Emulator: Display updated" << std::endl;
#else
    std::cout << "Console Emulator: Display updated (no visual)" << std::endl;
#endif
}

void SDL3Emulator::poll_events() {
    if (!initialized_) {
        return;
    }
    
#ifdef HAVE_SDL3
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_QUIT:
                quit_requested_ = true;
                break;
                
            case SDL_EVENT_KEY_DOWN:
                if (event.key.scancode == SDL_SCANCODE_ESCAPE) {
                    quit_requested_ = true;
                } else {
                    handle_keydown(event.key.scancode);
                }
                break;
        }
    }
#else
    // In console mode, just check for a simple quit condition
    // For now, don't quit automatically
#endif
}

void SDL3Emulator::set_button_callback(ButtonCallback callback) {
    button_callback_ = callback;
}

void SDL3Emulator::handle_keydown(int scancode) {
    if (!button_callback_) {
        return;
    }
    
#ifdef HAVE_SDL3
    int button = -1;
    switch (scancode) {
        case SDL_SCANCODE_1: button = 0; break; // Button A
        case SDL_SCANCODE_2: button = 1; break; // Button B  
        case SDL_SCANCODE_3: button = 2; break; // Button C
        case SDL_SCANCODE_4: button = 3; break; // Button D
    }
    
    if (button >= 0) {
        std::cout << "Button " << char('A' + button) << " pressed" << std::endl;
        button_callback_(button);
    }
#endif
}

void SDL3Emulator::render_display() {
#ifdef HAVE_SDL3
    // Convert our palette-indexed buffer to RGB24
    void* texture_pixels;
    int texture_pitch;
    
    if (SDL_LockTexture(display_texture_, nullptr, &texture_pixels, &texture_pitch) < 0) {
        std::cerr << "Failed to lock texture: " << SDL_GetError() << std::endl;
        return;
    }
    
    uint8_t* rgb_pixels = static_cast<uint8_t*>(texture_pixels);
    
    for (int y = 0; y < DISPLAY_HEIGHT; y++) {
        for (int x = 0; x < DISPLAY_WIDTH; x++) {
            uint8_t color_index = pixel_buffer_[y * DISPLAY_WIDTH + x];
            const Color& color = PALETTE[color_index];
            
            int pixel_offset = y * texture_pitch + x * 3;
            rgb_pixels[pixel_offset + 0] = color.r; // R
            rgb_pixels[pixel_offset + 1] = color.g; // G
            rgb_pixels[pixel_offset + 2] = color.b; // B
        }
    }
    
    SDL_UnlockTexture(display_texture_);
    
    // Render the texture scaled to window size
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);
    SDL_RenderTexture(renderer_, display_texture_, nullptr, nullptr);
    SDL_RenderPresent(renderer_);
#endif
}