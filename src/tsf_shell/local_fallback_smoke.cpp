// Copyright (C) 2025-2026 Localpro株式会社 (Localpro Co., Ltd.). All rights reserved.
// Brand Official Website: https://keyro.jp
//
// This file is part of KeyroIME (キーロ) v1.0 OpenCore.
// KeyroIME is free software: you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software Foundation.
//
// For commercial use licensing, custom deployment, or proprietary integrations,
// please contact Localpro株式会社 via https://localpro.jp. Unauthorized closed-source
// commercial exploitation is strictly prohibited.
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
