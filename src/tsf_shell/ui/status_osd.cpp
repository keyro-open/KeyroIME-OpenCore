// Copyright (C) 2025-2026 株式会社LocalPro (LocalPro Co., Ltd.). All rights reserved.
// Brand Official Website: https://keyro.jp
//
// This file is part of KeyroIME (キーロ) OpenCore.
// It is source-available under the KeyroIME OpenCore Non-Commercial Source
// License 1.0. See LICENSE. Commercial use requires a separate written license
// from 株式会社LocalPro.
#include "status_osd.h"

namespace KeyroIME {

namespace {

constexpr wchar_t kStatusOsdClassName[] = L"KeyroIME_StatusOsd";
constexpr int kWindowWidth = 520;
constexpr int kWindowHeight = 68;

} // namespace

StatusOsd::StatusOsd()
    : m_instance(nullptr)
    , m_window(nullptr)
    , m_alpha(0)
{
}

StatusOsd::~StatusOsd()
{
    if (m_window) {
        DestroyWindow(m_window);
        m_window = nullptr;
    }
}

bool StatusOsd::Initialize(HINSTANCE instance)
{
    if (m_window) {
        return true;
    }
    if (!instance) {
        return false;
    }

    m_instance = instance;
    WNDCLASSW windowClass = {};
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
    windowClass.lpszClassName = kStatusOsdClassName;
    if (RegisterClassW(&windowClass) == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return false;
    }

    m_window = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW |
            WS_EX_NOACTIVATE | WS_EX_TOPMOST,
        kStatusOsdClassName,
        L"",
        WS_POPUP,
        0,
        0,
        kWindowWidth,
        kWindowHeight,
        nullptr,
        nullptr,
        instance,
        this);
    return m_window != nullptr;
}

void StatusOsd::ShowStatus(const std::wstring& text)
{
    if (!m_window || text.empty()) {
        return;
    }

    KillTimer(m_window, kHoldTimerId);
    KillTimer(m_window, kFadeTimerId);
    m_text = text;
    m_alpha = kVisibleAlpha;
    SetLayeredWindowAttributes(m_window, 0, m_alpha, LWA_ALPHA);
    PositionWindow();
    InvalidateRect(m_window, nullptr, TRUE);
    UpdateWindow(m_window);
    SetTimer(m_window, kHoldTimerId, 1000, nullptr);
}

void StatusOsd::Hide()
{
    if (!m_window) {
        return;
    }
    KillTimer(m_window, kHoldTimerId);
    KillTimer(m_window, kFadeTimerId);
    ShowWindow(m_window, SW_HIDE);
    m_alpha = 0;
}

bool StatusOsd::IsVisible() const
{
    return m_window && IsWindowVisible(m_window) != FALSE;
}

void StatusOsd::PositionWindow()
{
    HWND foreground = GetForegroundWindow();
    HMONITOR monitor = MonitorFromWindow(foreground, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO monitorInfo = {};
    monitorInfo.cbSize = sizeof(monitorInfo);
    if (!GetMonitorInfoW(monitor, &monitorInfo)) {
        monitorInfo.rcWork = {
            0,
            0,
            GetSystemMetrics(SM_CXSCREEN),
            GetSystemMetrics(SM_CYSCREEN)};
    }

    int workWidth = monitorInfo.rcWork.right - monitorInfo.rcWork.left;
    int workHeight = monitorInfo.rcWork.bottom - monitorInfo.rcWork.top;
    int x = monitorInfo.rcWork.left + (workWidth - kWindowWidth) / 2;
    int y = monitorInfo.rcWork.top + (workHeight * 2 / 3) - (kWindowHeight / 2);
    SetWindowPos(
        m_window,
        HWND_TOPMOST,
        x,
        y,
        kWindowWidth,
        kWindowHeight,
        SWP_NOACTIVATE | SWP_SHOWWINDOW);
}

void StatusOsd::Paint()
{
    PAINTSTRUCT paint = {};
    HDC dc = BeginPaint(m_window, &paint);
    if (!dc) {
        return;
    }

    RECT rect = {};
    GetClientRect(m_window, &rect);
    HBRUSH background = CreateSolidBrush(RGB(32, 34, 38));
    HPEN border = CreatePen(PS_SOLID, 1, RGB(76, 80, 88));
    HGDIOBJ oldBrush = SelectObject(dc, background);
    HGDIOBJ oldPen = SelectObject(dc, border);
    RoundRect(dc, rect.left, rect.top, rect.right, rect.bottom, 16, 16);
    SelectObject(dc, oldPen);
    SelectObject(dc, oldBrush);
    DeleteObject(border);
    DeleteObject(background);

    HFONT font = CreateFontW(
        -24, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Yu Gothic UI");
    HGDIOBJ oldFont = font ? SelectObject(dc, font) : nullptr;
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(248, 248, 250));
    RECT textRect = {20, 8, rect.right - 20, rect.bottom - 8};
    DrawTextW(
        dc,
        m_text.c_str(),
        static_cast<int>(m_text.size()),
        &textRect,
        DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    if (oldFont) {
        SelectObject(dc, oldFont);
    }
    if (font) {
        DeleteObject(font);
    }
    EndPaint(m_window, &paint);
}

void StatusOsd::OnTimer(UINT_PTR timerId)
{
    if (timerId == kHoldTimerId) {
        KillTimer(m_window, kHoldTimerId);
        SetTimer(m_window, kFadeTimerId, 30, nullptr);
        return;
    }
    if (timerId != kFadeTimerId) {
        return;
    }

    m_alpha = static_cast<BYTE>(m_alpha > 24 ? m_alpha - 24 : 0);
    if (m_alpha == 0) {
        Hide();
        return;
    }
    SetLayeredWindowAttributes(m_window, 0, m_alpha, LWA_ALPHA);
}

LRESULT CALLBACK StatusOsd::WindowProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    if (message == WM_NCCREATE) {
        CREATESTRUCTW* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
    }

    StatusOsd* osd = reinterpret_cast<StatusOsd*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (osd) {
        if (message == WM_PAINT) {
            osd->Paint();
            return 0;
        }
        if (message == WM_TIMER) {
            osd->OnTimer(static_cast<UINT_PTR>(wParam));
            return 0;
        }
        if (message == WM_ERASEBKGND) {
            return 1;
        }
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

} // namespace KeyroIME
