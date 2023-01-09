#pragma once

#include <vector>
#include <string>     // std::string, std::stoll
#include <algorithm>
#include <functional>
#include <stdexcept>
#include <cctype>
#include <errno.h>

inline unsigned char char_toupper(unsigned char c)
{
    return toupper(c);
}

inline std::string str_toupper(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), char_toupper);
    return s;
}

// trim from start
static inline std::string& ltrim(std::string& s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
        std::not1(std::ptr_fun<int, int>(std::isspace))));
    return s;
}

// trim from end
static inline std::string& rtrim(std::string& s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(),
        std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
    return s;
}

// trim from both ends
static inline std::string& trim(std::string& s)
{
    return ltrim(rtrim(s));
}


// convert string to long long
inline __int64 stoll(const std::string& str)
{
    const char* ptr = str.c_str();
    char* error_ptr;
    __int64 result = _strtoi64(ptr, &error_ptr, 10);

    if (ptr == error_ptr)
    {
        throw std::out_of_range("invalid stoll argument");
    }

    if (errno == EINVAL)
    {
        throw std::out_of_range("stoll argument out of range");
    }

    return result;
}

inline __int64 parse_int64(const std::string& str)
{
    __int64 rez;
    try
    {
        rez = ::stoll(str);
    }
    catch (const std::out_of_range&)
    {
        rez = -1;
    }
    return rez;
}

inline std::vector<std::string> split(const std::string& source, const std::string& delimiters = " ")
{
    std::size_t prev = 0;
    std::size_t currentPos;
    std::vector<std::string> results;

    while ((currentPos = source.find_first_of(delimiters, prev)) != std::string::npos)
    {
        if (currentPos > prev)
        {
            results.push_back(source.substr(prev, currentPos - prev));
        }
        prev = currentPos + 1;
    }
    if (prev < source.length())
    {
        results.push_back(source.substr(prev));
    }
    return results;
}
