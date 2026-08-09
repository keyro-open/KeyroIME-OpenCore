// Copyright (C) 2025-2026 株式会社LocalPro (LocalPro Co., Ltd.). All rights reserved.
// Brand Official Website: https://keyro.jp
//
// This file is part of KeyroIME (キーロ) OpenCore.
// GNU GPLv3に基づいて配布されます。LICENSE（英語正文）を参照してください。
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
