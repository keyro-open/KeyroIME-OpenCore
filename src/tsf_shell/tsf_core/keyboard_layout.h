// Copyright (C) 2025-2026 株式会社LocalPro (LocalPro Co., Ltd.). All rights reserved.
// Brand Official Website: https://keyro.jp
//
// This file is part of KeyroIME (キーロ) OpenCore.
// It is source-available under the KeyroIME OpenCore Non-Commercial Source
// License 1.0. See LICENSE. Commercial use requires a separate written license
// from 株式会社LocalPro.
// keyboard_layout.h
// KeyroIME TSF シェル - キーボード配列動的マッピングモジュール
// US/JIS キーボード配列の仮想キーコード変換を担当します。

#pragma once
#include <windows.h>

#include <string>

namespace KeyroIME {

/// キーボード配列型
enum KeyboardLayout {
    LAYOUT_JIS,   // JISキーボード
    LAYOUT_US     // USキーボード
};

/// キーボード配列変換クラス
class KeyboardLayoutMapper {
public:
    KeyboardLayoutMapper();
    ~KeyboardLayoutMapper();

    /// 現在のキーボード配列を設定します。
    void SetLayout(KeyboardLayout layout);

    /// 現在のキーボード配列を取得します。
    KeyboardLayout GetLayout() const;

    /// 入力を捕捉し、配列に応じて仮想キーコードを動的に変換します。
    /// @param wParam 元仮想キーコード
    /// @param outVKey 出力: 補正後の仮想キーコード
    /// @param outScanCode 出力: 対応するスキャンコード
    /// @return マッピング変換を行ったかどうか
    bool TranslateKeyLayout(WPARAM wParam, WPARAM& outVKey, BYTE& outScanCode);

    /// 文字キーまたは数字列キーを、現在の JIS/ANSI 配列に対応するコミット可能文字へ変換します。
    bool TranslatePrintableKey(
        WPARAM wParam,
        bool shiftPressed,
        wchar_t& outCharacter,
        UINT scanCode = 0) const;

private:
    KeyboardLayout m_currentLayout;
    
    /// Shift キーが押下中かどうかを確認します。
    bool IsShiftPressed() const;

    /// Shift + 数字キーの文字をマッピングします。
    bool MapShiftNumber(WPARAM wParam, WPARAM& outVKey);
    bool MapAnsiScanCode(UINT scanCode, bool shiftPressed, wchar_t& outCharacter) const;
    bool MapJisScanCode(UINT scanCode, bool shiftPressed, wchar_t& outCharacter) const;
};

/// 日本語句読点モード時に、ASCII 記号を日本語入力で期待される記号へ変換します。
/// 変換対象外の文字は空文字列を返します。
std::wstring JapaneseStyleSymbolFor(wchar_t character);

} // namespace KeyroIME
