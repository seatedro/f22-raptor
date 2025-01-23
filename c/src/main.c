#include <stdbool.h>
#include <SDL.h>
#include "game_state.h"
#include "renderer.h"
#include <stdio.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

// Global state for emscripten main loop
typedef struct {
    bool quit;
    bool thrust_active;
    GameState game_state;
    Renderer renderer;
    uint32_t last_frame_time;  // Track frame timing
    float delta_time;       
    float accumulated_time;   // Time between frames in seconds
    float target_fps;          // Target frame rate
    float frame_time;          // Target time per frame in ms
} GameContext;

void handle_input(GameContext* ctx) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                ctx->quit = true;
                break;
            case SDL_MOUSEBUTTONDOWN:
                game_state_handle_click(&ctx->game_state, event.button.x, event.button.y);
                #ifdef __EMSCRIPTEN__
                EM_ASM(
                    Module.setGameState(1);
                    console.log("GAME HAS BEGUN");
                );
                #endif
                break;
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_SPACE:
                    case SDLK_UP:
                        ctx->thrust_active = true;
                        break;
                    case SDLK_ESCAPE:
                        ctx->quit = true;
                        break;
                }
                break;
            case SDL_KEYUP:
                switch (event.key.keysym.sym) {
                    case SDLK_SPACE:
                    case SDLK_UP:
                        ctx->thrust_active = false;
                        break;
                }
                break;
        }
    }
}

void main_loop(void* arg) {
    GameContext* ctx = (GameContext*)arg;
    const float FIXED_TIME_STEP = 1.0f / 60.0f;  // Fixed 60Hz physics
    
    uint32_t current_time = SDL_GetTicks();
    float frame_time = (current_time - ctx->last_frame_time) / 1000.0f;
    ctx->last_frame_time = current_time;
    
    // Prevent spiral of death
    if (frame_time > 0.25f) frame_time = 0.25f;
    
    // Accumulate time and update in fixed steps
    ctx->accumulated_time += frame_time;
    
    while (ctx->accumulated_time >= FIXED_TIME_STEP) {
        handle_input(ctx);
        game_state_update(&ctx->game_state, ctx->thrust_active, FIXED_TIME_STEP);
        renderer_draw_frame(&ctx->renderer, &ctx->game_state, ctx->thrust_active);
        ctx->accumulated_time -= FIXED_TIME_STEP;
    }
    // Check collisions
    if (game_state_check_collisions(&ctx->game_state)) {
        #ifdef __EMSCRIPTEN__
        emscripten_cancel_main_loop();
        EM_ASM({
        Module.showGameOver($0);
        }, ctx->game_state.score);
        #else
        ctx->quit = true;
        printf("Game Over! Score: %u\n", ctx->game_state.score);
        #endif
        return;
    }

    #ifdef __EMSCRIPTEN__
    // SDL_Delay(16); // ~60 FPS cap for native builds
    #endif
}

int main() {
    #ifdef __EMSCRIPTEN__
    setvbuf(stdout, NULL, _IOLBF, 0);
    #endif

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_Log("SDL init failed: %s", SDL_GetError());
        return 1;
    }

    // Initialize context with new timing variables
    GameContext ctx = {
        .quit = false,
        .thrust_active = false,
        .game_state = game_state_init(),
        .last_frame_time = SDL_GetTicks(),  // Initialize timing
        .delta_time = 0.0f,
        .accumulated_time = 0.0f,
        .target_fps = 60.0f,  // Set target frame rate
        .frame_time = 1000.0f / 60.0f  // Calculate ms per frame (33.33ms for 30fps)
    };

    if (renderer_init(&ctx.renderer) < 0) {
        SDL_Log("Renderer init failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Rect viewport;
    SDL_RenderGetViewport(ctx.renderer.renderer, &viewport);
    printf("Viewport size: x=%d, y=%d, w=%d, h=%d\n",
           viewport.x, viewport.y, viewport.w, viewport.h);

    // Force the viewport size
    SDL_RenderSetLogicalSize(ctx.renderer.renderer, WINDOW_WIDTH, WINDOW_HEIGHT);

    #ifdef EMSCRIPTEN
        // Set up frame-capped main loop for web
        // The '0' parameter lets Emscripten handle timing
        // The '1' means to simulate infinite loop
        emscripten_set_main_loop_arg(main_loop, &ctx, 0, 1);
    #else
    // Native build - manual frame timing
    while (!ctx.quit) {
        uint32_t current_time = SDL_GetTicks();
        uint32_t delta = current_time - ctx.last_frame_time;
        
        // Only update if enough time has passed
        if (delta >= ctx.frame_time) {
            ctx.delta_time = delta / 1000.0f;  // Convert to seconds
            main_loop(&ctx);
            ctx.last_frame_time = current_time;
        } else {
            // Sleep to avoid maxing CPU
            SDL_Delay(1);
        }
    }
    #endif

    renderer_cleanup(&ctx.renderer);
    SDL_Quit();
    return 0;
}
