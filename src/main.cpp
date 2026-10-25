#include "app/application.hpp"
#include <exception>
#include <iostream>
int main(int argc, char** argv) {
    try {
        return blobber::run(argc, argv);
    } catch (const std::exception& error) {
        std::cerr << "Blobber: " << error.what() << '\n';
        return 1;
    }
}
