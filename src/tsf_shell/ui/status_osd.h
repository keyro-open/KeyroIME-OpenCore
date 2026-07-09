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
