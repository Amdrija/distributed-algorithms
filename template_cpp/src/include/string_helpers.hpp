#pragma once

#include <iostream>
#include <string>
#include <vector>

class StringHelpers {
public:
    static std::vector<std::string> split(const std::string &str, const std::string &delimeter) {
        std::vector<std::string> result;
        unsigned long int start = 0;
        unsigned long int end = str.find(delimeter);

        while (end != std::string::npos) {
            result.push_back(str.substr(start, end - start));
            start = end + delimeter.length();
            end = str.find(delimeter, start);
        }
        result.push_back(str.substr(start, end - start));

        return result;
    }
};
