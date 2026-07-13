// Copyright (C) 2025-2026 株式会社LocalPro (LocalPro Co., Ltd.). All rights reserved.
// Brand Official Website: https://keyro.jp
//
// This file is part of KeyroIME (キーロ) OpenCore.
// It is source-available under the KeyroIME OpenCore Non-Commercial Source
// License 1.0. See LICENSE. Commercial use requires a separate written license
// from 株式会社LocalPro.
// caps_lock_hook.h
// KeyroIME TSF シェル - Caps Lock 動作制御モジュール
// Caps Lock キーを制御し、英語大文字を直接コミットします。

#pragma once
#include <windows.h>

namespace KeyroIME {

/// 入力モード列挙型
enum ImeMode {
    MODE_HIRAGANA,           // ひらがなモード
    MODE_KATAKANA,           // カタカナモード
    MODE_ENGLISH_LOWERCASE,  // 英語小文字モード
    MODE_ENGLISH_UPPERCASE   // 英語大文字モード（Caps Lock 有効）
};

/// Caps Lock 制御マネージャー
class CapsLockHook {
public:
    CapsLockHook();
    ~CapsLockHook();

    /// 現在の入力モードを設定します。
    void SetMode(ImeMode mode);

    /// 現在の入力モードを取得します。
    ImeMode GetMode() const;

    /// Caps Lock キー押下イベントを処理します。
    /// @param wParam 仮想キーコード
    /// @return true はイベントを消費済みとして下位へ渡さないことを示します。
    bool HandleCapsLock(WPARAM wParam);

    /// 文字キーイベントを処理します（大文字ロックモード時）。
    /// @param wParam 仮想キーコード
    /// @param outChar 出力：コミットが必要な文字
    /// @return true は Rust エンジンを経由せず直接コミットすることを示します。
    bool HandleLetterKey(WPARAM wParam, wchar_t& outChar);

    /// 大文字ロックモード中かどうかを確認します。
    bool IsUppercaseMode() const;

private:
    ImeMode m_currentMode;
};

} // namespace KeyroIME
