#pragma once
// 工具类

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <boost/algorithm/string.hpp>

namespace ns_util
{
    class FileUtil
    {
    public:
        static bool ReadFile(const std::string &file_path, std::string *out)
        {
            // 创建一个输入文件流对象in
            std::ifstream in(file_path, std::ios::in); // std::ios::in 是一个打开模式标志，表示以 读取模式 打开文件。
            if (!in.is_open())
            {
                std::cerr << "open file" << file_path << " error" << std::endl;
                return false;
            }

            std::string line;
            while (std::getline(in, line))
            {
                *out += line;
            }

            in.close();
            return true;
        }
    };
    class StringUtil
    {
    public:
        static void CutString(const std::string &target, std::vector<std::string> *out, std::string sep)
        {
            boost::split(*out, target, boost::is_any_of("\3"), boost::token_compress_on);
        }
    };
} // namespace ns_util
