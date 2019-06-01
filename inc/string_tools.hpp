#pragma once

#include <string>

namespace cynth::string_tools {

    std::string to_string (const std::string& from) {
        return from;
    }
    std::string to_string (const std::wstring& from) {
        return {from.begin(), from.end()};
    }
    /*std::string to_tring(wchar_t* from) {;
        std::wstring wstr{from};
        std::string result{wstr.begin(), wstr.end()};
        return result;
    }*/
    template <typename T, typename... Dummy>
    std::string to_string (const T& from, Dummy...) {
        return std::to_string(from);
    }

}