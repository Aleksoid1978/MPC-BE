#include "stdafx.h"
#include "RegexUtil.h"

size_t RegexUtil::stringMatch(const std::string& pattern, const std::string& text, regexResults& results) {
    std::regex regex_pattern(pattern, RegexFlags);
    return stringMatch(regex_pattern, text, results);
}

size_t RegexUtil::stringMatch(const std::regex& pattern, const std::string& text, regexResults& results) {
    results.clear();

    std::string data(text);
    std::smatch match_pieces;
    while (std::regex_search(data, match_pieces, pattern)) {
        regexResult result;
        for (auto match = match_pieces.begin(); match != match_pieces.end(); ++match) {
            if (match != match_pieces.begin()) {
                result.push_back((*match).str());
            }
        }
        results.push_back(result);
        data = match_pieces.suffix().str();
    }
    return results.size();
}

size_t RegexUtil::stringMatch(const std::string& pattern, const std::string& text, regexResult& result) {
    std::regex regex_pattern(pattern, RegexFlags);
    return stringMatch(regex_pattern, text, result);
}

size_t RegexUtil::stringMatch(const std::regex& pattern, const std::string& text, regexResult& result) {
    result.clear();

    std::smatch match_pieces;
    std::regex_search(text, match_pieces, pattern);
    for (const auto& match : match_pieces) {
        if (match != *match_pieces.begin()) {
            result.push_back(match.str());
        }
    }
    return result.size();
}

size_t RegexUtil::wstringMatch(const std::wstring& pattern, const std::wstring& text, wregexResults& results) {
    std::wregex regex_pattern(pattern, WRegexFlags);
    return wstringMatch(regex_pattern, text, results);
}

size_t RegexUtil::wstringMatch(const std::wregex& pattern, const std::wstring& text, wregexResults& results) {
    results.clear();

    std::wstring data(text);
    std::wsmatch match_pieces;
    while (std::regex_search(data, match_pieces, pattern)) {
        wregexResult result;
        for (auto match = match_pieces.begin(); match != match_pieces.end(); ++match) {
            if (match != match_pieces.begin()) {
                result.push_back((*match).str());
            }
        }
        results.push_back(result);
        data = match_pieces.suffix().str();
    }
    return results.size();
}

size_t RegexUtil::wstringMatch(const std::wstring& pattern, const std::wstring& text, wregexResult& result) {
    std::wregex regex_pattern(pattern, RegexFlags);
    return wstringMatch(regex_pattern, text, result);
}

size_t RegexUtil::wstringMatch(const std::wregex& pattern, const std::wstring& text, wregexResult& result) {
    result.clear();

    std::wsmatch match_pieces;
    std::regex_search(text, match_pieces, pattern);
    for (const auto& match : match_pieces) {
        if (match != *match_pieces.begin()) {
            result.push_back(match.str());
        }
    }
    return result.size();
}
