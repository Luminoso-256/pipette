/*
 * pipette
 * main.cpp
 * created 11-13-21
 -------------------
 (C) Luminoso 2021/All Rights Reserved
 * */
#include <iostream>
#include "include/kissnet/kissnet.hpp"

#undef DrawText
#undef DrawTextEx
#undef LoadImage
#undef RLCloseWindow
#undef ShowCursor
#undef Rectangle

#include "include/raylib/raylib.h"
#include "gemtxt.h"
#include <vector>
#include <thread>
#include <fstream>
#include <chrono>
#include <filesystem>
#include "portable_endian.h"

namespace kn = kissnet;
namespace gm = gemtext;
using namespace std::chrono_literals;

#define VERSION "21D318B-DEV"
#define SCREEN_WIDTH (800)
#define SCREEN_HEIGHT (450)
#define WINDOW_TITLE "Pipette - the tiny piper browser"
#define MAX_INPUT_CHARS  64


void browse(std::vector<gm::GemLine> *result, std::string url, int *status, int *contentstatus) {
    std::cout << "Navigating to url " << url << "\n";
    /* set up key strings */

    std::vector<std::string> parts = util::split(url, "/");
    std::string host = "";
    if (parts[0].find(":") != std::string::npos) {
        host = parts[0];
    } else {
        host = parts[0] + ":60";
    }
    //clean out the first part
    parts.erase(parts.begin());
    //pull together the URI
    std::string uri = "/" + util::join(parts, '/');

    /* socket time! */

    //== Send
    try {
        kn::tcp_socket tcpsocket((kn::endpoint(host)));
        if (!tcpsocket.is_valid()) {
            std::cout << "Invalid Socket!" << std::endl;
            *status = 3;
            return;
        }
        kn::socket_status sstat = tcpsocket.connect();
        std::vector<char> request;
        short len = (short) uri.length();
        request.push_back(char(len & 0xff));
        request.push_back(char((len >> 8) & 0xff));
        std::vector<char> uribytes(uri.begin(), uri.end());
        request.insert(request.end(), uribytes.begin(), uribytes.end());
        std::vector<std::byte> brequest;
        for (char c:request) {
            brequest.push_back(std::byte(c));
        }
        //abuse the C++ spec requiring vecs being stored contigously to convert
        //god i hate all the casts in this mess.
        tcpsocket.send(&brequest[0], brequest.size());
        //== Recieve
        bool continue_rec = true;
        bool has_parsed_size = false;
        std::byte static_buffer[16384];
        int data_size = 0;
        while(continue_rec) {
            auto [size, valid] = tcpsocket.recv(static_buffer,sizeof(static_buffer));
            data_size += size;
            if (valid) {
                //buffer magik:tm:
                if (!has_parsed_size && data_size >= 9){
                    //behold, cursedness!
                    uint64_t contentlen = (uint64_t)(static_buffer[1]) |
                            (uint64_t)(static_buffer[2]) |
                            (uint64_t)(static_buffer[3]) |
                            (uint64_t)(static_buffer[4]) |
                            (uint64_t)(static_buffer[5]) |
                            (uint64_t)(static_buffer[6]) |
                            (uint64_t)(static_buffer[7]) |
                            (uint64_t)(static_buffer[8]);
                    std::cout << "content len: " << contentlen << "\n";
                    //now do the array resizing (ugh)
                    //kn::buffer<9+contentlen> new_buffer;
                    has_parsed_size = true;
                }

                //find out if we're done
                if (valid.value == kn::socket_status::cleanly_disconnected) {
                    continue_rec = false;
                }
            } else {
                continue_rec = false;
            }
        }
        *status = 2;

        int contenttype = (int) static_buffer[0];
        *contentstatus = contenttype;
        std::string res = "";
        bool error = false;
        bool warn = false;
        for (int i = 9; i < data_size; i++) {
            if (sizeof(static_buffer)>i) {
                res += (char) static_buffer[i];
            } else{
                warn = true;
            }
        }

        switch (contenttype) {
            case 0x0:
                result->clear();
                *result = gm::parse(res, true);
                break;
            case 0x01:
                result->clear();
                *result = gm::parse(res, false);
                break;
            case 0x10: {
                result->clear();
                result->push_back(
                        gm::GemLine{
                                "Saving " + parts[parts.size() - 1] + " to ./downloads/" + parts[parts.size() - 1], 1,
                                ""
                        }
                );
                result->push_back(
                        gm::GemLine{
                                std::to_string(res.size()) + " bytes.", 6, ""
                        }
                );
                *status = 4;
                std::filesystem::path path{"./downloads"};
                path /= parts[parts.size() - 1];
                std::filesystem::create_directories(path.parent_path());
                std::ofstream file(path);
                file << res;
                file.close();
            }
                break;
            case 0x22:
                result->clear();
                result->push_back(
                        gm::GemLine{
                                "0x22 Resource Not Found", 1, ""
                        }
                );
                error = true;
                break;
            default:
                result->clear();

                result->push_back(
                        gm::GemLine{
                                "Unknown Content Type " + std::to_string(contenttype), 1, ""
                        }
                );
                break;
        }
        if (error) {
            *status = 3;
        } else if (warn){
            *status = 5;
        } else {
            *status = 0;
        }
    } catch (const std::exception &e) {
        *status = 3;
        result->clear();
        result->push_back(
                gm::GemLine{
                        "Error Encountered: " + std::string(e.what()), 0, ""
                }
        );
        return;
    }
}


int main(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, WINDOW_TITLE);
    char target_url[MAX_INPUT_CHARS + 1] = "\0";
    int letterCount = 0;
    int framesCounter = 0;
    int status = 0;
    int fontsize = 15;
    int contentstatus = 0;
    int scrollbarOffset = 0;
    bool debug = false;
    int scrollSpeed = 4;
    Font font = LoadFontEx("font.ttf", 32, 0, 250);

    // int rendermode = 1;
    std::vector<gm::GemLine> gemlines = gm::attributionTextGen();

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        int key = GetCharPressed();
        while (key > 0) {
            if ((key >= 32) && (key <= 125) && (letterCount < MAX_INPUT_CHARS)) {
                target_url[letterCount] = (char) key;
                target_url[letterCount + 1] = '\0';
                letterCount++;
            }
            key = GetCharPressed();
        }

        if (IsKeyDown(KEY_BACKSPACE) && framesCounter % 4 == 0) {
            letterCount--;
            if (letterCount < 0) letterCount = 0;
            target_url[letterCount] = '\0';
        }
        if (IsKeyPressed(KEY_ENTER) && status != 1) {
            status = 1;
            scrollbarOffset = 0;
            std::thread thread(browse, &gemlines, std::string(target_url), &status, &contentstatus);
            thread.detach();
        }
        scrollbarOffset -= (GetMouseWheelMove() * scrollSpeed);
        if (scrollbarOffset < 0) {
            scrollbarOffset = 0;
        }
        if (IsKeyDown(KEY_UP)) {
            fontsize += 1;
        }
        if (IsKeyDown(KEY_DOWN)) {
            fontsize -= 1;
        }

        if (IsKeyPressed(KEY_LEFT_BRACKET)) { debug = !debug; }
        if (IsKeyPressed(KEY_RIGHT_BRACKET)) {
            status = 1;
            std::thread thread(browse, &gemlines, std::string("localhost/big.txt"), &status, &contentstatus);
            thread.detach();
        }
        framesCounter++;

        //draw

        BeginDrawing();
        ClearBackground(RAYWHITE);
        //StatusBar
        DrawTextEx(font, "piper://", Vector2{5, 5}, 20, 2, GRAY);
        DrawTextEx(font, target_url, Vector2{100, 5}, 20, 2, BLUE);
        switch (status) {
            case 0:
                DrawTextEx(font, "idle", Vector2{(float)GetScreenWidth() - 50, 5}, 20, 2, BLACK);
                break;
            case 1:
                DrawTextEx(font, "load", Vector2{(float)GetScreenWidth() - 50, 5}, 20, 2, BLACK);
                break;
            case 2:
                DrawTextEx(font, "parse", Vector2{(float)GetScreenWidth() - 70, 5}, 20, 2, BLACK);
                break;
            case 3:
                DrawTextEx(font, "error", Vector2{(float)GetScreenWidth() - 70, 5}, 20, 2, RED);
                break;
            case 4:
                DrawTextEx(font, "save", Vector2{(float)GetScreenWidth() - 70, 5}, 20, 2, BLACK);
                break;
            case 5:
                //overflow!
                DrawTextEx(font, "ovfl!", Vector2{(float)GetScreenWidth() - 70, 5}, 20, 2,ORANGE);
                break;
        }
        DrawLine(0, 25, GetScreenWidth(), 25, GRAY);
        //Main
        int y = 35;
        //hacky way to implement scrolling
        y -= scrollbarOffset * fontsize;
        for (gm::GemLine line:gemlines) {
            bool render = true;
            if (y < 25) {
                render = false;
            }
            switch (line.rendertype) {
                case 0:
                    if (render) {
                        DrawTextEx(font, line.content.c_str(), Vector2{5, (float) y}, fontsize, 2, BLACK);
                    }
                    y += fontsize + 5;
                    break;
                case 1:
                    if (render) {
                        DrawTextEx(font, line.content.c_str(), Vector2{5, (float) y}, fontsize + 8, 2, BLACK);
                    }
                    y += fontsize + 13;
                    break;
                case 2:
                    if (render) {
                        DrawTextEx(font, line.content.c_str(), Vector2{5, (float) y}, fontsize + 4, 2, BLACK);
                    }
                    y += fontsize + 13;
                    break;
                case 3:
                    if (render) {
                        DrawTextEx(font, line.content.c_str(), Vector2{5, (float) y}, fontsize + 2, 2, BLACK);
                    }
                    y += fontsize + 7;
                    break;
                case 4:
                    if (render) {
                        DrawTextEx(font, line.content.c_str(), Vector2{5, (float) y}, fontsize, 2, GRAY);
                    }
                    y += fontsize + 5;
                    break;
                case 6:
                    if (render) {
                        DrawRectangle(5, y + 1, 3, fontsize - 2, GRAY);
                        DrawTextEx(font, line.content.c_str(), Vector2{10, (float) y}, fontsize, 2, GRAY);
                    }
                    y += fontsize + 5;
                    break;
                case 7:
                    if (render) {
                        float x1 = 5;
                        float y1 = y;
                        float x2 = 5 + (MeasureText(line.content.c_str(), fontsize) * 1.5);
                        float y2 = y + fontsize + 3;
                        Vector2 mousePos = GetMousePosition();
                        bool touching = (mousePos.x > x1 && mousePos.x < x2 && mousePos.y > y1 && mousePos.y < y2);
                        if (debug) {
                            if (touching) {
                                DrawRectangle(5, y, MeasureText(line.content.c_str(), fontsize) * 1.5, fontsize + 3,
                                              GREEN);
                            } else {
                                DrawRectangle(5, y, MeasureText(line.content.c_str(), fontsize) * 1.5, fontsize + 3,
                                              YELLOW);
                            }
                            DrawTextEx(font, ("[Dbg] " + line.metadata).c_str(), Vector2{5, (float) y - 10}, 10, 2,
                                       PURPLE);
                        }

                        if (touching && IsMouseButtonDown(MouseButton::MOUSE_LEFT_BUTTON)) {
                            std::string clean = line.metadata.replace(line.metadata.find("piper://"),
                                                                      sizeof("piper://") - 1, "");
                            std::cout << clean << std::endl;
                            strcpy(target_url, clean.c_str());
                            status = 1;
                            scrollbarOffset = 0;
                            std::thread thread(browse, &gemlines, clean, &status, &contentstatus);
                            thread.detach();
                        }
                        DrawTextEx(font, line.content.c_str(), Vector2{5, (float) y}, fontsize, 2, SKYBLUE);
                    }
                    y += fontsize + 5;
            }
        }
        if (gemlines.size() * 15 > GetScreenHeight()) {
            int estimatesize = gemlines.size() * 15;
            DrawRectangle(GetScreenWidth() - 10, 35 + scrollbarOffset, 10, (estimatesize / GetScreenHeight()) * 3, GRAY);
        }
        if (scrollbarOffset > GetScreenHeight()) {
            int estimatesize = gemlines.size() * 15;
            DrawRectangle(GetScreenWidth() - 10, GetScreenHeight()-((estimatesize / GetScreenHeight()) * 3), 10, (estimatesize / GetScreenHeight()) * 3, ORANGE);
            DrawTextEx(font, "!", Vector2{(float)GetScreenWidth() - 8, (float)GetScreenHeight() - 15}, 14, 2, WHITE);
        }
        if (debug) {
            std::string dbg =
                    "[Debug] ContentType: " + std::to_string(contentstatus) + " FPS: " + std::to_string(GetFPS()) +
                    " (of target 60) URLBuffer: " + std::to_string(letterCount) + "(of max 64) / Pipette " + VERSION;
            DrawTextEx(font, dbg.c_str(), Vector2{3, (float)GetScreenHeight() - 20}, 12, 2, PURPLE);
        }
        EndDrawing();
    }
    RLCloseWindow();
    return 0;
}

