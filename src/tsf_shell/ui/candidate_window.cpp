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
// candidate_window.cpp
// KeyroIME TSF シェル - 純縦並び候補ウィンドウ実装

#include "candidate_window.h"

#include <algorithm>
#include <sstream>

namespace KeyroIME {

// ウィンドウクラス名
static const wchar_t* CANDIDATE_WINDOW_CLASS = L"KeyroIME_CandidateWindow";

CandidateWindow::CandidateWindow()
    : m_hWnd(nullptr)
    , m_hInstance(nullptr)
    , m_isVisible(false)
    , m_highlightIndex(0)
    , m_currentPage(0)
    , m_totalPages(1)
    , m_dpi(USER_DEFAULT_SCREEN_DPI)
{
}

CandidateWindow::~CandidateWindow()
{
    if (m_hWnd) {
        DestroyWindow(m_hWnd);
        m_hWnd = nullptr;
    }
    if (m_hInstance) {
        UnregisterClassW(CANDIDATE_WINDOW_CLASS, m_hInstance);
        m_hInstance = nullptr;
    }
}

bool CandidateWindow::Initialize(HINSTANCE hInstance)
{
    m_hInstance = hInstance;

    if (!RegisterWindowClass()) {
        return false;
    }

    if (!CreateWindowInstance()) {
        return false;
    }

    return true;
}

bool CandidateWindow::RegisterWindowClass()
{
    WNDCLASSEXW wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.style = CS_HREDRAW | CS_VREDRAW | CS_DROPSHADOW;
    wcex.lpfnWndProc = WindowProc;
    wcex.hInstance = m_hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = CANDIDATE_WINDOW_CLASS;

    if (RegisterClassExW(&wcex) != 0) {
        return true;
    }

    return GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
}

bool CandidateWindow::CreateWindowInstance()
{
    m_dpi = GetDpiForSystem();
    m_hWnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        CANDIDATE_WINDOW_CLASS,
        L"",
        WS_POPUP,
        0, 0, WindowWidth(), Scale(100),
        nullptr,
        nullptr,
        m_hInstance,
        this  // this ポインターを渡します
    );

    return m_hWnd != nullptr;
}

void CandidateWindow::Show(POINT caretPos,
                           const std::vector<std::wstring>& candidates,
                           int highlightIndex,
                           int currentPage,
                           int totalPages)
{
    if (!m_hWnd || !IsWindow(m_hWnd)) {
        m_hWnd = nullptr;
        if (!CreateWindowInstance()) {
            return;
        }
    }

    m_highlightIndex = highlightIndex;
    m_currentPage = currentPage;
    m_totalPages = totalPages;
    m_candidates.clear();

    // 解析候補（抽出タグ）
    for (const auto& candidate : candidates) {
        if (!candidate.empty()) {
            m_candidates.push_back(ParseCandidate(candidate));
        }
    }

    UINT windowDpi = GetDpiForWindow(m_hWnd);
    if (windowDpi != 0) {
        m_dpi = windowDpi;
    }

    // ウィンドウ位置とサイズを計算します。
    int windowWidth = WindowWidth();
    int windowHeight = CalculateWindowHeight();

    // Caret-following 位置決めロジック
    // 既定ではカーソル右下に表示します。
    int x = caretPos.x + Scale(18);
    int y = caretPos.y + Scale(14);

    // カーソルがあるモニターの作業領域を使い、マルチモニター環境でのはみ出しを防ぎます。
    MONITORINFO monitorInfo = {};
    monitorInfo.cbSize = sizeof(monitorInfo);
    HMONITOR monitor = MonitorFromPoint(caretPos, MONITOR_DEFAULTTONEAREST);
    if (!GetMonitorInfoW(monitor, &monitorInfo)) {
        SystemParametersInfoW(SPI_GETWORKAREA, 0, &monitorInfo.rcWork, 0);
    }
    const RECT& workArea = monitorInfo.rcWork;

    if (x + windowWidth > workArea.right) {
        x = caretPos.x - windowWidth - Scale(10); // 左側表示
    }

    if (y + windowHeight > workArea.bottom) {
        y = caretPos.y - windowHeight - Scale(10); // 上側表示
    }

    x = std::clamp(x, static_cast<int>(workArea.left),
                   static_cast<int>(workArea.right) - windowWidth);
    y = std::clamp(y, static_cast<int>(workArea.top),
                   static_cast<int>(workArea.bottom) - windowHeight);

    // ウィンドウ位置とサイズを設定します。
    SetWindowPos(m_hWnd, HWND_TOPMOST, x, y, windowWidth, windowHeight,
                 SWP_NOACTIVATE | SWP_SHOWWINDOW);
    ShowWindow(m_hWnd, SW_SHOWNA);

    m_isVisible = true;
    InvalidateRect(m_hWnd, nullptr, TRUE);
    UpdateWindow(m_hWnd);
}

void CandidateWindow::Hide()
{
    if (m_hWnd) {
        ShowWindow(m_hWnd, SW_HIDE);
        m_isVisible = false;
    }
}

void CandidateWindow::UpdateHighlight(int highlightIndex)
{
    if (m_highlightIndex != highlightIndex) {
        m_highlightIndex = highlightIndex;
        InvalidateRect(m_hWnd, nullptr, FALSE);
    }
}

int CandidateWindow::CalculateWindowHeight() const
{
    return Scale(HEADER_HEIGHT + (5 * ITEM_HEIGHT) + FOOTER_HEIGHT);
}

CandidateItem CandidateWindow::ParseCandidate(const std::wstring& candidate)
{
    CandidateItem item;
    item.text = candidate;
    item.has_tag = false;

    // コア層の旧タグを右側の1文字タグへ統一します。
    if (candidate.length() > 2 && candidate[0] == L'[') {
        size_t endBracket = candidate.find(L']');
        if (endBracket != std::wstring::npos) {
            std::wstring sourceTag = candidate.substr(1, endBracket - 1);
            if (sourceTag == L"訳" || sourceTag == L"翻訳") {
                item.tag = L"訳";
            } else if (sourceTag == L"地" || sourceTag == L"地名" ||
                       sourceTag == L"駅" || sourceTag == L"駅名") {
                item.tag = L"地";
            } else if (sourceTag == L"人" || sourceTag == L"人名") {
                item.tag = L"人";
            }
            item.has_tag = !item.tag.empty();

            // 実内容を抽出します（タグと空白をスキップ）。
            size_t contentStart = endBracket + 1;
            while (contentStart < candidate.length() && candidate[contentStart] == L' ') {
                contentStart++;
            }
            item.content = candidate.substr(contentStart);
            return item;
        }
    }

    item.content = candidate;
    return item;
}

LRESULT CALLBACK CandidateWindow::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CandidateWindow* pThis = nullptr;

    if (msg == WM_CREATE) {
        CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        pThis = reinterpret_cast<CandidateWindow*>(pCreate->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
    } else {
        pThis = reinterpret_cast<CandidateWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (pThis) {
        switch (msg) {
            case WM_PAINT:
            {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hwnd, &ps);
                pThis->OnPaint(hdc);
                EndPaint(hwnd, &ps);
                return 0;
            }

            case WM_ERASEBKGND:
                return 1; // ダブルバッファリングのため、背景消去は不要です。

            case WM_DPICHANGED:
            {
                pThis->m_dpi = HIWORD(wParam);
                RECT* suggested = reinterpret_cast<RECT*>(lParam);
                SetWindowPos(
                    hwnd,
                    nullptr,
                    suggested->left,
                    suggested->top,
                    suggested->right - suggested->left,
                    suggested->bottom - suggested->top,
                    SWP_NOACTIVATE | SWP_NOZORDER);
                InvalidateRect(hwnd, nullptr, TRUE);
                return 0;
            }

            case WM_DESTROY:
                return 0;
        }
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void CandidateWindow::OnPaint(HDC hdc)
{
    RECT clientRect;
    GetClientRect(m_hWnd, &clientRect);

    // === ダブルバッファリング仕組み：ちらつきを防止 ===
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBitmap = CreateCompatibleBitmap(hdc, clientRect.right, clientRect.bottom);
    HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);

    // ポイント値でフォントを作成し、高 DPI 環境でも安定した視覚比率を保ちます。
    LOGFONTW logFont = {};
    logFont.lfHeight = -MulDiv(FONT_POINT_SIZE, static_cast<int>(m_dpi), 72);
    logFont.lfWeight = FW_NORMAL;
    logFont.lfCharSet = DEFAULT_CHARSET;
    logFont.lfOutPrecision = OUT_DEFAULT_PRECIS;
    logFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    logFont.lfQuality = CLEARTYPE_NATURAL_QUALITY;
    logFont.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
    wcscpy_s(logFont.lfFaceName, L"Yu Gothic UI");
    HFONT hFont = CreateFontIndirectW(&logFont);
    HFONT paintFont = hFont
        ? hFont
        : static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    HFONT oldFont = static_cast<HFONT>(SelectObject(memDC, paintFont));

    // 白背景を描画します。
    HBRUSH bgBrush = CreateSolidBrush(COLOR_BG);
    FillRect(memDC, &clientRect, bgBrush);
    DeleteObject(bgBrush);

    // 枠線を描画します。
    HPEN borderPen = CreatePen(PS_SOLID, 1, COLOR_BORDER);
    HPEN oldPen = (HPEN)SelectObject(memDC, borderPen);
    Rectangle(memDC, 0, 0, clientRect.right, clientRect.bottom);
    SelectObject(memDC, oldPen);
    DeleteObject(borderPen);

    // 上部かな表示領域を描画します（任意）。
    // 5件の候補項目を描画します。
    for (size_t i = 0; i < m_candidates.size() && i < 5; ++i) {
        bool isHighlight = (static_cast<int>(i) == m_highlightIndex);
        DrawCandidateItem(memDC, static_cast<int>(i), m_candidates[i], isHighlight);
    }

    // 5行とタグ列を軽く区切り、ページごとの視認リズムを一定に保ちます。
    HPEN separatorPen = CreatePen(PS_SOLID, 1, RGB(232, 232, 232));
    HPEN previousSeparatorPen = static_cast<HPEN>(SelectObject(memDC, separatorPen));
    int candidatesTop = Scale(HEADER_HEIGHT);
    int itemHeight = Scale(ITEM_HEIGHT);
    int candidatesBottom = candidatesTop + (5 * itemHeight);
    for (int row = 1; row < 5; ++row) {
        int separatorY = candidatesTop + (row * itemHeight);
        MoveToEx(memDC, 0, separatorY, nullptr);
        LineTo(memDC, clientRect.right, separatorY);
    }
    int tagDividerX = clientRect.right - Scale(PADDING + TAG_WIDTH + (COLUMN_GAP / 2));
    MoveToEx(memDC, tagDividerX, candidatesTop, nullptr);
    LineTo(memDC, tagDividerX, candidatesBottom);
    SelectObject(memDC, previousSeparatorPen);
    DeleteObject(separatorPen);

    // 描画下部ページインジケーター
    int footerY = Scale(HEADER_HEIGHT + (5 * ITEM_HEIGHT));
    DrawPageIndicator(memDC, footerY);

    // ダブルバッファリング：画面へコピーします。
    BitBlt(hdc, 0, 0, clientRect.right, clientRect.bottom, memDC, 0, 0, SRCCOPY);

    // リソースを解放
    SelectObject(memDC, oldFont);
    SelectObject(memDC, oldBitmap);
    if (hFont) {
        DeleteObject(hFont);
    }
    DeleteObject(memBitmap);
    DeleteDC(memDC);
}

void CandidateWindow::DrawCandidateItem(HDC hdc, int index, const CandidateItem& item, bool isHighlight)
{
    int yPos = Scale(HEADER_HEIGHT + (index * ITEM_HEIGHT));
    int itemHeight = Scale(ITEM_HEIGHT);
    int padding = Scale(PADDING);
    int indexWidth = Scale(INDEX_WIDTH);
    int tagWidth = Scale(TAG_WIDTH);
    int columnGap = Scale(COLUMN_GAP);
    int windowWidth = WindowWidth();

    RECT itemRect = { 0, yPos, windowWidth, yPos + itemHeight };

    // ハイライト背景を描画します。
    if (isHighlight) {
        HBRUSH highlightBrush = CreateSolidBrush(COLOR_HIGHLIGHT_BG);
        FillRect(hdc, &itemRect, highlightBrush);
        DeleteObject(highlightBrush);
    }

    // 左側の数字索引を描画します。 "1."
    wchar_t indexText[4];
    wsprintfW(indexText, L"%d.", index + 1);

    RECT indexRect = { padding, yPos, indexWidth, yPos + itemHeight };
    SetTextColor(hdc, RGB(92, 92, 92));
    SetBkMode(hdc, TRANSPARENT);
    DrawTextW(hdc, indexText, -1, &indexRect,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

    // 候補本文は常に左列を使い、右端タグ列には固定幅を確保します。
    int contentX = indexWidth + padding;
    int tagLeft = windowWidth - padding - tagWidth;
    int contentRight = tagLeft - columnGap;
    RECT contentRect = { contentX, yPos, contentRight, yPos + itemHeight };
    SetTextColor(hdc, COLOR_TEXT);
    DrawTextW(hdc, item.content.c_str(), -1, &contentRect,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);

    // 1文字タグは右端列に独立して描画し、候補本文とは連結しません。
    if (item.has_tag) {
        RECT tagRect = { tagLeft, yPos, windowWidth - padding, yPos + itemHeight };
        SetTextColor(hdc, COLOR_TAG);
        DrawTextW(hdc, item.tag.c_str(), -1, &tagRect,
                  DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    }
}

void CandidateWindow::DrawPageIndicator(HDC hdc, int yPos)
{
    // 描画ページインジケーター: ◀ 1 / 3 ▶
    std::wstringstream ss;
    ss << L"◀  " << (m_currentPage + 1) << L" / " << m_totalPages << L"  ▶";
    std::wstring pageText = ss.str();

    RECT pageRect = { 0, yPos, WindowWidth(), yPos + Scale(FOOTER_HEIGHT) };

    // 薄いグレー背景を描画します。
    HBRUSH footerBrush = CreateSolidBrush(RGB(250, 250, 250));
    FillRect(hdc, &pageRect, footerBrush);
    DeleteObject(footerBrush);

    // ページテキストを中央揃えで描画します。
    SetTextColor(hdc, RGB(100, 100, 100));
    SetBkMode(hdc, TRANSPARENT);
    DrawTextW(hdc, pageText.c_str(), -1, &pageRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

int CandidateWindow::Scale(int logicalPixels) const
{
    return MulDiv(logicalPixels, static_cast<int>(m_dpi), USER_DEFAULT_SCREEN_DPI);
}

int CandidateWindow::WindowWidth() const
{
    return Scale(WINDOW_WIDTH);
}

} // namespace KeyroIME
