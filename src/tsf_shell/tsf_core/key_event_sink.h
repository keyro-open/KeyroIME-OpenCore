// Copyright (C) 2025-2026 株式会社LocalPro (LocalPro Co., Ltd.). All rights reserved.
// Brand Official Website: https://keyro.jp
//
// This file is part of KeyroIME (キーロ) v1.0 OpenCore.
// KeyroIME is free software: you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software Foundation.
//
// For commercial use licensing, custom deployment, or proprietary integrations,
// please contact 株式会社LocalPro via https://localpro.jp. Unauthorized closed-source
// commercial exploitation is strictly prohibited.
// key_event_sink.h
// KeyroIME TSF シェル - キーイベントシンク
// TSF OnKeyDown/OnKeyUp イベントの捕捉と処理を担当します。

#pragma once
#include <windows.h>
#include <string>
#include "keyboard_layout.h"
#include "caps_lock_hook.h"

namespace KeyroIME {

/// Rust FFI 関数ポインター型定義
typedef char* (*MatchRomajiFunc)(const char*);
typedef void (*FreeStringFunc)(char*);

/// キーイベントシンク
class KeyEventSink {
public:
    KeyEventSink();
    ~KeyEventSink();

    /// 初期化（ロード Rust DLL）
    /// @param dllPath Rust エンジン DLL パス
    /// @return ロードに成功したかどうか
    bool Initialize(const wchar_t* dllPath);

    /// リソースを解放
    void Cleanup();

    /// キー押下イベントを処理します。
    /// @param wParam 仮想キーコード
    /// @param lParam スキャンコード等の付加情報
    /// @param outCommitText 出力: コミットするテキスト
    /// @return true は処理済み、false はシステムへ渡すことを示します。
    bool OnKeyDown(WPARAM wParam, LPARAM lParam, std::wstring& outCommitText);

    /// 設定キーボード配列
    void SetKeyboardLayout(KeyboardLayout layout);

    /// 現在の入力モードを取得します。
    ImeMode GetCurrentMode() const;

    /// 現在の入力モードを設定します。
    void SetCurrentMode(ImeMode mode);

private:
    // Rust DLL ハンドル
    HMODULE m_hRustDll;

    // Rust FFI 関数ポインター
    MatchRomajiFunc m_matchRomaji;
    FreeStringFunc m_freeString;

    // サブモジュール
    KeyboardLayoutMapper m_keyboardMapper;
    CapsLockHook m_capsLockHook;

    // 入力バッファー（ローマ字）
    std::string m_inputBuffer;

    /// Rust DLL をロードし、関数ポインターを取得します。
    bool LoadRustEngine(const wchar_t* dllPath);

    /// 文字入力を処理します。
    bool HandleCharInput(char ch, std::wstring& outCommitText);

    /// UTF-8 を Wide String へ変換します。
    std::wstring Utf8ToWide(const char* utf8Str);
};

} // namespace KeyroIME
