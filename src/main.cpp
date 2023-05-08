#include "parser.hpp"
#include <iostream>

sjtu::Parser parser;

int main()
{
    std::ios::sync_with_stdio(0);
    std::cin.tie(0);
    std::cout.tie(0);
    while (true)
    {
        std::string line;
        getline(std::cin, line);
        parser.parseline(line);
        parser.execute();
    }
}
