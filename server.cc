#include "searcher.hpp"
#include <iostream>
#include <string>
#include <cstdio>

const std::string input = "data/raw_html/raw.txt";

int main()
{
    ns_searcher::Searcher *search = new ns_searcher::Searcher();
    search->InitSearcher(input);

    std::string query;
    std::string json_string;
    char buffer[1024];
    while (true)
    {
        std::cout << "Please enter your search query:";
        fgets(buffer, sizeof(buffer) - 1, stdin);
        // std::cin >> query;
        buffer[strlen(buffer) - 1] = 0;
        query = buffer;
        search->Search(query, &json_string);

        if (json_string.size())
            std::cout << "搜索结果为：" << json_string << std::endl;
    }

    return 0;
}