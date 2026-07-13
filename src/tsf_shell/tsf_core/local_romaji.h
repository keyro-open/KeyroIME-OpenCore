// Copyright (C) 2025-2026 株式会社LocalPro (LocalPro Co., Ltd.). All rights reserved.
// Brand Official Website: https://keyro.jp
//
// This file is part of KeyroIME (キーロ) OpenCore.
// It is source-available under the KeyroIME OpenCore Non-Commercial Source
// License 1.0. See LICENSE. Commercial use requires a separate written license
// from 株式会社LocalPro.
#pragma once

#include <windows.h>

#include <string>

namespace KeyroIME {

std::wstring ConvertRomajiBufferToHiragana(const std::string& input);
std::wstring BuildLocalFallbackCommit(const std::string& input);
bool RemoveLastRomajiUnit(std::string& input);
bool IsRomajiLongVowelKey(WPARAM wParam, LPARAM lParam, bool shiftPressed);

} // namespace KeyroIME
