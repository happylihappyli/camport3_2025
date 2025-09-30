
#include <iostream>
#include "funny_Mat.hpp"

int main() {
    std::cout << "Testing common_lib..." << std::endl;
    funny_Mat mat(640, 480, CV_8UC3, NULL);
    std::cout << "Created funny_Mat object" << std::endl;
    return 0;
}
