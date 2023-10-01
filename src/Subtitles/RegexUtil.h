#pragma once

#include <regex>
#include <vector>

namespace RegexUtil { //taken from SubtitlesProvidersUtils and extended for wstring
    static constexpr std::regex::flag_type RegexFlags(std::regex_constants::ECMAScript | std::regex_constants::icase | std::regex_constants::optimize);
    static constexpr std::wregex::flag_type WRegexFlags(std::regex_constants::ECMAScript | std::regex_constants::icase | std::regex_constants::optimize);

    using wregexResult = std::vector<std::wstring>;
    using wregexResults = std::vector<wregexResult>;

    size_t wstringMatch(const std::wstring& pattern, const std::wstring& text, wregexResults& results);
    size_t wstringMatch(const std::wstring& pattern, const std::wstring& text, wregexResult& result);
    size_t wstringMatch(const std::wregex& pattern, const std::wstring& text, wregexResults& results);
    size_t wstringMatch(const std::wregex& pattern, const std::wstring& text, wregexResult& result);

    using regexResult = std::vector<std::string>;
    using regexResults = std::vector<regexResult>;

    size_t stringMatch(const std::string& pattern, const std::string& text, regexResults& results);
    size_t stringMatch(const std::string& pattern, const std::string& text, regexResult& result);
    size_t stringMatch(const std::regex& pattern, const std::string& text, regexResults& results);
    size_t stringMatch(const std::regex& pattern, const std::string& text, regexResult& result);

};

