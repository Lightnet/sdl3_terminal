# sdl3_terminal

# License: MIT

# Information:
    Prototype terminal emulator place holder by using the SDL Render test. Just simple code test how terminal works.

## Features

- Renders text using the "Kenney Mini" font with SDL_ttf.
- Creates a window with SDL3 to display top left text ("Hello, Terminal!").
- Configured for static linking of SDL3, SDL_ttf, and FreeType to avoid DLL dependencies.
- Built using CMake and MinGW-w64 for cross-platform compatibility.
- it use render text as texture base on screen window size.

- case sensitive characters
- scroll text when move
- Command history with up/down arrow navigation (MAX_HISTORY=50).
- Text wrapping (790px), backspace across lines, cursor movement (left arrow), delete, and Enter.
- Multi-line text with scrolling (MAX_LINES=100, LINES_PER_SCREEN=30).
- Blinking cursor using SDL_GetTextureSize.

# simple terminal
    Testing how it work some logic.
## commands:
```
clear
```
Clear all lines.
```
exit
```
Close application.

## Goals

- Provide a working example of SDL2 and SDL_ttf integration.
- terminal emulator.
- Ensure a straightforward setup process for Windows developers using MSYS2.

## Requirements

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
.\sdl2_terminal.exe
```
Run Application.

# Credits

- Kenney Fonts: The "Kenney Mini.ttf" font is provided by [Kenney](https://kenney.nl/assets/kenney-fonts).
- SDL2: [Simple DirectMedia Layer](https://www.libsdl.org/).
- SDL_ttf: [SDL_ttf Library](https://github.com/libsdl-org/SDL_ttf).
- FreeType: [FreeType Project](https://www.freetype.org/).

