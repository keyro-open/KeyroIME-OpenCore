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
// key_event_sink.cpp
// KeyroIME TSF シェル - キーイベントシンク実装

#include "key_event_sink.h"
#include "module_path.h"
#include <codecvt>
#include <locale>

namespace KeyroIME {

namespace {

void KeyEventSinkModuleAnchor()
{
}

} // namespace

KeyEventSink::KeyEventSink()
    : m_hRustDll(nullptr)
    , m_matchRomaji(nullptr)
    , m_freeString(nullptr)
{
}

KeyEventSink::~KeyEventSink()
{
    Cleanup();
}

bool KeyEventSink::Initialize(const wchar_t* dllPath)
{
    UNREFERENCED_PARAMETER(dllPath);

    HMODULE currentModule = nullptr;
    if (!GetModuleHandleExW(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&KeyEventSinkModuleAnchor),
            &currentModule)) {
        return false;
    }

    std::wstring absolutePath;
    if (!BuildModuleSiblingPath(currentModule, L"ime_core.dll", absolutePath)) {
        return false;
    }
    return LoadRustEngine(absolutePath.c_str());
}

void KeyEventSink::Cleanup()
{
    if (m_hRustDll) {
        FreeLibrary(m_hRustDll);
        m_hRustDll = nullptr;
        m_matchRomaji = nullptr;
        m_freeString = nullptr;
    }
}

bool KeyEventSink::OnKeyDown(WPARAM wParam, LPARAM lParam, std::wstring& outCommitText)
{
    outCommitText.clear();

    // 1. Caps Lock を優先処理します。
    if (m_capsLockHook.HandleCapsLock(wParam)) {
        // Caps Lock が押下されたため、大文字モードへ切り替えます。
        return true; // イベントは消費済みです。
    }

    // 2. 大文字ロックモードでは大文字を直接コミットします。
    wchar_t upperChar = 0;
    if (m_capsLockHook.HandleLetterKey(wParam, upperChar)) {
        outCommitText = upperChar;
        return true; // Rust エンジンを経由せず直接コミットします。
    }

    // 3. キーボード配列マッピング
    WPARAM mappedVKey = wParam;
    BYTE scanCode = 0;
    m_keyboardMapper.TranslateKeyLayout(wParam, mappedVKey, scanCode);

    // 4. 文字入力を処理します。（ローマ字）
    if (mappedVKey >= 'A' && mappedVKey <= 'Z') {
        // 小文字へ変換します。
        char ch = static_cast<char>(mappedVKey + 32);
        return HandleCharInput(ch, outCommitText);
    }

    if (mappedVKey >= 'a' && mappedVKey <= 'z') {
        char ch = static_cast<char>(mappedVKey);
        return HandleCharInput(ch, outCommitText);
    }

    // 5. 数字キーを処理します。
    if (mappedVKey >= '0' && mappedVKey <= '9') {
        // 数字を直接コミットします（Empty 状態）
        if (m_inputBuffer.empty()) {
            outCommitText = static_cast<wchar_t>(mappedVKey);
            return true;
        }
    }

    // 6. 処理特殊キー（Space、Enter等）
    switch (mappedVKey) {
        case VK_SPACE:
            // Space：発動変換
            if (!m_inputBuffer.empty()) {
                // Rust エンジン変換を呼び出します。
                if (m_matchRomaji) {
                    char* result = m_matchRomaji(m_inputBuffer.c_str());
                    if (result) {
                        outCommitText = Utf8ToWide(result);
                        m_freeString(result);
                        m_inputBuffer.clear();
                        return true;
                    }
                }
            } else {
                outCommitText = L" ";
                return true;
            }
            break;

        case VK_RETURN:
            // Enter: 現在のバッファー（変換後かな）をコミットします。
            if (!m_inputBuffer.empty()) {
                if (m_matchRomaji) {
                    char* result = m_matchRomaji(m_inputBuffer.c_str());
                    if (result) {
                        outCommitText = Utf8ToWide(result);
                        m_freeString(result);
                        m_inputBuffer.clear();
                        return true;
                    }
                }
            }
            break;

        case VK_BACK:
            // Backspace: 最後の1文字を削除します。
            if (!m_inputBuffer.empty()) {
                m_inputBuffer.pop_back();
                return true;
            }
            break;
    }

    return false;
}

void KeyEventSink::SetKeyboardLayout(KeyboardLayout layout)
{
    m_keyboardMapper.SetLayout(layout);
}

ImeMode KeyEventSink::GetCurrentMode() const
{
    return m_capsLockHook.GetMode();
}

void KeyEventSink::SetCurrentMode(ImeMode mode)
{
    m_capsLockHook.SetMode(mode);
}

bool KeyEventSink::LoadRustEngine(const wchar_t* dllPath)
{
    // ロード DLL
    m_hRustDll = LoadLibraryW(dllPath);
    if (!m_hRustDll) {
        return false;
    }

    // 取得関数ポインター
    m_matchRomaji = reinterpret_cast<MatchRomajiFunc>(
        GetProcAddress(m_hRustDll, "match_romaji"));
    m_freeString = reinterpret_cast<FreeStringFunc>(
        GetProcAddress(m_hRustDll, "free_string"));

    if (!m_matchRomaji || !m_freeString) {
        FreeLibrary(m_hRustDll);
        m_hRustDll = nullptr;
        return false;
    }

    return true;
}

bool KeyEventSink::HandleCharInput(char ch, std::wstring& outCommitText)
{
    // 入力バッファーへ追加します。
    m_inputBuffer.push_back(ch);

    // まだコミットせず、後続キー（Space/Enter）を待ちます。
    return true;
}

std::wstring KeyEventSink::Utf8ToWide(const char* utf8Str)
{
    if (!utf8Str) {
        return L"";
    }

    int wideLen = MultiByteToWideChar(CP_UTF8, 0, utf8Str, -1, nullptr, 0);
    if (wideLen <= 0) {
        return L"";
    }

    std::wstring result(static_cast<size_t>(wideLen), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8Str, -1, result.data(), wideLen);
    result.resize(static_cast<size_t>(wideLen - 1));

    return result;
}

} // namespace KeyroIME
