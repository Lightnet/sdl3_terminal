# SDL3 Terminal Emulator Documentation

This document provides a comprehensive guide to the SDL3 terminal emulator implemented in C, detailing its functionality, how to use it, how to extend it with new commands, and its internal workings. It includes a visual diagram illustrating the flow of input processing and text rendering.

## Table of Contents

1. [Overview](#overview)
2. [How It Works](#how-it-works)
3. [Using the Terminal](#using-the-terminal)
4. [Adding New Commands](#adding-new-commands)
5. [Visual Diagram: Input and Rendering Flow](#visual-diagram-input-and-rendering-flow)
6. [Internal Details](#internal-details)
7. [Compilation and Dependencies](#compilation-and-dependencies)
8. [Troubleshooting](#troubleshooting)
    

## Overview

The SDL3 terminal emulator is a lightweight, resizable terminal application built using SDL3 and SDL3_ttf libraries. It provides a text-based interface for entering commands, displaying output, and maintaining a history of inputs and outputs. Key features include:

- Commands: Built-in commands (clear, exit, help, echo) with support for custom extensions.
- Line Management: Supports up to MAX_LINES (100) lines, automatically removing the oldest line when the limit is reached.
- Text Wrapping: Automatically wraps text when it exceeds the window width.
- Command History: Stores up to MAX_HISTORY (50) commands, accessible via up/down arrow keys.
- Scrolling: Supports mouse wheel scrolling to view previous lines.
- Resizable Window: Adjusts text wrapping on window resize.
- Cursor: Blinking cursor for input, movable with left arrow, backspace, and delete keys.

The terminal is designed for simplicity and extensibility, suitable for educational purposes or as a base for more complex applications.

## How It Works

The terminal operates as an event-driven SDL3 application with the following core components:

1. Initialization:
    - Initializes SDL3 and SDL3_ttf.
    - Creates an 800x600 resizable window titled "SDL3 Terminal Test".
    - Loads the "Kenney Pixel.ttf" font (16pt).
    - Displays a welcome message: "SDL3 terminal. License: MIT\nSimple test terminal emulator."
2. Text Storage:
    - Stores lines in text_buffers[MAX_LINES][MAX_TEXT_LENGTH] (100 lines, 256 chars each).
    - Associates each line with an SDL texture (textures[MAX_LINES]) for rendering.
    - Tracks editability (is_line_editable[MAX_LINES]) to distinguish input lines from output.
3. Input Handling:
    - Processes keyboard input via SDL events (SDL_EVENT_TEXT_INPUT, SDL_EVENT_KEY_DOWN).
    - Supports text input, backspace, delete, left arrow, up/down for history, and Enter to execute commands.
    - Wraps text to a new line if input exceeds the window width (max_text_width).
4. Command Processing:
    - Parses input against a command table (commands[]).
    - Executes matching commands (clear, exit, help, echo) or treats input as non-command text.
    - Stores non-empty inputs in command_history[MAX_HISTORY].
5. Line Management:
    - When current_line reaches MAX_LINES - 1, the oldest line is removed via shift_lines_up().
    - Shifts text_buffers, textures, and is_line_editable up, clearing the last line.
6. Rendering:
    - Clears the screen with a black background.
    - Renders visible lines (up to LINES_PER_SCREEN, 30) starting from scroll_offset.
    - Displays a blinking cursor at the current input position.
7. Scrolling:
    - Adjusts scroll_offset via mouse wheel or when new lines are added beyond the visible area.
8. Cleanup:
    - Frees textures, command history, font, renderer, and window on exit.

### Using the Terminal

#### Running the Terminal

1. Compile the program (see Compilation and Dependencies (#compilation-and-dependencies)).
2. Ensure "Kenney Pixel.ttf" is in the executable’s directory.
3. Run the executable. The terminal opens with:
    
```text
SDL3 terminal. License: MIT
Simple test terminal emulator.
[cursor]
```
    

Built-in Commands

Commands are case-sensitive and entered at the cursor. Press Enter to execute.

| Command         | Description                                        | Example Usage  | Output Example                    |
| --------------- | -------------------------------------------------- | -------------- | --------------------------------- |
| clear           | Clears all text and resets the terminal.           | clear          | [cursor]                          |
| exit            | Closes the application.                            | exit           | (Application exits)               |
| help, -help, -h | Lists available commands.                          | help           | Commands: clear, exit, help, echo |
| echo <text>     | Prints<br><br><text><br><br>or empty line if none. | echo test test | test test                         |

- Example Interaction:
    
```text
SDL3 terminal. License: MIT
Simple test terminal emulator.
echo hello
hello
help
Commands: clear, exit, help, echo
[cursor]
```
    

## Features

- Input:
    - Type text in the current line (editable, marked by is_line_editable[current_line] = true).
    - Use Backspace to delete the previous character, Delete to remove the next character, Left Arrow to move the cursor.
    - Up/Down arrow keys navigate command history.
- Text Wrapping:
    - If input exceeds max_text_width, it wraps to a new line automatically.
- History:
    - Non-empty inputs are stored (up to 50).
    - Access previous/next commands with Up/Down arrows.
- Scrolling:
    - Mouse wheel scrolls up/down through lines.
    - New lines auto-scroll to keep the cursor visible.
- Resizing:
    - Resize the window; text re-wraps to fit the new width.

### Line Limit

- The terminal supports MAX_LINES = 100 lines.
- When a new line is needed and current_line = 99, the oldest line is removed (text_buffers[0]), and lines shift up.
- Example:
    - After 100 echo test commands, the welcome message is gone, and the last line shows the latest output.

### Adding New Commands

To extend the terminal with new commands, modify the command table and implement the command logic.

##### Steps
 - Define the Command Function:
    - Create a function with the signature void cmd_<name>(const char *input).
    - Use input to process arguments and add output to text_buffers.
    - Example: Add a date command to show the current date:

c
```c
void cmd_date(const char *input) {
	if (current_line <>= MAX_LINES - 1) {
		shift_lines_up();
	} else {
		current_line++;
	}
	time_t now = time(NULL);
	char *time_str = ctime(&now);
	time_str[strlen(time_str) - 1] = '\0'; // Remove newline
	strcpy(text_buffers[current_line], time_str);
	is_line_editable[current_line] = false; // Output not editable
	// Update texture
	if (textures[current_line]) SDL_DestroyTexture(textures[current_line]);
	SDL_Surface *surface = TTF_RenderText_Solid(font, text_buffers[current_line], strlen(text_buffers[current_line]), white);
	if (!surface) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Date text rendering failed: %s", SDL_GetError());
		return;
	}
	textures[current_line] = SDL_CreateTextureFromSurface(renderer, surface);
	SDL_DestroySurface(surface);
	if (!textures[current_line]) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Date texture creation failed: %s", SDL_GetError());
	}
}
```
        
- Add to Command Table:
    - In the commands array, add an entry:
c
```c
static const Command commands[] = {
	{"clear", cmd_clear, "Clear all text in the terminal"},
	{"exit", cmd_exit, "Exit the application"},
	{"help", cmd_help, "List available commands"},
	{"-help", cmd_help, NULL},
	{"-h", cmd_help, NULL},
	{"echo", cmd_echo, "Print the following text"},
	{"date", cmd_date, "Display current date and time"},
};
```
 - Update num_commands:

c
```c
static const int num_commands = sizeof(commands) / sizeof(commands[0]);
```
        
- Declare the Function:   
    - Add the prototype above the command table:
        
c
```c
void cmd_date(const char *input);
```
        
- Test the Command:
    - Compile and run.        
    - Enter date:
        
```text
Mon May 26 09:20:00 2025
[cursor]
```

# Guidelines

- Command Naming: Use lowercase, avoid spaces. Aliases (e.g., -h for help) can be added without descriptions.
- Line Management: Always check current_line >= MAX_LINES - 1 and call shift_lines_up() if needed.
- Output: Write to text_buffers[current_line], set is_line_editable[current_line] = false, and update textures[current_line].
- Error Handling: Check SDL_Surface and SDL_Texture creation, log errors with SDL_LogError.
- Arguments: Parse input after the command name (e.g., input + strlen("date")).

## Visual Diagram: Input and Rendering Flow

Below is a textual representation of the input processing and text rendering flow. Due to text-based limitations, this is a simplified ASCII diagram with a detailed explanation.

```text
[User Input]
    |
    v
[SDL Event Loop]
    | SDL_EVENT_TEXT_INPUT: Add text to text_buffers[current_line]
    | SDL_EVENT_KEY_DOWN:
    |   - Backspace/Delete: Modify text_buffers
    |   - Left/Up/Down: Move cursor or navigate history
    |   - Return: Process command or non-command input
    v
[Command Processing]
    | Match input against commands[]
    |   - Found: Execute cmd_<name>(input)
    |     e.g., cmd_echo -> Write output to text_buffers
    |   - Not Found: Store input in history
    v
[Line Management]
    | If current_line >= MAX_LINES - 1:
    |   shift_lines_up() -> Remove oldest line
    | Else:
    |   current_line++
    | Update text_buffers, textures, is_line_editable
    v
[Text Wrapping]
    | If text width > max_text_width:
    |   Create new line (shift if needed)
    |   Adjust scroll_offset
    v
[Rendering]
    | Clear screen (black)
    | Render textures[scroll_offset] to [scroll_offset + LINES_PER_SCREEN]
    | Draw blinking cursor at cursor_pos
    | SDL_RenderPresent
```

# Explanation

- User Input:
    - User types text or presses keys (e.g., echo test test, Enter, Up arrow).    
- SDL Event Loop:
    - SDL_EVENT_TEXT_INPUT: Adds characters to text_buffers[current_line] at cursor_pos.
    - SDL_EVENT_KEY_DOWN:
        - Backspace/Delete modifies the current line.
        - Left arrow moves cursor_pos.
        - Up/Down arrows navigate command_history.
        - Enter triggers command processing.
- Command Processing:
    - Input is compared to commands[] (e.g., echo).
    - If matched, the command function (e.g., cmd_echo) is called, writing output to a new line.
    - If not matched, input is stored in command_history and a new input line is created.
- Line Management:
    - Before adding a new line, check if current_line >= MAX_LINES - 1.
    - If true, call shift_lines_up() to remove the oldest line.
    - Update text_buffers, textures, and is_line_editable.
- Text Wrapping:
    - During input, if text width exceeds max_text_width, a new line is created (shifting if needed).
- Rendering:
    - Clear the screen.
    - Render up to 30 lines (LINES_PER_SCREEN) starting from scroll_offset.
    - Draw a blinking cursor based on cursor_pos and current_line.
    - Present the frame.

# Internal Details

## Key Data Structures
- text_buffers[MAX_LINES][MAX_TEXT_LENGTH]: Stores lines (100 lines, 256 chars each).
- textures[MAX_LINES]: SDL textures for rendering each line.
- is_line_editable[MAX_LINES]: Flags indicating if a line is editable (true for input lines).
- command_history[MAX_HISTORY]: Stores up to 50 previous inputs.
- commands[]: Array of Command structs (name, function, description).
- current_line: Index of the current line (0 to 99).
- scroll_offset: Index of the first visible line.
- cursor_pos: Cursor position within text_buffers[current_line].
- max_text_width: Maximum text width (window width - TEXT_MARGIN).

## Core Functions
- shift_lines_up(): Removes the oldest line when MAX_LINES is reached.
- rewrap_text(): Re-wraps all lines on window resize to fit max_text_width.
- cmd_clear(): Clears all lines and resets state.
- cmd_exit(): Sets running = false to exit.
- cmd_help(): Lists commands with descriptions.
- cmd_echo(): Outputs text after echo.

## Event Handling
- SDL_EVENT_TEXT_INPUT: Inserts text, wraps if needed.
- SDL_EVENT_KEY_DOWN: Handles backspace, delete, cursor movement, history navigation, and Enter.
- SDL_EVENT_WINDOW_RESIZED: Updates max_text_width and re-wraps text.
- SDL_EVENT_MOUSE_WHEEL: Adjusts scroll_offset.
- SDL_EVENT_QUIT: Exits the application.

## Rendering
- Clears with SDL_SetRenderDrawColor(black).
- Renders textures in a loop: for (i = scroll_offset; i <= current_line && i < scroll_offset + LINES_PER_SCREEN; i++).
- Draws a 16px white cursor, blinking every 500ms (CURSOR_BLINK_MS).

### Compilation and Dependencies

#### Requirements

- SDL3: Graphics and event handling.
- SDL3_ttf: Font rendering.
- Font: "Kenney Pixel.ttf" (16pt, in executable directory).
- Compiler: GCC/MinGW or equivalent.
    
#### Compilation

With MinGW:

bash
```bash
gcc -o terminal_test main.c -I/path/to/SDL3/include -I/path/to/SDL3_ttf/include -L/path/to/SDL3/lib -L/path/to/SDL3_ttf/lib -lSDL3 -lSDL3_ttf
```

With pkg-config:

bash
```bash
gcc -o terminal_test main.c $(pkg-config --cflags --libs sdl3 SDL3_ttf)
```

#### Dependencies Path
- Example: sdl3_terminal/build/_deps/.
- Ensure SDL3 and SDL3_ttf headers/libraries are accessible.
    

#### Troubleshooting

##### Common Issues

- Crash on echo test test:
    - Ensure command_index is used in SDLK_RETURN (fixed in updated code).
    - Add logging:
        
c
```c
printf("RETURN: input='%s', current_line=%d\n", text_buffers[current_line], current_line);
```
        
- Extra Line Breaks:
    - Verify cmd_echo and cmd_help don’t include the extra line break snippet.
    - Check SDLK_RETURN for correct line advancement.
- Font Not Found:
    - Place "Kenney Pixel.ttf" in the executable directory.
    - Fallback to arial.ttf:

c
```c
font = TTF_OpenFont("arial.ttf", 16);
```
        
- Lines Not Removed:
    - Log shift_lines_up:
        
c
```c
printf("Shifting: current_line=%d\n", current_line);
```
        
    - Ensure current_line >= MAX_LINES - 1 checks are present.
        
5. SDL Errors:
    - Check SDL_GetError() after texture/surface creation.
    - Example:
        
c
```c
SDL_Log("SDL Error: %s\n", SDL_GetError());
```

#### Debugging Tips

- Log Input:
    
c
```c
case SDL_EVENT_TEXT_INPUT:
	printf("Input: %s, current_line=%d\n", event.text.text, current_line);
```
    
- Log Rendering:
    
c
```c
printf("Rendering: scroll_offset=%d, current_line=%d\n", scroll_offset, current_line);
```
    
- Test Line Limit:
    - Run echo test 100 times to verify line removal.
    - Check if the welcome message disappears.

# Getting Help

- Provide: 
    - Error messages or crash details.
    - Debug output from logs.
    - Steps to reproduce (e.g., specific commands).
- Contact: Share via the platform where this code was provided.
    

This documentation covers the SDL3 terminal emulator’s usage, extension, and internals, with a clear visual flow of input and rendering. Let me know if you need further details or additional features!