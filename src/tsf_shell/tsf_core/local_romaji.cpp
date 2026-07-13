// Copyright (C) 2025-2026 株式会社LocalPro (LocalPro Co., Ltd.). All rights reserved.
// Brand Official Website: https://keyro.jp
//
// This file is part of KeyroIME (キーロ) OpenCore.
// It is source-available under the KeyroIME OpenCore Non-Commercial Source
// License 1.0. See LICENSE. Commercial use requires a separate written license
// from 株式会社LocalPro.
#include "local_romaji.h"

#include <cstring>
#include <string_view>

namespace KeyroIME {

namespace {

struct RomajiEntry {
    const char* romaji;
    const wchar_t* kana;
};

constexpr RomajiEntry kRomajiTable[] = {
    {"xtsu", L"っ"}, {"ltsu", L"っ"},
    {"kya", L"きゃ"}, {"kyu", L"きゅ"}, {"kyo", L"きょ"},
    {"sha", L"しゃ"}, {"shu", L"しゅ"}, {"sho", L"しょ"},
    {"sya", L"しゃ"}, {"syu", L"しゅ"}, {"syo", L"しょ"},
    {"cha", L"ちゃ"}, {"chu", L"ちゅ"}, {"cho", L"ちょ"},
    {"tya", L"ちゃ"}, {"tyu", L"ちゅ"}, {"tyo", L"ちょ"},
    {"nya", L"にゃ"}, {"nyu", L"にゅ"}, {"nyo", L"にょ"},
    {"hya", L"ひゃ"}, {"hyu", L"ひゅ"}, {"hyo", L"ひょ"},
    {"mya", L"みゃ"}, {"myu", L"みゅ"}, {"myo", L"みょ"},
    {"rya", L"りゃ"}, {"ryu", L"りゅ"}, {"ryo", L"りょ"},
    {"lya", L"りゃ"}, {"lyu", L"りゅ"}, {"lyo", L"りょ"},
    {"gya", L"ぎゃ"}, {"gyu", L"ぎゅ"}, {"gyo", L"ぎょ"},
    {"bya", L"びゃ"}, {"byu", L"びゅ"}, {"byo", L"びょ"},
    {"pya", L"ぴゃ"}, {"pyu", L"ぴゅ"}, {"pyo", L"ぴょ"},
    {"zya", L"じゃ"}, {"zyu", L"じゅ"}, {"zyo", L"じょ"},
    {"jya", L"じゃ"}, {"jyu", L"じゅ"}, {"jyo", L"じょ"},
    {"xya", L"ゃ"}, {"xyu", L"ゅ"}, {"xyo", L"ょ"},
    {"xka", L"ゕ"}, {"xke", L"ゖ"}, {"xwa", L"ゎ"},
    {"lka", L"ゕ"}, {"lke", L"ゖ"}, {"lwa", L"ゎ"},
    {"shi", L"し"}, {"chi", L"ち"}, {"tsu", L"つ"},
    {"ja", L"じゃ"}, {"ju", L"じゅ"}, {"jo", L"じょ"},
    {"fa", L"ふぁ"}, {"fi", L"ふぃ"}, {"fe", L"ふぇ"}, {"fo", L"ふぉ"},
    {"va", L"ゔぁ"}, {"vi", L"ゔぃ"}, {"vu", L"ゔ"}, {"ve", L"ゔぇ"}, {"vo", L"ゔぉ"},
    {"xa", L"ぁ"}, {"xi", L"ぃ"}, {"xu", L"ぅ"}, {"xe", L"ぇ"}, {"xo", L"ぉ"},
    {"la", L"ぁ"}, {"li", L"ぃ"}, {"lu", L"ぅ"}, {"le", L"ぇ"}, {"lo", L"ぉ"},
    {"xtu", L"っ"}, {"ltu", L"っ"},
    {"ka", L"か"}, {"ki", L"き"}, {"ku", L"く"}, {"ke", L"け"}, {"ko", L"こ"},
    {"sa", L"さ"}, {"si", L"し"}, {"su", L"す"}, {"se", L"せ"}, {"so", L"そ"},
    {"ta", L"た"}, {"ti", L"ち"}, {"tu", L"つ"}, {"te", L"て"}, {"to", L"と"},
    {"na", L"な"}, {"ni", L"に"}, {"nu", L"ぬ"}, {"ne", L"ね"}, {"no", L"の"},
    {"ha", L"は"}, {"hi", L"ひ"}, {"hu", L"ふ"}, {"fu", L"ふ"}, {"he", L"へ"}, {"ho", L"ほ"},
    {"ma", L"ま"}, {"mi", L"み"}, {"mu", L"む"}, {"me", L"め"}, {"mo", L"も"},
    {"ya", L"や"}, {"yu", L"ゆ"}, {"yo", L"よ"},
    {"ra", L"ら"}, {"ri", L"り"}, {"ru", L"る"}, {"re", L"れ"}, {"ro", L"ろ"},
    {"wa", L"わ"}, {"wo", L"を"},
    {"ga", L"が"}, {"gi", L"ぎ"}, {"gu", L"ぐ"}, {"ge", L"げ"}, {"go", L"ご"},
    {"za", L"ざ"}, {"zi", L"じ"}, {"ji", L"じ"}, {"zu", L"ず"}, {"ze", L"ぜ"}, {"zo", L"ぞ"},
    {"da", L"だ"}, {"di", L"ぢ"}, {"du", L"づ"}, {"de", L"で"}, {"do", L"ど"},
    {"ba", L"ば"}, {"bi", L"び"}, {"bu", L"ぶ"}, {"be", L"べ"}, {"bo", L"ぼ"},
    {"pa", L"ぱ"}, {"pi", L"ぴ"}, {"pu", L"ぷ"}, {"pe", L"ぺ"}, {"po", L"ぽ"},
    {"a", L"あ"}, {"i", L"い"}, {"u", L"う"}, {"e", L"え"}, {"o", L"お"},
    {"-", L"ー"},
};

std::string_view EffectiveInput(const std::string& input)
{
    std::string_view view(input);
    if (!view.empty() && (view.front() == 'q' || view.front() == 'v')) {
        view.remove_prefix(1);
    }
    return view;
}

bool IsVowel(char value)
{
    return value == 'a' || value == 'i' || value == 'u' || value == 'e' || value == 'o';
}

bool ContainsAsciiLetter(const std::wstring& value)
{
    for (wchar_t ch : value) {
        if ((ch >= L'a' && ch <= L'z') || (ch >= L'A' && ch <= L'Z')) {
            return true;
        }
    }
    return false;
}

std::wstring AsciiToWide(std::string_view value)
{
    return std::wstring(value.begin(), value.end());
}

} // namespace

std::wstring ConvertRomajiBufferToHiragana(const std::string& input)
{
    if (!input.empty() && (input.front() == 'q' || input.front() == 'v')) {
        return AsciiToWide(input);
    }

    std::string_view remaining = EffectiveInput(input);
    std::wstring result;

    while (!remaining.empty()) {
        char first = remaining.front();

        if (remaining.size() >= 2 && first == remaining[1] &&
            first >= 'a' && first <= 'z' && !IsVowel(first) && first != 'n') {
            result.push_back(L'っ');
            remaining.remove_prefix(1);
            continue;
        }

        if (first == 'n') {
            if (remaining.size() == 1) {
                result.push_back(L'ん');
                remaining.remove_prefix(1);
                continue;
            }

            char next = remaining[1];
            if (next == 'n') {
                result.push_back(L'ん');
                bool secondNStartsSyllable = remaining.size() > 2 &&
                    (IsVowel(remaining[2]) || remaining[2] == 'y');
                remaining.remove_prefix(secondNStartsSyllable ? 1 : 2);
                continue;
            }
            if (!IsVowel(next) && next != 'y') {
                result.push_back(L'ん');
                remaining.remove_prefix(1);
                continue;
            }
        }

        bool matched = false;
        for (const RomajiEntry& entry : kRomajiTable) {
            size_t length = std::strlen(entry.romaji);
            if (remaining.size() >= length && remaining.compare(0, length, entry.romaji) == 0) {
                result.append(entry.kana);
                remaining.remove_prefix(length);
                matched = true;
                break;
            }
        }

        if (!matched) {
            result.push_back(static_cast<unsigned char>(first));
            remaining.remove_prefix(1);
        }
    }

    return result;
}

std::wstring BuildLocalFallbackCommit(const std::string& input)
{
    if (!input.empty() && (input.front() == 'q' || input.front() == 'v')) {
        return AsciiToWide(input);
    }

    std::string_view effective = EffectiveInput(input);
    std::wstring kana = ConvertRomajiBufferToHiragana(input);
    if (ContainsAsciiLetter(kana)) {
        return AsciiToWide(effective);
    }
    return kana;
}

bool RemoveLastRomajiUnit(std::string& input)
{
    if (input.empty()) {
        return false;
    }

    if (input.size() == 1 || input.front() == 'q' || input.front() == 'v') {
        input.pop_back();
        return true;
    }

    std::wstring currentText = ConvertRomajiBufferToHiragana(input);
    for (size_t cut = input.size() - 1; cut > 0; --cut) {
        std::string candidate = input.substr(0, cut);
        std::wstring candidateText = ConvertRomajiBufferToHiragana(candidate);
        if (candidateText.size() < currentText.size() &&
            currentText.compare(0, candidateText.size(), candidateText) == 0) {
            input.resize(cut);
            return true;
        }
    }

    input.clear();
    return true;
}

bool IsRomajiLongVowelKey(WPARAM wParam, LPARAM lParam, bool shiftPressed)
{
    if (shiftPressed) {
        return false;
    }

    UINT scanCode = static_cast<UINT>((static_cast<ULONG_PTR>(lParam) >> 16) & 0xFF);
    return scanCode == 0x0C || (scanCode == 0 && wParam == VK_OEM_MINUS);
}

} // namespace KeyroIME
