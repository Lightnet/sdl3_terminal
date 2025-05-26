# sdl3_terminal

# License: MIT

## Overview

This is a prototype terminal emulator built using SDL3 and SDL3_ttf, designed as a simple test to explore terminal functionality. It renders text as textures within a resizable window, using the "Kenney Pixel" font. The project serves as a working example of SDL3 and SDL3_ttf integration, with a focus on straightforward setup and cross-platform compatibility using CMake and MinGW-w64.

It has no functions for terminal. It simple commands input test to see how it works for SDL3 Render Text.

## Features

- Text Rendering: Uses SDL3_ttf to render text with the "Kenney Pixel" font (16pt) as textures, dynamically sized based on the window dimensions (default: 800x600 pixels).
- Initial Display: Shows welcome message in the top-left corner (10px margin) upon launch.
- Input Handling:
    - Case-sensitive character input.
    - Cursor movement with the left arrow key for in-line editing.
    - Backspace deletes the character before the cursor, moving to the previous line if at the start (except on the first line).
    - Delete removes the character at the cursor position.
    - Text wrapping at 790px (window width minus 10px margin) to the next line.
- Multi-Line Support:
    - Supports up to 100 lines (MAX_LINES) with automatic scrolling after 30 visible lines (LINES_PER_SCREEN).
    - Lines are spaced 20 pixels apart vertically.
- Command History:
    - Stores up to 50 non-command inputs (MAX_HISTORY) for recall using up/down arrow keys.
    - Commands (clear, exit, help) are not stored in history to keep it clean.
- Blinking Cursor: A 16px vertical white cursor blinks every 500ms, positioned using SDL_GetTextureSize for accuracy.
- Commands: Supports clear, exit, and help (with aliases -help, -h) via a command table for extensibility.
- Build Configuration:
    - Statically linked with SDL3, SDL3_ttf, and FreeType to eliminate DLL dependencies.
    - Built with CMake and MinGW-w64 for Windows, with cross-platform compatibility.
- Resizable Window: Adjusts rendering to window size, maintaining text layout.
- Mouse Scroll texts
- Resize Window to readjust text lines.

## Commands

The terminal supports the following case-sensitive commands, entered by typing and pressing Enter:

- clear
    - Description: Clears all text in the terminal, resetting to a single empty line.
    - History: Not stored in command history.
- exit
    - Description: Closes the application.
    - History: Not stored in command history.
- help, -help, -h
    - Description: Displays a list of available commands ("Commands: clear, exit, help") on the next line, then moves to a new line for input.
    - History: Not stored in command history.
- echo text

### Usage

1. Run the Application:
    - Launch the executable to open an 800x600 window titled "SDL3 Terminal Test".
    - The initial text welcome message appears on the first two line.
2. Interact:
    - Type to input text at the blinking cursor.
    - Use Left Arrow to move the cursor left within the current line.
    - Press Backspace to delete the previous character or move to the previous line.
    - Press Delete to remove the character at the cursor.
    - Press Enter to submit the line:
        - Commands (clear, exit, help) execute their actions.
        - Non-command inputs are printed to the console and stored in history.
    - Use Up Arrow to recall previous inputs (excluding commands).
    - Use Down Arrow to move forward in history or clear the line at the end.
    - Text wraps to the next line if it exceeds 790px.
3. Close:
    - Use the exit command or close the window.


## Requirements

#### Prerequisites
- CMake: Version 3.10 or higher.
- MinGW-w64: For Windows builds (e.g., via MSYS2).
- SDL3, SDL3_ttf, FreeType: Source or prebuilt libraries (statically linked).
- Font: "Kenney Mini.ttf" in the executable’s directory (or update the path, e.g., C:/Windows/Fonts/arial.ttf).

### Software
- Operating System: Windows 10 or later (64-bit).
- MSYS2: A software distribution and building platform for Windows.
    - Download: [MSYS2 Installer](https://www.msys2.org/)
- CMake: Version 3.10 or later (recommended: latest version available via MSYS2).
- MinGW-w64 Toolchain: Provides GCC, G++, and other tools for compiling.
- Git: For cloning the project repository (optional, if using source control).

# Dependencies

The project uses the following libraries, which are automatically fetched and built via CMake’s FetchContent:

- SDL2: Version 2.32.6 (Simple DirectMedia Layer for window and graphics handling).
- SDL_ttf: Version 2.24.0 (Font rendering library for SDL2).
- FreeType: Version 2.13.3 (Font rendering engine used by SDL_ttf).
- Font: "Kenney Mini.ttf" (included in the project directory).

## MSYS2 Packages

Install the following packages in MSYS2 to ensure a complete build environment:

bash
```bash
pacman -Syu
pacman -S mingw-w64-x86_64-gcc
pacman -S mingw-w64-x86_64-g++
pacman -S mingw-w64-x86_64-cmake
pacman -S mingw-w64-x86_64-make
pacman -S mingw-w64-x86_64-pkgconf
```

# Setup Instructions

1. Install MSYS2
2. Download and install MSYS2 from [msys2.org](https://www.msys2.org/).
3. Open the MSYS2 MinGW 64-bit terminal (mingw64.exe) from the MSYS2 installation directory (e.g., C:\msys64\mingw64.exe).
4. Update the package database and install required tools:

bash
```bash
pacman -Syu
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-g++ mingw-w64-x86_64-cmake mingw-w64-x86_64-make mingw-w64-x86_64-pkgconf
```

build.bat
```text
@echo off
setlocal
set MSYS2_PATH=C:\msys64\mingw64\bin
set PATH=%MSYS2_PATH%;%PATH%
if not exist build mkdir build
cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=%MSYS2_PATH%\gcc.exe -DCMAKE_CXX_COMPILER=%MSYS2_PATH%\g++.exe
cmake --build . --config Debug
endlocal
```

cmd
```text
cd build
.\sdl_terminal.exe
```
Run Application.

# Credits

- Kenney Fonts: The "Kenney Mini.ttf" font is provided by [Kenney](https://kenney.nl/assets/kenney-fonts).
- SDL2: [Simple DirectMedia Layer](https://www.libsdl.org/).
- SDL_ttf: [SDL_ttf Library](https://github.com/libsdl-org/SDL_ttf).
- FreeType: [FreeType Project](https://www.freetype.org/).

