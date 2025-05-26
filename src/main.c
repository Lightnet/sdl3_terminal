#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#define MAX_TEXT_LENGTH 256
#define MAX_LINES 10

/* We will use this renderer to draw into this window every frame. */
static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;

int main(int argc, char *argv[]) {
    printf("SDL3 freetype\n");

    // Initialize SDL
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s", SDL_GetError());
        return 1;
    }

    // Initialize SDL_ttf
    printf("TTF_Init\n");
    if (!TTF_Init()) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "TTF_Init failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // Create window and renderer
    printf("SDL_CreateWindowAndRenderer\n");
    if (!SDL_CreateWindowAndRenderer("Font Test", 800, 600, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Window/Renderer creation failed: %s", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // Load font
    printf("TTF_OpenFont\n");
    TTF_Font *font = TTF_OpenFont("Kenney Mini.ttf", 16);
    if (!font) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Font loading failed: %s", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // Enable text input
    SDL_StartTextInput(window);

    // Text buffer for multiple lines
    char text_buffers[MAX_LINES][MAX_TEXT_LENGTH] = {{0}};
    strcpy(text_buffers[0], "Hello, Terminal!");
    int current_line = 0;
    SDL_Texture *textures[MAX_LINES] = {NULL};
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color black = {0, 0, 0, 255};

    // Initial text rendering for first line
    SDL_Surface *surface = TTF_RenderText_Solid(font, text_buffers[0], strlen(text_buffers[0]), white);
    if (!surface) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Text rendering failed: %s", SDL_GetError());
        TTF_CloseFont(font);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    textures[0] = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);
    if (!textures[0]) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Texture creation failed: %s", SDL_GetError());
        TTF_CloseFont(font);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // Main loop
    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    running = false;
                    break;
                case SDL_EVENT_TEXT_INPUT: {
                    // Append input text to current line's buffer
                    size_t current_len = strlen(text_buffers[current_line]);
                    if (current_len + strlen(event.text.text) < MAX_TEXT_LENGTH - 1) {
                        strcat(text_buffers[current_line], event.text.text);
                        // Update texture for current line
                        if (textures[current_line]) SDL_DestroyTexture(textures[current_line]);
                        surface = TTF_RenderText_Solid(font, text_buffers[current_line], strlen(text_buffers[current_line]), white);
                        if (!surface) {
                            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Text rendering failed: %s", SDL_GetError());
                            running = false;
                            break;
                        }
                        textures[current_line] = SDL_CreateTextureFromSurface(renderer, surface);
                        SDL_DestroySurface(surface);
                        if (!textures[current_line]) {
                            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Texture creation failed: %s", SDL_GetError());
                            running = false;
                        }
                    }
                    break;
                }
                case SDL_EVENT_KEY_DOWN:
                    if (event.key.key == SDLK_BACKSPACE && strlen(text_buffers[current_line]) > 0) {
                        // Remove last character from current line
                        text_buffers[current_line][strlen(text_buffers[current_line]) - 1] = '\0';
                        // Update texture for current line
                        if (textures[current_line]) SDL_DestroyTexture(textures[current_line]);
                        surface = TTF_RenderText_Solid(font, text_buffers[current_line], strlen(text_buffers[current_line]), white);
                        if (!surface) {
                            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Text rendering failed: %s", SDL_GetError());
                            running = false;
                            break;
                        }
                        textures[current_line] = SDL_CreateTextureFromSurface(renderer, surface);
                        SDL_DestroySurface(surface);
                        if (!textures[current_line]) {
                            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Texture creation failed: %s", SDL_GetError());
                            running = false;
                        }
                    } else if (event.key.key == SDLK_RETURN && current_line < MAX_LINES - 1) {
                        // Parse current line (print to console)
                        if (strlen(text_buffers[current_line]) > 0) {
                            printf("Parsed input: %s\n", text_buffers[current_line]);
                        }
                        // Move to next line
                        current_line++;
                        text_buffers[current_line][0] = '\0'; // Clear new line
                        if (textures[current_line]) SDL_DestroyTexture(textures[current_line]);
                        textures[current_line] = NULL; // Ensure texture is null for empty line
                    }
                    break;
            }
        }

        // Render
        SDL_SetRenderDrawColor(renderer, black.r, black.g, black.b, black.a);
        SDL_RenderClear(renderer);
        // Render all lines
        for (int i = 0; i <= current_line; i++) {
            if (textures[i]) {
                SDL_FRect dest = {10.0f, 10.0f + i * 20.0f, 0.0f, 0.0f}; // 20px vertical spacing
                SDL_GetTextureSize(textures[i], &dest.w, &dest.h);
                SDL_RenderTexture(renderer, textures[i], NULL, &dest);
            }
        }
        SDL_RenderPresent(renderer);
    }

    // Cleanup
    for (int i = 0; i < MAX_LINES; i++) {
        if (textures[i]) SDL_DestroyTexture(textures[i]);
    }
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}