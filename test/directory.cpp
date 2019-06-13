#include <iostream>

#include "../include/essentials.hpp"

using namespace essentials;

int main() {
    directory dir("..");
    std::cout << "directory name: '" << dir.name() << "'" << std::endl;
    std::cout << "found " << dir.items() << " items" << std::endl;

    for (auto const& fn : dir) {
        if (fn.extension == "cpp") {
            std::cout << "---" << std::endl;
            std::cout << fn.name << std::endl;
            std::cout << fn.fullpath << std::endl;
            std::cout << fn.extension << std::endl;
        }
    }

    return 0;
}
