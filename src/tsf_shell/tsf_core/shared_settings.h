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
#pragma once

#include <windows.h>

namespace KeyroIME {

enum class SharedInputMode : LONG {
    Hiragana = 0,
    Katakana = 1,
    English = 2,
};

enum class SharedCharWidth : LONG {
    HalfWidth = 0,
    FullWidth = 1,
};

enum class SharedKeyboardLayout : LONG {
    Jis = 0,
    Ansi = 1,
};

enum class SharedPunctuationStyle : LONG {
    Japanese = 0,
    Western = 1,
};

enum class SharedEnglishCase : LONG {
    Lower = 0,
    Upper = 1,
};

struct InputSettingsSnapshot {
    SharedInputMode inputMode = SharedInputMode::Hiragana;
    SharedCharWidth charWidth = SharedCharWidth::HalfWidth;
    SharedKeyboardLayout keyboardLayout = SharedKeyboardLayout::Jis;
    SharedPunctuationStyle punctuationStyle = SharedPunctuationStyle::Japanese;
    SharedEnglishCase englishCase = SharedEnglishCase::Lower;
};

InputSettingsSnapshot ToggleCapsLockSetting(const InputSettingsSnapshot& snapshot);
InputSettingsSnapshot ToggleCharacterWidthSetting(const InputSettingsSnapshot& snapshot);
InputSettingsSnapshot ToggleKeyboardLayoutSetting(const InputSettingsSnapshot& snapshot);
InputSettingsSnapshot CycleKanaWidthSetting(const InputSettingsSnapshot& snapshot);
bool IsKeyboardLayoutToggleShortcut(WPARAM wParam, LPARAM lParam, bool altPressed);
bool IsImeModeToggleKey(WPARAM wParam);
bool IsKanaModeKey(WPARAM wParam);
bool IsExplicitHiraganaModeKey(WPARAM wParam);
bool IsExplicitKatakanaModeKey(WPARAM wParam);
bool IsConvertKey(WPARAM wParam);
bool IsNonConvertKey(WPARAM wParam);

class SharedInputSettings {
public:
    SharedInputSettings();
    ~SharedInputSettings();

    SharedInputSettings(const SharedInputSettings&) = delete;
    SharedInputSettings& operator=(const SharedInputSettings&) = delete;

    bool Initialize(bool createIfMissing);
    void Close();
    bool IsInitialized() const;
    InputSettingsSnapshot Read() const;
    bool Write(const InputSettingsSnapshot& snapshot);

private:
    struct SharedState;

    HANDLE m_mapping;
    SharedState* m_state;
    bool m_writable;
};

} // namespace KeyroIME
