#include <iostream>
#include "version.hpp"

int main(int argc, char* argv[]) {
    std::cout << "Lib STA: " << build_version << " Compiled:" << build_date << std::endl;
}

