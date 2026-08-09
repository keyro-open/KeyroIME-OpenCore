// Copyright (C) 2025-2026 株式会社LocalPro (LocalPro Co., Ltd.). All rights reserved.
// Brand Official Website: https://keyro.jp
//
// This file is part of KeyroIME (キーロ) OpenCore.
// GNU GPLv3に基づいて配布されます。LICENSE（英語正文）を参照してください。
#pragma once

#include <windows.h>

#include <string>

namespace KeyroIME {

std::wstring ConvertRomajiBufferToHiragana(const std::string& input);
std::wstring BuildLocalFallbackCommit(const std::string& input);
bool RemoveLastRomajiUnit(std::string& input);
bool IsRomajiLongVowelKey(WPARAM wParam, LPARAM lParam, bool shiftPressed);

} // namespace KeyroIME
