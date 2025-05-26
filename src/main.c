#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#define MAX_TEXT_LENGTH 256
#define MAX_LINES 100 // Increased to allow more lines
#define LINES_PER_SCREEN 30 // Approx. 600px height / 20px per line
#define CURSOR_BLINK_MS 500
#define INITIAL_SCREEN_WIDTH 800 // Initial window width
#define TEXT_MARGIN 10 // Left margin
#define MAX_HISTORY 50 // Max commands in history

/* We will use this renderer to draw into this window every frame. */
static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static TTF_Font *font = NULL; // For command functions
static int max_text_width = INITIAL_SCREEN_WIDTH - TEXT_MARGIN; // Dynamic max width for text

// Command structure
typedef struct {
    const char *name;
    void (*function)(void);
    const char *description;
} Command;

// Forward declarations
void cmd_clear(void);
void cmd_exit(void);
void cmd_help(void);
void rewrap_text(void);

// Command table
static const Command commands[] = {
    {"clear", cmd_clear, "Clear all text in the terminal"},
    {"exit", cmd_exit, "Exit the application"},
    {"help", cmd_help, "List available commands"},
    {"-help", cmd_help, NULL}, // Alias, no description to avoid duplication
    {"-h", cmd_help, NULL},    // Alias
};
static const int num_commands = sizeof(commands) / sizeof(commands[0]);

// Global state for commands
static char text_buffers[MAX_LINES][MAX_TEXT_LENGTH] = {{0}};
static SDL_Texture *textures[MAX_LINES] = {NULL};
static bool is_line_editable[MAX_LINES] = {false}; // Track editable lines
static int current_line = 0;
static int scroll_offset = 0;
static int cursor_pos = 0;
static bool *running = NULL; // Set in main
static SDL_Color white = {255, 255, 255, 255};
static char *command_history[MAX_HISTORY] = {NULL};
static int history_count = 0;
static int history_pos = -1;

// Re-wrap text based on current max_text_width
void rewrap_text(void) {
    char temp_buffers[MAX_LINES][MAX_TEXT_LENGTH] = {{0}};
    bool temp_editable[MAX_LINES] = {false};
    SDL_Texture *temp_textures[MAX_LINES] = {NULL};
    int new_line_count = 0;
    int old_cursor_line = current_line;
    int old_cursor_pos = cursor_pos;

    // Process each existing line
    for (int i = 0; i <= current_line && new_line_count < MAX_LINES; i++) {
        if (strlen(text_buffers[i]) == 0) {
            if (new_line_count < MAX_LINES - 1) {
                temp_buffers[new_line_count][0] = '\0';
                temp_editable[new_line_count] = is_line_editable[i];
                new_line_count++;
            }
            continue;
        }

        char *text = text_buffers[i];
        int text_len = strlen(text);
        int start = 0;

        while (start < text_len && new_line_count < MAX_LINES) {
            // Find how much text fits within max_text_width
            int chars_to_render = text_len - start;
            char temp[MAX_TEXT_LENGTH] = {0};
            strncpy(temp, text + start, chars_to_render);
            SDL_Surface *surface = TTF_RenderText_Solid(font, temp, strlen(temp), white);
            if (surface) {
                SDL_Texture *temp_texture = SDL_CreateTextureFromSurface(renderer, surface);
                float text_width = 0;
                if (temp_texture) {
                    SDL_GetTextureSize(temp_texture, &text_width, NULL);
                    SDL_DestroyTexture(temp_texture);
                }
                SDL_DestroySurface(surface);

                // Binary search to find the max chars that fit
                if (text_width > max_text_width) {
                    int low = 0, high = chars_to_render;
                    while (low < high) {
                        int mid = (low + high + 1) / 2;
                        strncpy(temp, text + start, mid);
                        temp[mid] = '\0';
                        surface = TTF_RenderText_Solid(font, temp, strlen(temp), white);
                        if (surface) {
                            temp_texture = SDL_CreateTextureFromSurface(renderer, surface);
                            text_width = 0;
                            if (temp_texture) {
                                SDL_GetTextureSize(temp_texture, &text_width, NULL);
                                SDL_DestroyTexture(temp_texture);
                            }
                            SDL_DestroySurface(surface);
                            if (text_width <= max_text_width) {
                                low = mid;
                            } else {
                                high = mid - 1;
                            }
                        } else {
                            high = mid - 1;
                        }
                    }
                    chars_to_render = low;
                }
            }

            // Copy the fitting portion to the new buffer
            if (chars_to_render > 0 && new_line_count < MAX_LINES) {
                strncpy(temp_buffers[new_line_count], text + start, chars_to_render);
                temp_buffers[new_line_count][chars_to_render] = '\0';
                temp_editable[new_line_count] = is_line_editable[i];
                // Create texture
                surface = TTF_RenderText_Solid(font, temp_buffers[new_line_count], strlen(temp_buffers[new_line_count]), white);
                if (surface) {
                    temp_textures[new_line_count] = SDL_CreateTextureFromSurface(renderer, surface);
                    SDL_DestroySurface(surface);
                }
                new_line_count++;
            }
            start += chars_to_render;
        }
    }

    // Update global state
    for (int i = 0; i < MAX_LINES; i++) {
        if (textures[i]) {
            SDL_DestroyTexture(textures[i]);
            textures[i] = NULL;
        }
        text_buffers[i][0] = '\0';
        is_line_editable[i] = false;
    }
    for (int i = 0; i < new_line_count; i++) {
        strcpy(text_buffers[i], temp_buffers[i]);
        textures[i] = temp_textures[i];
        is_line_editable[i] = temp_editable[i];
    }

    // Adjust current_line and cursor_pos
    current_line = new_line_count - 1;
    if (current_line < 0) current_line = 0;
    is_line_editable[current_line] = true;
    cursor_pos = strlen(text_buffers[current_line]);

    // Adjust scroll_offset
    if (current_line >= scroll_offset + LINES_PER_SCREEN) {
        scroll_offset = current_line - LINES_PER_SCREEN + 1;
    }
    if (scroll_offset < 0) scroll_offset = 0;
}

// Command implementations
void cmd_clear(void) {
    for (int i = 0; i < MAX_LINES; i++) {
        text_buffers[i][0] = '\0';
        if (textures[i]) {
            SDL_DestroyTexture(textures[i]);
            textures[i] = NULL;
        }
        is_line_editable[i] = false;
    }
    current_line = 0;
    scroll_offset = 0;
    cursor_pos = 0;
    history_pos = -1;
    is_line_editable[0] = true; // New input line is editable
}

void cmd_exit(void) {
    if (running) *running = false;
}

void cmd_help(void) {
    if (current_line < MAX_LINES - 1) {
        // Move to next line for help output
        current_line++;
        // Build help text
        char help_text[MAX_TEXT_LENGTH] = "Commands: ";
        int first = 1;
        for (int i = 0; i < num_commands; i++) {
            if (commands[i].description) { // Only include commands with descriptions
                if (!first) {
                    strcat(help_text, ", ");
                }
                strcat(help_text, commands[i].name);
                first = 0;
            }
        }
        // Copy to buffer
        strcpy(text_buffers[current_line], help_text);
        is_line_editable[current_line] = false; // Help output is not editable
        // Update texture
        if (textures[current_line]) SDL_DestroyTexture(textures[current_line]);
        SDL_Surface *surface = TTF_RenderText_Solid(font, text_buffers[current_line], strlen(text_buffers[current_line]), white);
        if (!surface) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Help text rendering failed: %s", SDL_GetError());
            return;
        }
        textures[current_line] = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_DestroySurface(surface);
        if (!textures[current_line]) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Help texture creation failed: %s", SDL_GetError());
            return;
        }
        // Prepare next line for input
        if (current_line < MAX_LINES - 1) {
            current_line++;
            text_buffers[current_line][0] = '\0';
            if (textures[current_line]) SDL_DestroyTexture(textures[current_line]);
            textures[current_line] = NULL;
            is_line_editable[current_line] = true; // Input line is editable
            cursor_pos = 0;
            if (current_line >= scroll_offset + LINES_PER_SCREEN) {
                scroll_offset++;
            }
        }
    }
}

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
    if (!SDL_CreateWindowAndRenderer("SDL3 Terminal Test", INITIAL_SCREEN_WIDTH, 600, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Window/Renderer creation failed: %s", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // Load font
    printf("TTF_OpenFont\n");
    font = TTF_OpenFont("Kenney Pixel.ttf", 16);
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

    // Initialize text buffer with welcome message
    strcpy(text_buffers[0], "SDL3 terminal. License: MIT");
    strcpy(text_buffers[1], "Simple test terminal emulator.");
    is_line_editable[0] = false; // Welcome message not editable
    is_line_editable[1] = false; // Welcome message not editable
    current_line = 2;
    scroll_offset = 0;
    cursor_pos = 0;
    is_line_editable[2] = true; // Input line is editable
    SDL_Color black = {0, 0, 0, 255};
    bool cursor_visible = true;
    Uint32 last_cursor_toggle = 0;

    // Initial text rendering for welcome message
    for (int i = 0; i < 2; i++) {
        SDL_Surface *surface = TTF_RenderText_Solid(font, text_buffers[i], strlen(text_buffers[i]), white);
        if (!surface) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Text rendering failed: %s", SDL_GetError());
            TTF_CloseFont(font);
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            TTF_Quit();
            SDL_Quit();
            return 1;
        }
        textures[i] = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_DestroySurface(surface);
        if (!textures[i]) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Texture creation failed: %s", SDL_GetError());
            TTF_CloseFont(font);
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            TTF_Quit();
            SDL_Quit();
            return 1;
        }
    }
    // Initialize input line
    text_buffers[2][0] = '\0';
    textures[2] = NULL;

    // Set running pointer for commands
    bool is_running = true;
    running = &is_running;

    // Main loop
    while (is_running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    is_running = false;
                    break;
                case SDL_EVENT_WINDOW_RESIZED: {
                    // Update max_text_width based on new window size
                    int new_width;
                    SDL_GetWindowSize(window, &new_width, NULL);
                    max_text_width = new_width - TEXT_MARGIN;
                    if (max_text_width < 10) max_text_width = 10; // Minimum width
                    rewrap_text();
                    break;
                }
                case SDL_EVENT_MOUSE_WHEEL: {
                    // Handle mouse wheel scrolling
                    int y = event.wheel.y;
                    if (y > 0) { // Scroll up
                        if (scroll_offset > 0) {
                            scroll_offset--;
                        }
                    } else if (y < 0) { // Scroll down
                        int max_offset = current_line - LINES_PER_SCREEN + 1;
                        if (max_offset < 0) max_offset = 0;
                        if (scroll_offset < max_offset) {
                            scroll_offset++;
                        }
                    }
                    break;
                }
                case SDL_EVENT_TEXT_INPUT: {
                    // Check if adding text exceeds screen width
                    size_t current_len = strlen(text_buffers[current_line]);
                    size_t input_len = strlen(event.text.text);
                    if (current_len + input_len < MAX_TEXT_LENGTH - 1 && is_line_editable[current_line]) {
                        // Create temporary buffer with new text inserted
                        char temp[MAX_TEXT_LENGTH] = {0};
                        strncpy(temp, text_buffers[current_line], cursor_pos);
                        strcat(temp, event.text.text);
                        strcat(temp, &text_buffers[current_line][cursor_pos]);
                        // Measure width
                        SDL_Surface *surface = TTF_RenderText_Solid(font, temp, strlen(temp), white);
                        bool wrap = false;
                        if (surface) {
                            SDL_Texture *temp_texture = SDL_CreateTextureFromSurface(renderer, surface);
                            SDL_DestroySurface(surface);
                            if (temp_texture) {
                                float text_width;
                                SDL_GetTextureSize(temp_texture, &text_width, NULL);
                                SDL_DestroyTexture(temp_texture);
                                if (text_width > max_text_width && current_line < MAX_LINES - 1) {
                                    // Wrap to next line
                                    current_line++;
                                    text_buffers[current_line][0] = '\0';
                                    cursor_pos = 0;
                                    if (textures[current_line]) SDL_DestroyTexture(textures[current_line]);
                                    textures[current_line] = NULL;
                                    is_line_editable[current_line] = true;
                                    if (current_line >= scroll_offset + LINES_PER_SCREEN) {
                                        scroll_offset++;
                                    }
                                    wrap = true;
                                }
                            }
                        }
                        // Insert text at cursor position
                        if (!wrap) {
                            memmove(&text_buffers[current_line][cursor_pos + input_len],
                                    &text_buffers[current_line][cursor_pos],
                                    current_len - cursor_pos + 1);
                            memcpy(&text_buffers[current_line][cursor_pos], event.text.text, input_len);
                            cursor_pos += input_len;
                        } else {
                            strcpy(text_buffers[current_line], event.text.text);
                            cursor_pos = input_len;
                        }
                        history_pos = -1; // Reset history position
                        // Update texture for current line
                        if (textures[current_line]) SDL_DestroyTexture(textures[current_line]);
                        if (strlen(text_buffers[current_line]) > 0) {
                            SDL_Surface *surface = TTF_RenderText_Solid(font, text_buffers[current_line], strlen(text_buffers[current_line]), white);
                            if (!surface) {
                                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Text rendering failed: %s", SDL_GetError());
                                is_running = false;
                                break;
                            }
                            textures[current_line] = SDL_CreateTextureFromSurface(renderer, surface);
                            SDL_DestroySurface(surface);
                            if (!textures[current_line]) {
                                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Texture creation failed: %s", SDL_GetError());
                                is_running = false;
                            }
                        } else {
                            textures[current_line] = NULL;
                        }
                    }
                    break;
                }
                case SDL_EVENT_KEY_DOWN:
                    if (event.key.key == SDLK_BACKSPACE) {
                        if (cursor_pos > 0 && is_line_editable[current_line]) {
                            // Remove character before cursor
                            memmove(&text_buffers[current_line][cursor_pos - 1],
                                    &text_buffers[current_line][cursor_pos],
                                    strlen(text_buffers[current_line]) - cursor_pos + 1);
                            cursor_pos--;
                            history_pos = -1;
                            // Update texture
                            if (textures[current_line]) SDL_DestroyTexture(textures[current_line]);
                            if (strlen(text_buffers[current_line]) > 0) {
                                SDL_Surface *surface = TTF_RenderText_Solid(font, text_buffers[current_line], strlen(text_buffers[current_line]), white);
                                if (!surface) {
                                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Text rendering failed: %s", SDL_GetError());
                                    is_running = false;
                                    break;
                                }
                                textures[current_line] = SDL_CreateTextureFromSurface(renderer, surface);
                                SDL_DestroySurface(surface);
                                if (!textures[current_line]) {
                                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Texture creation failed: %s", SDL_GetError());
                                    is_running = false;
                                    break;
                                }
                            } else {
                                textures[current_line] = NULL;
                            }
                        }
                        // Prevent moving to previous line if it's not editable
                    } else if (event.key.key == SDLK_DELETE) {
                        // Remove character at cursor if not at end
                        if (cursor_pos < (int)strlen(text_buffers[current_line]) && is_line_editable[current_line]) {
                            memmove(&text_buffers[current_line][cursor_pos],
                                    &text_buffers[current_line][cursor_pos + 1],
                                    strlen(text_buffers[current_line]) - cursor_pos);
                            history_pos = -1;
                            // Update texture
                            if (textures[current_line]) SDL_DestroyTexture(textures[current_line]);
                            if (strlen(text_buffers[current_line]) > 0) {
                                SDL_Surface *surface = TTF_RenderText_Solid(font, text_buffers[current_line], strlen(text_buffers[current_line]), white);
                                if (!surface) {
                                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Text rendering failed: %s", SDL_GetError());
                                    is_running = false;
                                    break;
                                }
                                textures[current_line] = SDL_CreateTextureFromSurface(renderer, surface);
                                SDL_DestroySurface(surface);
                                if (!textures[current_line]) {
                                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Texture creation failed: %s", SDL_GetError());
                                    is_running = false;
                                    break;
                                }
                            } else {
                                textures[current_line] = NULL;
                            }
                        }
                    } else if (event.key.key == SDLK_LEFT) {
                        // Move cursor left
                        if (cursor_pos > 0) {
                            cursor_pos--;
                            history_pos = -1;
                        }
                    } else if (event.key.key == SDLK_UP) {
                        // Recall previous command
                        if (history_count > 0 && history_pos < history_count - 1 && is_line_editable[current_line]) {
                            history_pos++;
                            strcpy(text_buffers[current_line], command_history[history_count - 1 - history_pos]);
                            cursor_pos = strlen(text_buffers[current_line]);
                            // Update texture
                            if (textures[current_line]) SDL_DestroyTexture(textures[current_line]);
                            if (strlen(text_buffers[current_line]) > 0) {
                                SDL_Surface *surface = TTF_RenderText_Solid(font, text_buffers[current_line], strlen(text_buffers[current_line]), white);
                                if (!surface) {
                                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Text rendering failed: %s", SDL_GetError());
                                    is_running = false;
                                    break;
                                }
                                textures[current_line] = SDL_CreateTextureFromSurface(renderer, surface);
                                SDL_DestroySurface(surface);
                                if (!textures[current_line]) {
                                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Texture creation failed: %s", SDL_GetError());
                                    is_running = false;
                                    break;
                                }
                            } else {
                                textures[current_line] = NULL;
                            }
                        }
                    } else if (event.key.key == SDLK_DOWN) {
                        // Recall next command or clear input
                        if (history_pos >= 0 && is_line_editable[current_line]) {
                            history_pos--;
                            if (history_pos >= 0) {
                                strcpy(text_buffers[current_line], command_history[history_count - 1 - history_pos]);
                            } else {
                                text_buffers[current_line][0] = '\0';
                            }
                            cursor_pos = strlen(text_buffers[current_line]);
                            // Update texture
                            if (textures[current_line]) SDL_DestroyTexture(textures[current_line]);
                            if (strlen(text_buffers[current_line]) > 0) {
                                SDL_Surface *surface = TTF_RenderText_Solid(font, text_buffers[current_line], strlen(text_buffers[current_line]), white);
                                if (!surface) {
                                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Text rendering failed: %s", SDL_GetError());
                                    is_running = false;
                                    break;
                                }
                                textures[current_line] = SDL_CreateTextureFromSurface(renderer, surface);
                                SDL_DestroySurface(surface);
                                if (!textures[current_line]) {
                                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Texture creation failed: %s", SDL_GetError());
                                    is_running = false;
                                    break;
                                }
                            } else {
                                textures[current_line] = NULL;
                            }
                        }
                    } else if (event.key.key == SDLK_RETURN && current_line < MAX_LINES - 1) {
                        // Check for commands
                        bool is_command = false;
                        bool is_clear = false;
                        for (int i = 0; i < num_commands; i++) {
                            if (strcmp(text_buffers[current_line], commands[i].name) == 0) {
                                if (strcmp(commands[i].name, "clear") == 0) {
                                    is_clear = true;
                                }
                                commands[i].function();
                                is_command = true;
                                break;
                            }
                        }
                        if (!is_command && strlen(text_buffers[current_line]) > 0) {
                            // Store in history if not empty
                            printf("Parsed input: %s\n", text_buffers[current_line]);
                            if (history_count < MAX_HISTORY) {
                                command_history[history_count] = strdup(text_buffers[current_line]);
                                if (command_history[history_count]) {
                                    history_count++;
                                }
                            } else {
                                // Free oldest command and shift
                                free(command_history[0]);
                                memmove(&command_history[0], &command_history[1], (MAX_HISTORY - 1) * sizeof(char *));
                                command_history[MAX_HISTORY - 1] = strdup(text_buffers[current_line]);
                            }
                            // Move to next line
                            current_line++;
                            text_buffers[current_line][0] = '\0';
                            if (textures[current_line]) SDL_DestroyTexture(textures[current_line]);
                            textures[current_line] = NULL;
                            is_line_editable[current_line] = true;
                            cursor_pos = 0;
                            history_pos = -1;
                            // Adjust scroll offset if needed
                            if (current_line >= scroll_offset + LINES_PER_SCREEN) {
                                scroll_offset++;
                            }
                        } else if (is_command && !is_clear) {
                            // Move to next line after command, except for clear
                            // (handled in cmd_help for help command)
                            if (strcmp(text_buffers[current_line], "help") != 0 &&
                                strcmp(text_buffers[current_line], "-help") != 0 &&
                                strcmp(text_buffers[current_line], "-h") != 0) {
                                current_line++;
                                text_buffers[current_line][0] = '\0';
                                if (textures[current_line]) SDL_DestroyTexture(textures[current_line]);
                                textures[current_line] = NULL;
                                is_line_editable[current_line] = true;
                                cursor_pos = 0;
                                history_pos = -1;
                                if (current_line >= scroll_offset + LINES_PER_SCREEN) {
                                    scroll_offset++;
                                }
                            }
                        }
                        // If clear was executed, no need to move to next line (handled in cmd_clear)
                        break;
                    }
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
                SDL_Surface *surface = TTF_RenderText_Solid(font, temp, strlen(temp), white);
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
    for (int i = 0; i < history_count; i++) {
        free(command_history[i]);
    }
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}