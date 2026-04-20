#include<iostream>
#include<string>
#include<vector>
#include<boost/algorithm/string.hpp>

int main()
{
    std::string s = "I am a student,and you?";
    std::vector<std::string> list;

    boost::split(list,s,boost::is_any_of(" ,"));
    for(auto &l:list)
    {
        std::cout << l << std::endl;
    }
    return 0;
}