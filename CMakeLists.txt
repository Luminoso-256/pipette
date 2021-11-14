cmake_minimum_required(VERSION 3.16)
project(pipette)

set(CMAKE_CXX_STANDARD 20)
add_subdirectory(lib/raylib)
add_executable(pipette
        main.cpp
        include/raylib/raylib.h
        include/raylib/raygui.h
        include/kissnet/kissnet.hpp
        gemtxt.h
        util.h
        )
target_link_libraries(pipette PRIVATE raylib)