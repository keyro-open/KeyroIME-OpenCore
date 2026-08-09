// Copyright (C) 2025-2026 株式会社LocalPro (LocalPro Co., Ltd.). All rights reserved.
// Brand Official Website: https://keyro.jp
//
// This file is part of KeyroIME (キーロ) OpenCore.
// GNU GPLv3に基づいて配布されます。LICENSE（英語正文）を参照してください。
// caps_lock_hook.cpp
// KeyroIME TSF シェル - Caps Lock 動作制御モジュール実装

#include "caps_lock_hook.h"

namespace KeyroIME {

CapsLockHook::CapsLockHook()
    : m_currentMode(MODE_HIRAGANA) // 既定はひらがなモード
{
}

CapsLockHook::~CapsLockHook()
{
}

void CapsLockHook::SetMode(ImeMode mode)
{
    m_currentMode = mode;
}

ImeMode CapsLockHook::GetMode() const
{
    return m_currentMode;
}

bool CapsLockHook::HandleCapsLock(WPARAM wParam)
{
    if (wParam == VK_CAPITAL) {
        // 英語大文字モードへ切り替えます。
        m_currentMode = MODE_ENGLISH_UPPERCASE;
        
        // システム既定の Caps Lock 切替動作を抑止します。
        // true を返すとイベントは完全に消費済みです。
        return true;
    }
    
    return false;
}

bool CapsLockHook::HandleLetterKey(WPARAM wParam, wchar_t& outChar)
{
    // 大文字ロックモードの場合のみ処理します。
    if (m_currentMode != MODE_ENGLISH_UPPERCASE) {
        return false;
    }

    // 文字キー A-Z かどうかを確認します。
    if (wParam >= 'A' && wParam <= 'Z') {
        // Rust エンジンを経由せず、大文字を直接出力します。
        outChar = static_cast<wchar_t>(wParam);
        return true;
    }

    // 小文字入力キー（実際にはシステムは大文字仮想キーコードを送ります）
    if (wParam >= 'a' && wParam <= 'z') {
        outChar = static_cast<wchar_t>(wParam - 32); // 大文字へ変換
        return true;
    }

    return false;
}

bool CapsLockHook::IsUppercaseMode() const
{
    return m_currentMode == MODE_ENGLISH_UPPERCASE;
}

} // namespace KeyroIME
