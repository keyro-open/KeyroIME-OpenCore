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
#include "shared_settings.h"

#include <sddl.h>

namespace KeyroIME {

namespace {

// v4 adds writable AppContainer access and lets long-lived TSF hosts reconnect
// after the tray creates the mapping.
constexpr wchar_t kSettingsMappingName[] = L"Local\\KeyroIME.Settings.v4";
constexpr LONG kSettingsMagic = 0x4B524F31;
constexpr LONG kSettingsVersion = 4;
constexpr WPARAM kVkDbeAlphanumeric = 0xF0;
constexpr WPARAM kVkDbeKatakana = 0xF1;
constexpr WPARAM kVkDbeHiragana = 0xF2;

bool IsInputModeValue(LONG value)
{
    return value >= static_cast<LONG>(SharedInputMode::Hiragana) &&
        value <= static_cast<LONG>(SharedInputMode::English);
}

bool IsCharWidthValue(LONG value)
{
    return value >= static_cast<LONG>(SharedCharWidth::HalfWidth) &&
        value <= static_cast<LONG>(SharedCharWidth::FullWidth);
}

bool IsKeyboardLayoutValue(LONG value)
{
    return value >= static_cast<LONG>(SharedKeyboardLayout::Jis) &&
        value <= static_cast<LONG>(SharedKeyboardLayout::Ansi);
}

bool IsPunctuationStyleValue(LONG value)
{
    return value >= static_cast<LONG>(SharedPunctuationStyle::Japanese) &&
        value <= static_cast<LONG>(SharedPunctuationStyle::Western);
}

bool IsEnglishCaseValue(LONG value)
{
    return value >= static_cast<LONG>(SharedEnglishCase::Lower) &&
        value <= static_cast<LONG>(SharedEnglishCase::Upper);
}

LONG PackSettings(const InputSettingsSnapshot& snapshot)
{
    return static_cast<LONG>(snapshot.inputMode) |
        (static_cast<LONG>(snapshot.charWidth) << 2) |
        (static_cast<LONG>(snapshot.keyboardLayout) << 3) |
        (static_cast<LONG>(snapshot.punctuationStyle) << 4) |
        (static_cast<LONG>(snapshot.englishCase) << 5);
}

} // namespace

struct SharedInputSettings::SharedState {
    volatile LONG magic;
    volatile LONG version;
    volatile LONG packedSettings;
};

InputSettingsSnapshot ToggleCapsLockSetting(const InputSettingsSnapshot& snapshot)
{
    InputSettingsSnapshot next = snapshot;
    next.englishCase = next.englishCase == SharedEnglishCase::Lower
        ? SharedEnglishCase::Upper
        : SharedEnglishCase::Lower;
    return next;
}

InputSettingsSnapshot ToggleCharacterWidthSetting(const InputSettingsSnapshot& snapshot)
{
    InputSettingsSnapshot next = snapshot;
    next.charWidth = next.charWidth == SharedCharWidth::HalfWidth
        ? SharedCharWidth::FullWidth
        : SharedCharWidth::HalfWidth;
    return next;
}

InputSettingsSnapshot ToggleKeyboardLayoutSetting(const InputSettingsSnapshot& snapshot)
{
    InputSettingsSnapshot next = snapshot;
    next.keyboardLayout = next.keyboardLayout == SharedKeyboardLayout::Jis
        ? SharedKeyboardLayout::Ansi
        : SharedKeyboardLayout::Jis;
    return next;
}

InputSettingsSnapshot CycleKanaWidthSetting(const InputSettingsSnapshot& snapshot)
{
    InputSettingsSnapshot next = snapshot;
    if (next.inputMode == SharedInputMode::Katakana &&
        next.charWidth == SharedCharWidth::FullWidth) {
        next.charWidth = SharedCharWidth::HalfWidth;
    } else if (next.inputMode == SharedInputMode::Katakana) {
        next.inputMode = SharedInputMode::Hiragana;
        next.charWidth = SharedCharWidth::HalfWidth;
    } else {
        next.inputMode = SharedInputMode::Katakana;
        next.charWidth = SharedCharWidth::FullWidth;
    }
    return next;
}

bool IsKeyboardLayoutToggleShortcut(WPARAM wParam, LPARAM lParam, bool altPressed)
{
    if (!altPressed) {
        return false;
    }

    UINT scanCode = static_cast<UINT>((static_cast<ULONG_PTR>(lParam) >> 16) & 0xFF);
    if (scanCode != 0) {
        // Physical semicolon key for both ANSI and JIS keyboards.
        return scanCode == 0x27;
    }
    return wParam == VK_OEM_1 || wParam == VK_OEM_PLUS;
}

bool IsImeModeToggleKey(WPARAM wParam)
{
    return wParam == VK_KANJI ||
        wParam == VK_IME_ON ||
        wParam == VK_IME_OFF ||
        wParam == kVkDbeAlphanumeric;
}

bool IsExplicitHiraganaModeKey(WPARAM wParam)
{
    return wParam == kVkDbeHiragana;
}

bool IsExplicitKatakanaModeKey(WPARAM wParam)
{
    return wParam == kVkDbeKatakana;
}

bool IsKanaModeKey(WPARAM wParam)
{
    return wParam == VK_KANA ||
        IsExplicitHiraganaModeKey(wParam) ||
        IsExplicitKatakanaModeKey(wParam);
}

bool IsConvertKey(WPARAM wParam)
{
    return wParam == VK_CONVERT;
}

bool IsNonConvertKey(WPARAM wParam)
{
    return wParam == VK_NONCONVERT;
}

SharedInputSettings::SharedInputSettings()
    : m_mapping(nullptr)
    , m_state(nullptr)
    , m_writable(false)
{
}

SharedInputSettings::~SharedInputSettings()
{
    Close();
}

bool SharedInputSettings::Initialize(bool createIfMissing)
{
    Close();
    m_writable = false;

    bool created = false;
    if (createIfMissing) {
        PSECURITY_DESCRIPTOR descriptor = nullptr;
        SECURITY_ATTRIBUTES attributes = {};
        attributes.nLength = sizeof(attributes);
        if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
                L"D:P(A;;GA;;;SY)(A;;GA;;;BA)(A;;GA;;;IU)(A;;GA;;;OW)"
                L"(A;;GA;;;S-1-15-2-1)(A;;GA;;;S-1-15-2-2)"
                L"S:(ML;;NW;;;LW)",
                SDDL_REVISION_1,
                &descriptor,
                nullptr)) {
            return false;
        }
        attributes.lpSecurityDescriptor = descriptor;

        m_mapping = CreateFileMappingW(
            INVALID_HANDLE_VALUE,
            &attributes,
            PAGE_READWRITE,
            0,
            sizeof(SharedState),
            kSettingsMappingName);
        created = m_mapping && GetLastError() != ERROR_ALREADY_EXISTS;
        if (descriptor) {
            LocalFree(descriptor);
        }
    } else {
        m_mapping = OpenFileMappingW(FILE_MAP_READ, FALSE, kSettingsMappingName);
    }

    if (!m_mapping) {
        return false;
    }

    DWORD access = createIfMissing ? FILE_MAP_ALL_ACCESS : FILE_MAP_READ;
    m_state = static_cast<SharedState*>(MapViewOfFile(m_mapping, access, 0, 0, sizeof(SharedState)));
    if (!m_state) {
        Close();
        return false;
    }
    m_writable = createIfMissing;

    if (created) {
        InterlockedExchange(&m_state->packedSettings, PackSettings(InputSettingsSnapshot{}));
        InterlockedExchange(&m_state->version, kSettingsVersion);
        InterlockedExchange(&m_state->magic, kSettingsMagic);
    }

    MemoryBarrier();
    if (!IsInitialized()) {
        Close();
        return false;
    }
    return true;
}

void SharedInputSettings::Close()
{
    if (m_state) {
        UnmapViewOfFile(m_state);
        m_state = nullptr;
    }
    if (m_mapping) {
        CloseHandle(m_mapping);
        m_mapping = nullptr;
    }
    m_writable = false;
}

bool SharedInputSettings::IsInitialized() const
{
    if (!m_state) {
        return false;
    }

    MemoryBarrier();
    LONG magic = m_state->magic;
    LONG version = m_state->version;
    MemoryBarrier();
    return magic == kSettingsMagic && version == kSettingsVersion;
}

InputSettingsSnapshot SharedInputSettings::Read() const
{
    InputSettingsSnapshot snapshot;
    if (!IsInitialized()) {
        return snapshot;
    }

    MemoryBarrier();
    LONG magic = m_state->magic;
    LONG version = m_state->version;
    LONG packedSettings = m_state->packedSettings;
    MemoryBarrier();
    LONG inputMode = packedSettings & 0x3;
    LONG charWidth = (packedSettings >> 2) & 0x1;
    LONG keyboardLayout = (packedSettings >> 3) & 0x1;
    LONG punctuationStyle = (packedSettings >> 4) & 0x1;
    LONG englishCase = (packedSettings >> 5) & 0x1;
    MemoryBarrier();
    if (magic != kSettingsMagic || version != kSettingsVersion) {
        return snapshot;
    }
    if (IsInputModeValue(inputMode)) {
        snapshot.inputMode = static_cast<SharedInputMode>(inputMode);
    }
    if (IsCharWidthValue(charWidth)) {
        snapshot.charWidth = static_cast<SharedCharWidth>(charWidth);
    }
    if (IsKeyboardLayoutValue(keyboardLayout)) {
        snapshot.keyboardLayout = static_cast<SharedKeyboardLayout>(keyboardLayout);
    }
    if (IsPunctuationStyleValue(punctuationStyle)) {
        snapshot.punctuationStyle = static_cast<SharedPunctuationStyle>(punctuationStyle);
    }
    if (IsEnglishCaseValue(englishCase)) {
        snapshot.englishCase = static_cast<SharedEnglishCase>(englishCase);
    }
    return snapshot;
}

bool SharedInputSettings::Write(const InputSettingsSnapshot& snapshot)
{
    if (!m_writable || !IsInitialized()) {
        return false;
    }
    InterlockedExchange(&m_state->packedSettings, PackSettings(snapshot));
    return true;
}

} // namespace KeyroIME
