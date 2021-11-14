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
#include <vector>
#include <thread>

#define VERSION "21D317-DEV"
#define SCREEN_WIDTH (800)
#define SCREEN_HEIGHT (450)
#define WINDOW_TITLE "Pipette - the tiny piper browser"
#define MAX_INPUT_CHARS     64

namespace kn = kissnet;
#include <chrono>
using namespace std::chrono_literals;


struct GemLine{
    std::string content;
    int rendertype; //0 = plain, 1 = Heading, 2 = Semiheading, 3 = SemiSemi heading, 4 = Monospace, 5 = List, 6 = Quote, 7 = Link
    std::string metadata;
};

std::vector<std::string> split (std::string s, std::string delimiter) {
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    std::string token;
    std::vector<std::string> res;
    while ((pos_end = s.find (delimiter, pos_start)) != std::string::npos) {
        token = s.substr (pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back (token);
    }
    res.push_back (s.substr (pos_start));
    return res;
}

std::string join(const std::vector<std::string>& v, char c) {
    std::string s = "";
    s.clear();
    for (auto p = v.begin();
         p != v.end(); ++p) {
        s += *p;
        if (p != v.end() - 1)
            s += c;
    }
    return s;
}

std::vector<GemLine> gemParse(std::string input,bool ignoreformatdirectives){
    std::vector<std::string> lines = split(input,std::string("\n"));
    std::vector<GemLine> gemlines;
    bool monomode = false;
    for (std::string line:lines){
        if (line.find("\r") != std::string::npos) {
            line = line.replace(line.find("\r"), sizeof("\r") - 1, "");
        }
        if (ignoreformatdirectives){
            gemlines.push_back(GemLine{
                    line, 0,""
            });
            continue;
        }
        if (line.starts_with("```")){
            monomode = !monomode;
            continue;
        }
        if (monomode){
            gemlines.push_back(GemLine{
               line, 4,""
            });
            continue;
        }
        if (line.starts_with("# ")){
            gemlines.push_back(GemLine{
                    line.replace(line.find("# "), sizeof("# ") - 1, ""), 1,""
            });
        } else if (line.starts_with("## ")){
            gemlines.push_back(GemLine{
                    line.replace(line.find("## "), sizeof("## ") - 1, ""), 2,""
            });
        } else if (line.starts_with("### ")){
            gemlines.push_back(GemLine{
                    line.replace(line.find("### "), sizeof("### ") - 1, ""), 3,""
            });
        } else if (line.starts_with("> ")){
            gemlines.push_back(GemLine{
                    line.replace(line.find("> "), sizeof("> ") - 1, ""), 6,""
            });
        } else if (line.starts_with("=> ")){
            line = line.replace(line.find("=> "), sizeof("=> ") - 1, "");
            std::vector<std::string> parts = split(line," ");
            if (parts.size() <= 1){
                gemlines.push_back(GemLine{
                        parts[0], 7, parts[0]
                });
            } else {
                std::string target = parts[0];
                parts.erase(parts.begin());
                std::string collected = join(parts, ' ');
                gemlines.push_back(GemLine{
                        collected, 7, target
                });
            }
        } else {
            gemlines.push_back(GemLine{
                    line, 0,""
            });
        }
    }
    return gemlines;
}

void browse(std::vector<GemLine> *result,std::string url,int *status,int *contentstatus){
    std::cout << "Navigating to url " << url << "\n";
    /* set up key strings */

    std::vector<std::string> parts = split(url,"/");
    std::string host = "";
    if (parts[0].find(":") != std::string::npos){
        host = parts[0];
    } else{
        host = parts[0] + ":60";
    }
    //clean out the first part
    parts.erase(parts.begin());
    //pull together the URI
    std::string uri = "/"+join(parts, '/');

    /* socket time! */

    //== Send

    kn::tcp_socket tcpsocket((kn::endpoint(host)));

    kn::socket_status sstat = tcpsocket.connect();

    if (! tcpsocket.is_valid()){
        *status = 3;
        return;
    }

    std::vector<char> request;
    short len = (short) uri.length();
    request.push_back(char(len & 0xff));
    request.push_back(char((len >> 8) & 0xff));
    std::vector<char> uribytes(uri.begin(), uri.end());
    request.insert(request.end(),uribytes.begin(),uribytes.end());
    std::vector<std::byte> brequest;
    for (char c:request){
        brequest.push_back(std::byte(c));
    }
    //abuse the C++ spec requiring vecs being stored contigously to convert
    //god i hate all the casts in this mess.
    tcpsocket.send(&brequest[0],brequest.size());
    while(tcpsocket.bytes_available() <= 9){}
    std::cout << "bytes available to read : " << tcpsocket.bytes_available() << '\n';
    //== Recieve
    kn::buffer<4096> static_buffer;
    const auto [data_size, status_code] = tcpsocket.recv(static_buffer);
    if(data_size < static_buffer.size())
        static_buffer[data_size] = std::byte{ '\0' };
    *status = 2;

    int contenttype = (int) static_buffer[0];
    *contentstatus = contenttype;
    std::string res = "";
    for (int i = 9; i<data_size; i++){
        res += (char)static_buffer[i];
    }
    switch (contenttype){
        case 0x0:
            result->clear();
            *result = gemParse(res,true);
            break;
        case 0x01:
            result->clear();
            *result = gemParse(res,false);
            break;
        case 0x22:
            result->clear();
            result->push_back(
                    GemLine{
                        "0x22 Resource Not Found",0,""
                    }
                    );
            break;
        default:
            result->clear();
            *result = gemParse(res,true);
            break;
    }

    *status = 0;
}

int main(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, WINDOW_TITLE);
    char target_url[MAX_INPUT_CHARS + 1] = "\0";
    int letterCount = 0;
    int framesCounter = 0;
    int status = 0;
    int fontsize = 15;
    int contentstatus = 0;
    bool debug = false;
    int scrollSpeed = 4;
    Font font = LoadFontEx("font.ttf",32,0,250);


   // int rendermode = 1;
    std::vector<GemLine> gemlines(1,GemLine{"waiting for browse activity\n\n\n\n--------------------\nPipette is (C) Luminoso 2021 All Rights Reserved\nPipette uses the kissnet library, which is under the MIT license\n (https://github.com/Ybalrid/kissnet)\nPipette uses the Raylib library, which is under the Zlib license\n (https://github.com/raysan5/raylib)\nPipette uses Cascadia Code, which is under the OFL1.1\n (https://github.com/microsoft/cascadia-code)",0,""});

    SetTargetFPS(30);

    while (!WindowShouldClose()) {

        fontsize -= (GetMouseWheelMove()*scrollSpeed);

        int key = GetCharPressed();
        while (key > 0) {
            if ((key >= 32) && (key <= 125) && (letterCount < MAX_INPUT_CHARS)) {
                target_url[letterCount] = (char) key;
                target_url[letterCount + 1] = '\0';
                letterCount++;
            }
            key = GetCharPressed();
        }

        if (IsKeyDown(KEY_BACKSPACE) && framesCounter % 3 == 0) {
            letterCount--;
            if (letterCount < 0) letterCount = 0;
            target_url[letterCount] = '\0';
        }
        if (IsKeyPressed(KEY_ENTER) && status == 0){
            status = 1;
           std::thread thread(browse,&gemlines,std::string(target_url),&status,&contentstatus);
           thread.detach();
        }

        if (IsKeyPressed(KEY_LEFT_BRACKET)){debug = !debug;}

        framesCounter++;

        //draw

        BeginDrawing();
        ClearBackground(RAYWHITE);
        //StatusBar
        DrawTextEx(font,"piper://", Vector2{5, 5}, 20,2, GRAY);
        DrawTextEx(font,target_url, Vector2{100, 5}, 20,2, BLUE);
        switch (status){
            case 0:
                DrawTextEx(font,"idle", Vector2{SCREEN_WIDTH-50, 5}, 20,2, BLACK);
                break;
            case 1:
                DrawTextEx(font,"load", Vector2{SCREEN_WIDTH-50, 5}, 20,2, BLACK);
                break;
            case 2:
                DrawTextEx(font,"parse", Vector2{SCREEN_WIDTH-70, 5}, 20,2, BLACK);
                break;
            case 3:
                DrawTextEx(font,"error", Vector2{SCREEN_WIDTH-70, 5}, 20,2, RED);
                break;
        }
        DrawLine(0,25,SCREEN_WIDTH,25,GRAY);
        //Main
        int y = 35;
        for (GemLine line:gemlines){
            switch(line.rendertype){
                case 0:
                    DrawTextEx(font,line.content.c_str(), Vector2{5, (float)y}, fontsize,2, BLACK);
                    y += fontsize+5;
                    break;
                case 1:
                    DrawTextEx(font,line.content.c_str(), Vector2{5, (float)y}, fontsize+8,2, BLACK);
                    y += fontsize+13;
                    break;
                case 2:
                    DrawTextEx(font,line.content.c_str(), Vector2{5, (float)y}, fontsize+4,2, BLACK);
                    y += fontsize+13;
                    break;
                case 3:
                    DrawTextEx(font,line.content.c_str(), Vector2{5, (float)y}, fontsize+2,2, BLACK);
                    y += fontsize+7;
                    break;
                case 4:
                    DrawTextEx(font,line.content.c_str(), Vector2{5, (float)y}, fontsize,2, GRAY);
                    y += fontsize+5;
                    break;
                case 6:
                    DrawRectangle(5,y+1,3,fontsize-2,GRAY);
                    DrawTextEx(font,line.content.c_str(), Vector2{10, (float)y}, fontsize,2, GRAY);
                    y += fontsize+5;
                    break;
                case 7:
                    float x1 = 5;
                    float y1 = y;
                    float x2 = 5+(MeasureText(line.content.c_str(),fontsize)*1.5);
                    float y2 = y+fontsize+3;
                    Vector2 mousePos = GetMousePosition();
                    bool touching = (mousePos.x > x1 && mousePos.x < x2 && mousePos.y > y1 && mousePos.y < y2);
                    if (debug){
                        if (touching) {
                            DrawRectangle(5, y, MeasureText(line.content.c_str(), fontsize) * 1.5, fontsize + 3,
                                          GREEN);
                        } else {
                            DrawRectangle(5, y, MeasureText(line.content.c_str(), fontsize) * 1.5, fontsize + 3,
                                          YELLOW);
                        }
                        DrawTextEx(font,("[Dbg] "+line.metadata).c_str(), Vector2{5, (float)y-10}, 10,2, PURPLE);
                    }
                    if (touching && IsMouseButtonDown(MouseButton::MOUSE_LEFT_BUTTON)){
                        std::string clean = line.metadata.replace(line.metadata.find("piper://"), sizeof("piper://") - 1, "");
                        std::cout << clean << std::endl;
                        status = 1;
                        std::thread thread(browse,&gemlines,clean,&status,&contentstatus);
                        thread.detach();
                    }
                    DrawTextEx(font,line.content.c_str(), Vector2{5, (float)y}, fontsize,2, SKYBLUE);
                    y += fontsize+5;
            }
        }
        if (debug) {
            std::string dbg = "[Debug] ContentType: " + std::to_string(contentstatus) + " FPS: " + std::to_string(GetFPS()) +
                              " (of target 30) URLBuffer: "+ std::to_string(letterCount) +"(of max 64) / Pipette " + VERSION;
            DrawTextEx(font,dbg.c_str(), Vector2{3, SCREEN_HEIGHT-20}, 12,2, PURPLE);
        }
        EndDrawing();
    }
    RLCloseWindow();
    return 0;
}

