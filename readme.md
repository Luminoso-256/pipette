# Pipette 
a better, and more spec compliant piper client

## Licensing
Pipette code (written by me) is licensed under MIT, as noted in the source files.

Raylib code, found in the `lib/raylib` and `include/raylib` directories is licensed under Zlib by the Raylib Authors

Kissnet code, found in the `include/kissnet` directory, is licensed under MIT by the Kissnet Authors

## Here be dragons!
in case you're wondering why some raylib functions have been renamed, and why on the below exists:
```cpp
#undef DrawText
#undef DrawTextEx
#undef LoadImage
#undef RLCloseWindow
#undef ShowCursor
#undef Rectangle
```

Kissnet includes the win32 headers on windows, which have naming conflicts with raylib in places. Instead of attempting to find an
elegant solution, I simply resorted to #undef-ing things, and renaming some symbols. You've been warned.

## Building

The project uses cmake as it's build system. Generate cmake build files for your preffered compiler, and then compile!

note that the project is intended to use the Cascadia Code typeface (as is noted in the attribution page in the code). This is expected as a `font.ttf` file next to the executable

