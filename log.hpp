#pragma once

#include <iostream>
#include <string>
#include <ctime>

#define NORMAL 1
#define WARNING 2
#define DEBUG 3
#define FATAL 4

#define LOG(LEVEL, MESSAGE) log(#LEVEL, MESSAGE, __FILE__, __LINE__)

void log(const std::string& level, const std::string& msg,const std::string& file, int line)
{
    //[1][2026-4-25][message][file.cc][34]
    std::cout << "[" << level << "]" << "[" << time(nullptr) << "]"
              << "[" << msg << "]" << "[" << file << "]" << "[" << line << "]" << std::endl;
}