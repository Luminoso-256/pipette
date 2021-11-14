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


#define SCREEN_WIDTH (800)
#define SCREEN_HEIGHT (450)
#define WINDOW_TITLE "Pipette - the tiny piper browser"
#define MAX_INPUT_CHARS     64

namespace kn = kissnet;
#include <chrono>
using namespace std::chrono_literals;


struct GemLine{
    std::string content;
    int rendertype;
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


void browse(std::string *result,char *url,int *status){

    /* set up key strings */

    std::vector<std::string> parts = split(std::string(url),"/");
    std::string host = parts[0] + ":60";
    //clean out the first part
    parts.erase(parts.begin());
    //pull together the URI
    std::string uri = "/"+join(parts, '/');

    /* socket time! */

    //== Send

    kn::tcp_socket tcpsocket((kn::endpoint(host)));
    tcpsocket.connect();
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
    while(tcpsocket.bytes_available() == 0){}
    std::cout << "bytes available to read : " << tcpsocket.bytes_available() << '\n';
    //== Recieve
    kn::buffer<4096> static_buffer;
    const auto [data_size, status_code] = tcpsocket.recv(static_buffer);
    if(data_size < static_buffer.size())
        static_buffer[data_size] = std::byte{ '\0' };
    *status = 2;

    int contenttype = (int) static_buffer[0];
    *result = "";
    for (int i = 9; i<data_size; i++){
        *result += (char)static_buffer[i];
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
    int scrollSpeed = 4;


    int rendermode = 1;
    std::vector<GemLine> gemlines(1,GemLine{"",0,""});
    std::string plaintext = "waiting for browse activity";

    bool dbgrend = false;
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
           std::thread thread(browse,&plaintext,target_url,&status);
           thread.detach();
        }

        framesCounter++;

        //draw

        BeginDrawing();
        ClearBackground(RAYWHITE);
        //StatusBar
        DrawText("piper://", 5, 5, 20, GRAY);
        DrawText(target_url, 95, 5, 20, BLUE);
        switch (status){
            case 0:
                DrawText("idle", SCREEN_WIDTH-50, 5, 20, GRAY);
                break;
            case 1:
                DrawText("load", SCREEN_WIDTH-50, 5, 20, GRAY);
                break;
            case 2:
                DrawText("parse", SCREEN_WIDTH-70, 5, 20, GRAY);
                break;
            case 3:
                DrawText("error", SCREEN_WIDTH-70, 5, 20, RED);
                break;
        }
        DrawLine(0,25,SCREEN_WIDTH,25,GRAY);
        //Main
        switch (rendermode){
            case 1:
                DrawText(plaintext.c_str(), 5, 35, fontsize, BLACK);
                break;
        }
        EndDrawing();
    }
    RLCloseWindow();
    return 0;
}

