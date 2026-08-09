// Copyright (C) 2025-2026 株式会社LocalPro (LocalPro Co., Ltd.). All rights reserved.
// Brand Official Website: https://keyro.jp
//
// This file is part of KeyroIME (キーロ) OpenCore.
// GNU GPLv3に基づいて配布されます。LICENSE（英語正文）を参照してください。
#pragma once

#include <windows.h>

#include <string>

namespace KeyroIME {

class StatusOsd {
public:
    StatusOsd();
    ~StatusOsd();

    StatusOsd(const StatusOsd&) = delete;
    StatusOsd& operator=(const StatusOsd&) = delete;

    bool Initialize(HINSTANCE instance);
    void ShowStatus(const std::wstring& text);
    void Hide();
    bool IsVisible() const;
    const std::wstring& CurrentText() const { return m_text; }

private:
    static constexpr UINT_PTR kHoldTimerId = 1;
    static constexpr UINT_PTR kFadeTimerId = 2;
    static constexpr BYTE kVisibleAlpha = 235;

    HINSTANCE m_instance;
    HWND m_window;
    std::wstring m_text;
    BYTE m_alpha;

    void PositionWindow();
    void Paint();
    void OnTimer(UINT_PTR timerId);
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
};

} // namespace KeyroIME
