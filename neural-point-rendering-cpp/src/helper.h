#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <iostream>
#include <algorithm>

namespace Helper {
    std::vector<std::filesystem::directory_entry> get_directory_entries_sorted(const std::string& path);
}

