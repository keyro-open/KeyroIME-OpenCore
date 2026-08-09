// Copyright (C) 2025-2026 株式会社LocalPro (LocalPro Co., Ltd.). All rights reserved.
// Brand Official Website: https://keyro.jp
//
// This file is part of KeyroIME (キーロ) OpenCore.
// GNU GPLv3に基づいて配布されます。LICENSE（英語正文）を参照してください。
#include "tsf_core/local_romaji.h"

#include <iostream>

namespace {

bool Expect(const std::string& input, const std::wstring& expected)
{
    std::wstring actual = KeyroIME::BuildLocalFallbackCommit(input);
    if (actual == expected) {
        return true;
    }
    std::wcerr << L"fallback mismatch for input: "
               << std::wstring(input.begin(), input.end()) << L"\n";
    return false;
}

bool ExpectBackspace(const std::string& input, const std::string& expected)
{
    std::string actual = input;
    KeyroIME::RemoveLastRomajiUnit(actual);
    if (actual == expected) {
        return true;
    }
    std::cerr << "backspace mismatch for input: " << input
              << " expected: " << expected
              << " actual: " << actual << "\n";
    return false;
}

} // namespace

int main()
{
    bool passed = true;
    passed = Expect("jya", L"じゃ") && passed;
    passed = Expect("lya", L"りゃ") && passed;
    passed = Expect("github", L"github") && passed;
    passed = Expect("qcheck", L"qcheck") && passed;
    passed = Expect("insuto-ru", L"いんすとーる") && passed;
    passed = ExpectBackspace("kyo", "") && passed;
    passed = ExpectBackspace("kyou", "kyo") && passed;
    passed = ExpectBackspace("koukan", "kouka") && passed;
    passed = ExpectBackspace("qcheck", "qchec") && passed;
    LPARAM minusScanCode = static_cast<LPARAM>(0x0C) << 16;
    passed = KeyroIME::IsRomajiLongVowelKey(VK_OEM_MINUS, minusScanCode, false) && passed;
    passed = !KeyroIME::IsRomajiLongVowelKey(VK_OEM_MINUS, minusScanCode, true) && passed;

    if (!passed) {
        return 1;
    }

    std::cout << "Local fallback smoke test passed.\n";
    return 0;
}
