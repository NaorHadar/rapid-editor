#pragma once

#include <string>

std::string ToLower(std::string Source)
{
    for(char& Char : Source)
    {
        Char = (char)tolower(Char);
    }
    
    return Source;
}
