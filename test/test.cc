#include <iostream>
#include <string>
#include <vector>
#include <boost/algorithm/string.hpp>
#include <jsoncpp/json/json.h>

int main()
{
    // std::string s = "I am a student,and you?";
    // std::vector<std::string> list;

    // boost::split(list,s,boost::is_any_of(" ,"));
    // for(auto &l:list)
    // {
    //     std::cout << l << std::endl;
    // }

    Json::Value root;

    Json::Value item1;
    item1["key1"] = "value1";
    item1["key2"] = "value2";
    Json::Value item2;
    item2["key1"] = "value1";
    item2["key2"] = "value2";

    root.append(item1);
    root.append(item2);

    Json::StreamWriterBuilder writer;
    std::string s = Json::writeString(writer, root);

    std::cout << s << std::endl;
    return 0;
}