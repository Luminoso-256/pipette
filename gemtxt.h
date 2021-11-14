/*
 * pipette
 * gemtxt.h
 * created 11-14-21
 -------------------
 (C) Luminoso 2021/All Rights Reserved
 * */

#ifndef PIPETTE_GEMTXT_H
#define PIPETTE_GEMTXT_H

#include <vector>
#include "util.h"
namespace gemtext{
    struct GemLine{
        std::string content;
        int rendertype; //0 = plain, 1 = Heading, 2 = Semiheading, 3 = SemiSemi heading, 4 = Monospace, 5 = List, 6 = Quote, 7 = Link
        std::string metadata;
    };


    std::vector<GemLine> parse(std::string input,bool ignoreformatdirectives){
        std::vector<std::string> lines = util::split(input,std::string("\n"));
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
                std::vector<std::string> parts = util::split(line," ");
                if (parts.size() <= 1){
                    gemlines.push_back(GemLine{
                            parts[0], 7, parts[0]
                    });
                } else {
                    std::string target = parts[0];
                    parts.erase(parts.begin());
                    std::string collected = util::join(parts, ' ');
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
}

#endif //PIPETTE_GEMTXT_H
