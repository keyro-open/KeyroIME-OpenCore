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
// keyboard_layout.cpp
// KeyroIME TSF シェル - キーボード配列動的マッピングモジュール実装

#include "keyboard_layout.h"

namespace KeyroIME {

KeyboardLayoutMapper::KeyboardLayoutMapper()
    : m_currentLayout(LAYOUT_JIS) // 既定は JIS キーボード
{
}

KeyboardLayoutMapper::~KeyboardLayoutMapper()
{
}

void KeyboardLayoutMapper::SetLayout(KeyboardLayout layout)
{
    m_currentLayout = layout;
}

KeyboardLayout KeyboardLayoutMapper::GetLayout() const
{
    return m_currentLayout;
}

bool KeyboardLayoutMapper::TranslateKeyLayout(WPARAM wParam, WPARAM& outVKey, BYTE& outScanCode)
{
    outVKey = wParam;
    outScanCode = 0;

    // US キーボード配列の場合のみマッピングが必要です。
    if (m_currentLayout != LAYOUT_US) {
        // JIS 配列ではシステム既定のマッピングをそのまま使用します。
        outScanCode = static_cast<BYTE>(MapVirtualKeyW(static_cast<UINT>(wParam), MAPVK_VK_TO_VSC));
        return false;
    }

    // USキーボード配列マッピングロジック
    bool mapped = false;

    // Shift キー状態を確認し、数字キーをマッピングします。
    if (IsShiftPressed()) {
        mapped = MapShiftNumber(wParam, outVKey);
    }

    // 取得スキャンコード
    outScanCode = static_cast<BYTE>(MapVirtualKeyW(static_cast<UINT>(outVKey), MAPVK_VK_TO_VSC));

    return mapped;
}

bool KeyboardLayoutMapper::TranslatePrintableKey(
    WPARAM wParam,
    bool shiftPressed,
    wchar_t& outCharacter,
    UINT scanCode) const
{
    if (wParam >= L'A' && wParam <= L'Z') {
        outCharacter = static_cast<wchar_t>(shiftPressed
            ? wParam
            : wParam - L'A' + L'a');
        return true;
    }

    if (scanCode != 0) {
        bool mapped = m_currentLayout == LAYOUT_US
            ? MapAnsiScanCode(scanCode, shiftPressed, outCharacter)
            : MapJisScanCode(scanCode, shiftPressed, outCharacter);
        if (mapped) {
            return true;
        }
    }

    if (wParam < L'0' || wParam > L'9') {
        switch (wParam) {
            case VK_SPACE: outCharacter = L' '; return true;
            case VK_OEM_1:
                outCharacter = m_currentLayout == LAYOUT_US
                    ? (shiftPressed ? L':' : L';')
                    : (shiftPressed ? L'*' : L':');
                return true;
            case VK_OEM_PLUS:
                outCharacter = m_currentLayout == LAYOUT_US
                    ? (shiftPressed ? L'+' : L'=')
                    : (shiftPressed ? L'+' : L';');
                return true;
            case VK_OEM_COMMA: outCharacter = shiftPressed ? L'<' : L','; return true;
            case VK_OEM_MINUS: outCharacter = shiftPressed ? L'_' : L'-'; return true;
            case VK_OEM_PERIOD: outCharacter = shiftPressed ? L'>' : L'.'; return true;
            case VK_OEM_2: outCharacter = shiftPressed ? L'?' : L'/'; return true;
            case VK_OEM_3:
                outCharacter = m_currentLayout == LAYOUT_US
                    ? (shiftPressed ? L'~' : L'`')
                    : (shiftPressed ? L'`' : L'@');
                return true;
            case VK_OEM_4: outCharacter = shiftPressed ? L'{' : L'['; return true;
            case VK_OEM_5: outCharacter = shiftPressed ? L'|' : L'\\'; return true;
            case VK_OEM_6: outCharacter = shiftPressed ? L'}' : L']'; return true;
            case VK_OEM_7:
                outCharacter = m_currentLayout == LAYOUT_US
                    ? (shiftPressed ? L'"' : L'\'')
                    : (shiftPressed ? L'~' : L'^');
                return true;
            case VK_OEM_102:
                outCharacter = shiftPressed ? L'_' : L'\\';
                return true;
            default: return false;
        }
    }
    if (!shiftPressed) {
        outCharacter = static_cast<wchar_t>(wParam);
        return true;
    }

    if (m_currentLayout == LAYOUT_US) {
        constexpr wchar_t kAnsiShiftDigits[] = L")!@#$%^&*(";
        outCharacter = kAnsiShiftDigits[wParam - L'0'];
        return true;
    }

    switch (wParam) {
        case L'1': outCharacter = L'!'; return true;
        case L'2': outCharacter = L'\"'; return true;
        case L'3': outCharacter = L'#'; return true;
        case L'4': outCharacter = L'$'; return true;
        case L'5': outCharacter = L'%'; return true;
        case L'6': outCharacter = L'&'; return true;
        case L'7': outCharacter = L'\''; return true;
        case L'8': outCharacter = L'('; return true;
        case L'9': outCharacter = L')'; return true;
        case L'0': outCharacter = L'0'; return true;
        default: return false;
    }
}

bool KeyboardLayoutMapper::MapAnsiScanCode(
    UINT scanCode,
    bool shiftPressed,
    wchar_t& outCharacter) const
{
    if (scanCode >= 0x02 && scanCode <= 0x0B) {
        constexpr wchar_t kDigits[] = L"1234567890";
        constexpr wchar_t kShiftDigits[] = L"!@#$%^&*()";
        size_t index = static_cast<size_t>(scanCode - 0x02);
        outCharacter = shiftPressed ? kShiftDigits[index] : kDigits[index];
        return true;
    }

    switch (scanCode) {
        case 0x0C: outCharacter = shiftPressed ? L'_' : L'-'; return true;
        case 0x0D: outCharacter = shiftPressed ? L'+' : L'='; return true;
        case 0x1A: outCharacter = shiftPressed ? L'{' : L'['; return true;
        case 0x1B: outCharacter = shiftPressed ? L'}' : L']'; return true;
        case 0x27: outCharacter = shiftPressed ? L':' : L';'; return true;
        case 0x28: outCharacter = shiftPressed ? L'"' : L'\''; return true;
        case 0x29: outCharacter = shiftPressed ? L'~' : L'`'; return true;
        case 0x2B: outCharacter = shiftPressed ? L'|' : L'\\'; return true;
        case 0x33: outCharacter = shiftPressed ? L'<' : L','; return true;
        case 0x34: outCharacter = shiftPressed ? L'>' : L'.'; return true;
        case 0x35: outCharacter = shiftPressed ? L'?' : L'/'; return true;
        case 0x73:
        case 0x7D: outCharacter = shiftPressed ? L'|' : L'\\'; return true;
        default: return false;
    }
}

bool KeyboardLayoutMapper::MapJisScanCode(
    UINT scanCode,
    bool shiftPressed,
    wchar_t& outCharacter) const
{
    if (scanCode >= 0x02 && scanCode <= 0x0B) {
        constexpr wchar_t kDigits[] = L"1234567890";
        constexpr wchar_t kShiftDigits[] = L"!\"#$%&'()0";
        size_t index = static_cast<size_t>(scanCode - 0x02);
        outCharacter = shiftPressed ? kShiftDigits[index] : kDigits[index];
        return true;
    }

    switch (scanCode) {
        case 0x0C: outCharacter = shiftPressed ? L'=' : L'-'; return true;
        case 0x0D: outCharacter = shiftPressed ? L'~' : L'^'; return true;
        case 0x1A: outCharacter = shiftPressed ? L'`' : L'@'; return true;
        case 0x1B: outCharacter = shiftPressed ? L'{' : L'['; return true;
        case 0x27: outCharacter = shiftPressed ? L'+' : L';'; return true;
        case 0x28: outCharacter = shiftPressed ? L'*' : L':'; return true;
        case 0x2B: outCharacter = shiftPressed ? L'}' : L']'; return true;
        case 0x33: outCharacter = shiftPressed ? L'<' : L','; return true;
        case 0x34: outCharacter = shiftPressed ? L'>' : L'.'; return true;
        case 0x35: outCharacter = shiftPressed ? L'?' : L'/'; return true;
        case 0x73: outCharacter = shiftPressed ? L'_' : L'\\'; return true;
        case 0x7D: outCharacter = shiftPressed ? L'|' : L'\\'; return true;
        default: return false;
    }
}

bool KeyboardLayoutMapper::IsShiftPressed() const
{
    return (GetKeyState(VK_SHIFT) & 0x8000) != 0;
}

bool KeyboardLayoutMapper::MapShiftNumber(WPARAM wParam, WPARAM& outVKey)
{
    // US キーボードの Shift + 数字キーのマッピング
    // JIS キーボードと US キーボードでは記号位置が異なります。
    switch (wParam) {
        case '2':
            // JIS: Shift+2 -> "
            // US:  Shift+2 -> @
            // 仮想キーコードは維持し、TSF 層で文字マッピングを処理します。
            outVKey = '2';
            return true;

        case '6':
            // JIS: Shift+6 -> &
            // US:  Shift+6 -> ^
            outVKey = '6';
            return true;

        case '7':
            // JIS: Shift+7 -> '
            // US:  Shift+7 -> &
            outVKey = '7';
            return true;

        case '8':
            // JIS: Shift+8 -> (
            // US:  Shift+8 -> *
            outVKey = '8';
            return true;

        case '9':
            // JIS: Shift+9 -> )
            // US:  Shift+9 -> (
            outVKey = '9';
            return true;

        case '0':
            // JIS: Shift+0 -> 標準記号なし
            // US:  Shift+0 -> )
            outVKey = '0';
            return true;

        default:
            return false;
    }
}

std::wstring JapaneseStyleSymbolFor(wchar_t character)
{
    switch (character) {
        case L',': return L"、";
        case L'.': return L"。";
        case L'/': return L"・";
        case L'\\': return L"￥";
        case L'!': return L"！";
        case L'"': return L"”";
        case L'#': return L"＃";
        case L'$': return L"＄";
        case L'%': return L"％";
        case L'&': return L"＆";
        case L'\'': return L"’";
        case L'(': return L"（";
        case L')': return L"）";
        case L'[': return L"「";
        case L']': return L"」";
        case L'{': return L"｛";
        case L'}': return L"｝";
        case L':': return L"：";
        case L';': return L"；";
        case L'?': return L"？";
        case L'<': return L"＜";
        case L'>': return L"＞";
        case L'@': return L"＠";
        case L'-': return L"ー";
        case L'_': return L"＿";
        case L'=': return L"＝";
        case L'+': return L"＋";
        case L'^': return L"＾";
        case L'~': return L"〜";
        case L'`': return L"｀";
        case L'|': return L"｜";
        case L'*': return L"＊";
        default: return std::wstring();
    }
}

} // namespace KeyroIME
