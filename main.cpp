#include "OrderBook.h"

#include <iostream>
#include <string>
#include <fstream>

int main(int argc, char *argv[]) {
    std::string sym;

    if (argc < 2 or 3 < argc) {
        std::cout << "Usage: md_replay <file> [<symbol>]";
        return 0;
    }
    if (argc == 3) {
        sym = argv[2];
    }
    auto book = new OrderBook(sym);
    try {

        std::fstream file(argv[1]);
        if (!file.good()) {
            std::cout << "Failure to open a file.";
            return 0;
        }
        std::string line;
        while (std::getline(file, line)) {
        //while (std::getline(std::cin, line)) {
            book->parseLine(line);
        }
    }
    catch (const std::exception& e) {
        std::cerr << e.what();
    }
    return 0;
}
