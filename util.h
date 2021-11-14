/*
 * pipette
 * util.h
 * created 11-14-21
 -------------------
 (C) Luminoso 2021/All Rights Reserved
 * */
#ifndef PIPETTE_UTIL_H
#define PIPETTE_UTIL_H


#include <vector>
namespace util{
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
}

#endif //PIPETTE_UTIL_H
