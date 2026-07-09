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
// candidate_window.h
// KeyroIME TSF シェル - 純縦並び候補ウィンドウ
// Caret-following 形式。1ページ5件固定の候補を表示します。

#pragma once
#include <windows.h>
#include <vector>
#include <string>

namespace KeyroIME {

/// 候補項目構造体
struct CandidateItem {
    std::wstring text;       // 候補テキスト（タグを含む）
    bool has_tag;            // 簡略タグの有無（訳/地/人）
    std::wstring tag;        // 右揃えの1文字タグ
    std::wstring content;    // 実際内容
};

/// 純縦並び候補ウィンドウクラス
class CandidateWindow {
public:
    CandidateWindow();
    ~CandidateWindow();

    /// 候補ウィンドウを初期化します。
    bool Initialize(HINSTANCE hInstance);

    /// 候補ウィンドウを表示します（カーソル位置に追従）。
    /// @param caretPos カーソル位置
    /// @param candidates 候補一覧（最大5件）
    /// @param highlightIndex ハイライト索引（0-4）
    /// @param currentPage 現在ページ番号（0始まり）
    /// @param totalPages 総ページ数
    void Show(POINT caretPos, 
              const std::vector<std::wstring>& candidates,
              int highlightIndex,
              int currentPage,
              int totalPages);

    /// 隠す候補ウィンドウ
    void Hide();

    /// 更新ハイライト位置
    void UpdateHighlight(int highlightIndex);

    /// 表示中かどうか
    bool IsVisible() const { return m_isVisible; }

private:
    HWND m_hWnd;
    HINSTANCE m_hInstance;
    bool m_isVisible;

    // 候補データ
    std::vector<CandidateItem> m_candidates;
    int m_highlightIndex;
    int m_currentPage;
    int m_totalPages;
    UINT m_dpi;

    // 96 DPI での論理サイズ。描画時にウィンドウ DPI へ比例換算します。
    static constexpr int WINDOW_WIDTH = 340;
    static constexpr int ITEM_HEIGHT = 46;
    static constexpr int HEADER_HEIGHT = 12;
    static constexpr int FOOTER_HEIGHT = 38;
    static constexpr int PADDING = 14;
    static constexpr int INDEX_WIDTH = 40;
    static constexpr int TAG_WIDTH = 34;
    static constexpr int COLUMN_GAP = 12;
    static constexpr int FONT_POINT_SIZE = 15;

    // 色定数
    static const COLORREF COLOR_BG = RGB(255, 255, 255);           // 白背景
    static const COLORREF COLOR_TEXT = RGB(0, 0, 0);               // 黒文字
    static const COLORREF COLOR_HIGHLIGHT_BG = RGB(230, 240, 255); // ハイライト背景
    static const COLORREF COLOR_TAG = RGB(100, 100, 255);          // タグ色（青）
    static const COLORREF COLOR_BORDER = RGB(180, 180, 180);       // 枠線グレー

    /// ウィンドウクラスを登録
    bool RegisterWindowClass();

    /// 作成窗口
    bool CreateWindowInstance();

    /// ウィンドウプロシージャ
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    /// 描画候補ウィンドウ
    void OnPaint(HDC hdc);

    /// 解析候補（抽出タグ）
    CandidateItem ParseCandidate(const std::wstring& candidate);

    /// 単一候補項目を描画します。
    void DrawCandidateItem(HDC hdc, int index, const CandidateItem& item, bool isHighlight);

    /// 描画ページインジケーター
    void DrawPageIndicator(HDC hdc, int yPos);

    /// ウィンドウ全体の高さを計算します。
    int CalculateWindowHeight() const;

    int Scale(int logicalPixels) const;
    int WindowWidth() const;
};

} // namespace KeyroIME
