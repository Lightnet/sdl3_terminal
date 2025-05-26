#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#define MAX_TEXT_LENGTH 256
#define MAX_LINES 100 // Increased to allow more lines
#define LINES_PER_SCREEN 30 // Approx. 600px height / 20px per line
#define CURSOR_BLINK_MS 500

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
    if (!SDL_CreateWindowAndRenderer("SDL3 Terminal Test", 800, 600, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
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
    int scroll_offset = 0; // Start of visible lines
    int cursor_pos = strlen(text_buffers[0]); // Cursor position in current line
    SDL_Texture *textures[MAX_LINES] = {NULL};
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color black = {0, 0, 0, 255};
    bool cursor_visible = true;
    Uint32 last_cursor_toggle = 0;

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
                    // Insert input text at cursor position
                    size_t current_len = strlen(text_buffers[current_line]);
                    size_t input_len = strlen(event.text.text);
                    if (current_len + input_len < MAX_TEXT_LENGTH - 1) {
                        // Shift characters right to make space
                        memmove(&text_buffers[current_line][cursor_pos + input_len],
                                &text_buffers[current_line][cursor_pos],
                                current_len - cursor_pos + 1); // +1 for null terminator
                        // Insert new text
                        memcpy(&text_buffers[current_line][cursor_pos], event.text.text, input_len);
                        cursor_pos += input_len;
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
                    if (event.key.key == SDLK_BACKSPACE) {
                        // Remove character before cursor if not at start
                        if (cursor_pos > 0) {
                            memmove(&text_buffers[current_line][cursor_pos - 1],
                                    &text_buffers[current_line][cursor_pos],
                                    strlen(text_buffers[current_line]) - cursor_pos + 1);
                            cursor_pos--;
                            // Update texture for current line
                            if (textures[current_line]) SDL_DestroyTexture(textures[current_line]);
                            if (strlen(text_buffers[current_line]) > 0) {
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
                                    break;
                                }
                            } else {
                                textures[current_line] = NULL; // Empty line, no texture
                            }
                        }
                    } else if (event.key.key == SDLK_DELETE) {
                        // Remove character at cursor if not at end
                        if (cursor_pos < (int)strlen(text_buffers[current_line])) {
                            memmove(&text_buffers[current_line][cursor_pos],
                                    &text_buffers[current_line][cursor_pos + 1],
                                    strlen(text_buffers[current_line]) - cursor_pos);
                            // Update texture for current line
                            if (textures[current_line]) SDL_DestroyTexture(textures[current_line]);
                            if (strlen(text_buffers[current_line]) > 0) {
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
                                    break;
                                }
                            } else {
                                textures[current_line] = NULL; // Empty line, no texture
                            }
                        }
                    } else if (event.key.key == SDLK_LEFT) {
                        // Move cursor left
                        if (cursor_pos > 0) {
                            cursor_pos--;
                        }
                    } else if (event.key.key == SDLK_RETURN && current_line < MAX_LINES - 1) {
                        // Check for commands
                        if (strcmp(text_buffers[current_line], "clear") == 0) {
                            // Clear all lines and reset
                            for (int i = 0; i < MAX_LINES; i++) {
                                text_buffers[i][0] = '\0';
                                if (textures[i]) {
                                    SDL_DestroyTexture(textures[i]);
                                    textures[i] = NULL;
                                }
                            }
                            current_line = 0;
                            scroll_offset = 0;
                            cursor_pos = 0;
                        } else if (strcmp(text_buffers[current_line], "exit") == 0) {
                            running = false;
                        } else {
                            // Parse current line (print to console)
                            if (strlen(text_buffers[current_line]) > 0) {
                                printf("Parsed input: %s\n", text_buffers[current_line]);
                            }
                            // Move to next line
                            current_line++;
                            text_buffers[current_line][0] = '\0'; // Clear new line
                            if (textures[current_line]) SDL_DestroyTexture(textures[current_line]);
                            textures[current_line] = NULL; // Ensure texture is null for empty line
                            cursor_pos = 0; // Reset cursor for new line
                            // Adjust scroll offset if needed
                            if (current_line >= scroll_offset + LINES_PER_SCREEN) {
                                scroll_offset++;
                            }
                        }
                    }
                    break;
            }
        }

        // Update cursor blink
        Uint32 current_time = SDL_GetTicks();
        if (current_time - last_cursor_toggle >= CURSOR_BLINK_MS) {
            cursor_visible = !cursor_visible;
            last_cursor_toggle = current_time;
        }

        // Render
        SDL_SetRenderDrawColor(renderer, black.r, black.g, black.b, black.a);
        SDL_RenderClear(renderer);
        // Render visible lines
        for (int i = scroll_offset; i <= current_line && i < scroll_offset + LINES_PER_SCREEN; i++) {
            if (textures[i]) {
                SDL_FRect dest = {10.0f, 10.0f + (i - scroll_offset) * 20.0f, 0.0f, 0.0f}; // 20px vertical spacing
                SDL_GetTextureSize(textures[i], &dest.w, &dest.h);
                SDL_RenderTexture(renderer, textures[i], NULL, &dest);
            }
        }
        // Render blinking cursor on current line
        if (cursor_visible) {
            float text_width = 0.0f;
            if (textures[current_line] && cursor_pos > 0) {
                // Create temporary string up to cursor_pos
                char temp[MAX_TEXT_LENGTH] = {0};
                strncpy(temp, text_buffers[current_line], cursor_pos);
                surface = TTF_RenderText_Solid(font, temp, strlen(temp), white);
                if (surface) {
                    SDL_Texture *temp_texture = SDL_CreateTextureFromSurface(renderer, surface);
                    SDL_DestroySurface(surface);
                    if (temp_texture) {
                        SDL_GetTextureSize(temp_texture, &text_width, NULL);
                        SDL_DestroyTexture(temp_texture);
                    }
                }
            }
            float cursor_x = 10.0f + text_width;
            float cursor_y = 10.0f + (current_line - scroll_offset) * 20.0f;
            SDL_SetRenderDrawColor(renderer, white.r, white.g, white.b, white.a);
            SDL_RenderLine(renderer, cursor_x, cursor_y, cursor_x, cursor_y + 16.0f); // 16px cursor height
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