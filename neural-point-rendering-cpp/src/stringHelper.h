#pragma once

#include <vector>
#include <string>
#include <sstream>

namespace StringHelper {
    std::vector<std::string> split(const std::string& s, char delim);
    inline int to_int(const std::string& str)
    {
        return std::atoi(str.c_str());
    }
}

