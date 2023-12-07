#include "helper.h"

namespace Helper{
    std::vector<std::filesystem::directory_entry> get_directory_entries_sorted(const std::string& path){
        std::vector<std::filesystem::directory_entry> v;
        for (const auto& entry : std::filesystem::directory_iterator(path)) {

            v.emplace_back(entry);
        }
        /*std::cout << "------------------------------------------------------------------------------------ Pre" << std::endl;
        for(auto& file :v)
            std::cout << file << "  --  " << file.path() << std::endl;*/
        std::sort(v.begin(), v.end());
        /*std::cout << "------------------------------------------------------------------------------------ Post" << std::endl;
        for(auto& file :v)
            std::cout << file << "  --  " << file.path() << std::endl;*/

        return v;

    }
}