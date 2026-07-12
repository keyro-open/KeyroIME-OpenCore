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
#include <windows.h>

#include <iostream>
#include <string>

#include "ui/system_tray.h"
#include "tsf_core/keyboard_layout.h"
#include "tsf_core/shared_settings.h"

namespace {

bool MenuTextEquals(HMENU menu, int position, const wchar_t* expected)
{
    wchar_t text[128] = {};
    if (GetMenuStringW(menu, position, text, ARRAYSIZE(text), MF_BYPOSITION) == 0) {
        return false;
    }
    return std::wstring(text) == expected;
}

bool IsCheckedByPosition(HMENU menu, int position)
{
    UINT state = GetMenuState(menu, position, MF_BYPOSITION);
    return state != static_cast<UINT>(-1) && (state & MF_CHECKED) != 0;
}

bool HasMenuBitmapByPosition(HMENU menu, int position)
{
    MENUITEMINFOW info = {};
    info.cbSize = sizeof(info);
    info.fMask = MIIM_CHECKMARKS | MIIM_BITMAP;
    if (!GetMenuItemInfoW(menu, static_cast<UINT>(position), TRUE, &info)) {
        return false;
    }
    return info.hbmpItem != nullptr ||
        info.hbmpChecked != nullptr ||
        info.hbmpUnchecked != nullptr;
}

void PumpMessagesFor(DWORD durationMs)
{
    ULONGLONG deadline = GetTickCount64() + durationMs;
    while (GetTickCount64() < deadline) {
        MSG message = {};
        while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
        Sleep(10);
    }
}

bool DialogWindowContract(KeyroIME::SystemTray& tray)
{
    constexpr int kLicenseEditId = 2001;
    constexpr int kAboutOfficialLinkId = 2104;
    constexpr int kAboutSupportLinkId = 2105;
    constexpr int kAboutLicenseLinkId = 2106;

    tray.ShowAboutDialogForTest();
    PumpMessagesFor(100);
    HWND about = FindWindowW(L"KeyroIME_AboutWindow", nullptr);
    bool aboutOk = about &&
        GetDlgItem(about, kAboutOfficialLinkId) &&
        GetDlgItem(about, kAboutSupportLinkId) &&
        GetDlgItem(about, kAboutLicenseLinkId);
    if (about) {
        DestroyWindow(about);
        PumpMessagesFor(50);
    }

    tray.ShowLicenseWindowForTest();
    PumpMessagesFor(100);
    HWND license = FindWindowW(L"KeyroIME_LicenseWindow", nullptr);
    HWND edit = license ? GetDlgItem(license, kLicenseEditId) : nullptr;
    LONG_PTR style = edit ? GetWindowLongPtrW(edit, GWL_STYLE) : 0;
    bool licenseOk = edit &&
        (style & ES_MULTILINE) &&
        (style & ES_READONLY) &&
        (style & WS_VSCROLL) &&
        !(style & WS_HSCROLL) &&
        !(style & ES_AUTOHSCROLL);
    if (license) {
        DestroyWindow(license);
        PumpMessagesFor(50);
    }

    return aboutOk && licenseOk;
}

bool ExpectMappedKey(
    KeyroIME::KeyboardLayoutMapper& mapper,
    WPARAM vkey,
    UINT scanCode,
    bool shifted,
    wchar_t expected)
{
    wchar_t actual = 0;
    if (!mapper.TranslatePrintableKey(vkey, shifted, actual, scanCode) || actual != expected) {
        std::wcerr << L"keyboard map mismatch scan=0x" << std::hex << scanCode
                   << L" shifted=" << shifted
                   << L" expected=" << expected
                   << L" actual=" << actual << std::endl;
        return false;
    }
    return true;
}

bool ExpectJapaneseSymbol(wchar_t source, const wchar_t* expected)
{
    std::wstring actual = KeyroIME::JapaneseStyleSymbolFor(source);
    if (actual != expected) {
        std::wcerr << L"Japanese symbol mismatch source=" << source
                   << L" expected=" << expected
                   << L" actual=" << actual << std::endl;
        return false;
    }
    return true;
}

} // namespace

int main()
{
    HRESULT comResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    bool uninitializeCom = comResult == S_OK || comResult == S_FALSE;

    KeyroIME::SystemTray tray;
    if (!tray.Initialize(GetModuleHandleW(nullptr), nullptr)) {
        std::cerr << "tray initialization failed\n";
        return 1;
    }
    tray.SetActiveTipForTest(true);
    bool trayVisibilityPassed = tray.IsVisibleForTest();
    tray.SetActiveTipForTest(false);
    trayVisibilityPassed = trayVisibilityPassed && tray.IsVisibleForTest();
    tray.SetActiveTipForTest(true);
    trayVisibilityPassed = trayVisibilityPassed && tray.IsVisibleForTest();
    if (!trayVisibilityPassed) {
        std::cerr << "persistent tray icon visibility contract failed\n";
        return 1;
    }
    bool queriedActiveTip = false;
    bool activeTip = false;
    queriedActiveTip = tray.QueryActiveTipForTest(activeTip);
    if (!queriedActiveTip) {
        std::cerr << "TSF active profile query failed\n";
        return 1;
    }
    KeyroIME::SharedInputSettings reader;
    if (!reader.Initialize(false)) {
        std::cerr << "shared settings open failed\n";
        return 1;
    }
    KeyroIME::InputSettingsSnapshot originalSettings = reader.Read();
    tray.SetInputMode(KeyroIME::InputMode::Katakana);
    tray.SetCharWidth(KeyroIME::CharWidth::FullWidth);
    tray.SetKeyboardLayout(KeyroIME::TrayKeyboardLayout::Ansi);
    tray.SetPunctuationStyle(KeyroIME::PunctuationStyle::Western);
    tray.SetEnglishCase(KeyroIME::EnglishCase::Upper);

    KeyroIME::InputSettingsSnapshot snapshot = reader.Read();
    if (snapshot.inputMode != KeyroIME::SharedInputMode::Katakana ||
        snapshot.charWidth != KeyroIME::SharedCharWidth::FullWidth ||
        snapshot.keyboardLayout != KeyroIME::SharedKeyboardLayout::Ansi ||
        snapshot.punctuationStyle != KeyroIME::SharedPunctuationStyle::Western ||
        snapshot.englishCase != KeyroIME::SharedEnglishCase::Upper) {
        std::cerr << "shared settings contract failed\n";
        return 1;
    }

    KeyroIME::SharedInputSettings shortcutWriter;
    if (!shortcutWriter.Initialize(true)) {
        std::cerr << "shortcut writer initialization failed\n";
        return 1;
    }
    KeyroIME::InputSettingsSnapshot shortcutSnapshot = snapshot;
    shortcutSnapshot.keyboardLayout = KeyroIME::SharedKeyboardLayout::Jis;
    shortcutSnapshot.punctuationStyle = KeyroIME::SharedPunctuationStyle::Japanese;
    tray.SetActiveTipForTest(false);
    if (!shortcutWriter.Write(shortcutSnapshot)) {
        std::cerr << "shortcut settings write failed\n";
        return 1;
    }
    PumpMessagesFor(200);
    bool shortcutSyncPassed =
        tray.GetKeyboardLayout() == KeyroIME::TrayKeyboardLayout::Jis &&
        tray.GetPunctuationStyle() == KeyroIME::PunctuationStyle::Japanese &&
        tray.IsStatusOsdVisibleForTest() &&
        tray.StatusOsdTextForTest().find(L"JIS") != std::wstring::npos &&
        tray.StatusOsdTextForTest().find(L"、。・") != std::wstring::npos;
    if (!shortcutSyncPassed) {
        std::cerr << "shortcut-to-tray OSD synchronization failed\n";
        return 1;
    }
    if (!shortcutWriter.Write(snapshot)) {
        std::cerr << "shortcut settings restore failed\n";
        return 1;
    }
    PumpMessagesFor(200);

    KeyroIME::InputSettingsSnapshot capsSnapshot;
    capsSnapshot.inputMode = KeyroIME::SharedInputMode::Hiragana;
    capsSnapshot.charWidth = KeyroIME::SharedCharWidth::HalfWidth;
    capsSnapshot.englishCase = KeyroIME::SharedEnglishCase::Lower;
    capsSnapshot = KeyroIME::ToggleCapsLockSetting(capsSnapshot);
    bool capsCasePassed =
        capsSnapshot.charWidth == KeyroIME::SharedCharWidth::HalfWidth &&
        capsSnapshot.englishCase == KeyroIME::SharedEnglishCase::Upper;
    capsSnapshot = KeyroIME::ToggleCapsLockSetting(capsSnapshot);
    capsCasePassed = capsCasePassed &&
        capsSnapshot.charWidth == KeyroIME::SharedCharWidth::HalfWidth &&
        capsSnapshot.englishCase == KeyroIME::SharedEnglishCase::Lower;

    KeyroIME::InputSettingsSnapshot widthSnapshot;
    widthSnapshot.charWidth = KeyroIME::SharedCharWidth::HalfWidth;
    widthSnapshot.englishCase = KeyroIME::SharedEnglishCase::Lower;
    widthSnapshot = KeyroIME::ToggleCharacterWidthSetting(widthSnapshot);
    bool widthTogglePassed =
        widthSnapshot.charWidth == KeyroIME::SharedCharWidth::FullWidth &&
        widthSnapshot.englishCase == KeyroIME::SharedEnglishCase::Lower;
    widthSnapshot = KeyroIME::ToggleCharacterWidthSetting(widthSnapshot);
    widthTogglePassed = widthTogglePassed &&
        widthSnapshot.charWidth == KeyroIME::SharedCharWidth::HalfWidth;
    if (!capsCasePassed || !widthTogglePassed) {
        std::cerr << "CapsLock settings transition failed\n";
        return 1;
    }

    KeyroIME::InputSettingsSnapshot layoutSnapshot;
    layoutSnapshot.keyboardLayout = KeyroIME::SharedKeyboardLayout::Jis;
    layoutSnapshot = KeyroIME::ToggleKeyboardLayoutSetting(layoutSnapshot);
    LPARAM semicolonScanCode = static_cast<LPARAM>(0x27) << 16;
    LPARAM semicolonWithAltContext = semicolonScanCode | (static_cast<LPARAM>(1) << 29);
    bool layoutShortcutPassed =
        layoutSnapshot.keyboardLayout == KeyroIME::SharedKeyboardLayout::Ansi &&
        KeyroIME::IsKeyboardLayoutToggleShortcut(VK_OEM_1, semicolonScanCode, true) &&
        KeyroIME::IsKeyboardLayoutToggleShortcut(VK_OEM_PLUS, semicolonScanCode, true) &&
        KeyroIME::IsKeyboardLayoutToggleShortcut(VK_OEM_1, semicolonWithAltContext, true) &&
        !KeyroIME::IsKeyboardLayoutToggleShortcut(L'K', 0, true) &&
        !KeyroIME::IsKeyboardLayoutToggleShortcut(VK_OEM_1, semicolonScanCode, false);
    if (!layoutShortcutPassed) {
        std::cerr << "Alt+semicolon layout shortcut contract failed\n";
        return 1;
    }

    KeyroIME::StatusOsd osd;
    if (!osd.Initialize(GetModuleHandleW(nullptr))) {
        std::cerr << "status OSD initialization failed\n";
        return 1;
    }
    osd.ShowStatus(L"ひらがな  |  半角  |  JIS  |  、。・");
    bool osdPassed = osd.IsVisible() && osd.CurrentText().find(L"ひらがな") != std::wstring::npos;
    osd.ShowStatus(L"英語・大文字  |  全角  |  ANSI (US)  |  ,./");
    osdPassed = osdPassed && osd.IsVisible() &&
        osd.CurrentText().find(L"英語・大文字") != std::wstring::npos;
    PumpMessagesFor(1600);
    osdPassed = osdPassed && !osd.IsVisible();
    if (!osdPassed) {
        std::cerr << "status OSD refresh/fade contract failed\n";
        return 1;
    }

    KeyroIME::KeyboardLayoutMapper mapper;
    wchar_t character = 0;
    mapper.SetLayout(KeyroIME::LAYOUT_JIS);
    bool jisMapped = mapper.TranslatePrintableKey(L'2', true, character) && character == L'\"';
    mapper.SetLayout(KeyroIME::LAYOUT_US);
    bool ansiMapped = mapper.TranslatePrintableKey(L'2', true, character) && character == L'@';
    bool commaMapped = mapper.TranslatePrintableKey(VK_OEM_COMMA, false, character) && character == L',';
    bool shiftedCommaMapped = mapper.TranslatePrintableKey(VK_OEM_COMMA, true, character) && character == L'<';
    bool ansiPositionMap =
        mapper.TranslatePrintableKey(L'6', true, character, 0x07) && character == L'^' &&
        mapper.TranslatePrintableKey(L'7', true, character, 0x08) && character == L'&' &&
        mapper.TranslatePrintableKey(L'8', true, character, 0x09) && character == L'*' &&
        mapper.TranslatePrintableKey(L'9', true, character, 0x0A) && character == L'(' &&
        mapper.TranslatePrintableKey(L'0', true, character, 0x0B) && character == L')' &&
        mapper.TranslatePrintableKey(VK_OEM_MINUS, false, character, 0x0C) && character == L'-' &&
        mapper.TranslatePrintableKey(VK_OEM_PLUS, false, character, 0x0D) && character == L'=' &&
        mapper.TranslatePrintableKey(VK_OEM_4, false, character, 0x1A) && character == L'[' &&
        mapper.TranslatePrintableKey(VK_OEM_6, false, character, 0x1B) && character == L']' &&
        mapper.TranslatePrintableKey(VK_OEM_1, false, character, 0x27) && character == L';' &&
        mapper.TranslatePrintableKey(VK_OEM_7, false, character, 0x28) && character == L'\'' &&
        mapper.TranslatePrintableKey(VK_OEM_5, false, character, 0x2B) && character == L'\\' &&
        mapper.TranslatePrintableKey(VK_OEM_2, false, character, 0x35) && character == L'/';
    mapper.SetLayout(KeyroIME::LAYOUT_JIS);
    bool jisPositionMap =
        ExpectMappedKey(mapper, L'1', 0x02, false, L'1') &&
        ExpectMappedKey(mapper, L'1', 0x02, true, L'!') &&
        ExpectMappedKey(mapper, L'2', 0x03, true, L'\"') &&
        ExpectMappedKey(mapper, L'3', 0x04, true, L'#') &&
        ExpectMappedKey(mapper, L'4', 0x05, true, L'$') &&
        ExpectMappedKey(mapper, L'5', 0x06, true, L'%') &&
        mapper.TranslatePrintableKey(L'6', true, character, 0x07) && character == L'&' &&
        ExpectMappedKey(mapper, L'7', 0x08, true, L'\'') &&
        ExpectMappedKey(mapper, L'8', 0x09, true, L'(') &&
        ExpectMappedKey(mapper, L'9', 0x0A, true, L')') &&
        ExpectMappedKey(mapper, L'0', 0x0B, true, L'0') &&
        ExpectMappedKey(mapper, VK_OEM_MINUS, 0x0C, false, L'-') &&
        ExpectMappedKey(mapper, VK_OEM_MINUS, 0x0C, true, L'=') &&
        mapper.TranslatePrintableKey(VK_OEM_7, false, character, 0x0D) && character == L'^' &&
        ExpectMappedKey(mapper, VK_OEM_7, 0x0D, true, L'~') &&
        mapper.TranslatePrintableKey(VK_OEM_3, false, character, 0x1A) && character == L'@' &&
        ExpectMappedKey(mapper, VK_OEM_3, 0x1A, true, L'`') &&
        ExpectMappedKey(mapper, VK_OEM_4, 0x1B, false, L'[') &&
        ExpectMappedKey(mapper, VK_OEM_4, 0x1B, true, L'{') &&
        ExpectMappedKey(mapper, VK_OEM_PLUS, 0x27, false, L';') &&
        ExpectMappedKey(mapper, VK_OEM_PLUS, 0x27, true, L'+') &&
        mapper.TranslatePrintableKey(VK_OEM_1, false, character, 0x28) && character == L':' &&
        ExpectMappedKey(mapper, VK_OEM_1, 0x28, true, L'*') &&
        ExpectMappedKey(mapper, VK_OEM_6, 0x2B, false, L']') &&
        ExpectMappedKey(mapper, VK_OEM_6, 0x2B, true, L'}') &&
        ExpectMappedKey(mapper, VK_OEM_COMMA, 0x33, false, L',') &&
        ExpectMappedKey(mapper, VK_OEM_COMMA, 0x33, true, L'<') &&
        ExpectMappedKey(mapper, VK_OEM_PERIOD, 0x34, false, L'.') &&
        ExpectMappedKey(mapper, VK_OEM_PERIOD, 0x34, true, L'>') &&
        ExpectMappedKey(mapper, VK_OEM_2, 0x35, false, L'/') &&
        ExpectMappedKey(mapper, VK_OEM_2, 0x35, true, L'?') &&
        ExpectMappedKey(mapper, VK_OEM_102, 0x73, false, L'\\') &&
        ExpectMappedKey(mapper, VK_OEM_102, 0x73, true, L'_') &&
        ExpectMappedKey(mapper, VK_OEM_5, 0x7D, false, L'\\') &&
        ExpectMappedKey(mapper, VK_OEM_5, 0x7D, true, L'|');
    mapper.SetLayout(KeyroIME::LAYOUT_US);
    bool ansiFullPositionMap =
        ExpectMappedKey(mapper, L'1', 0x02, false, L'1') &&
        ExpectMappedKey(mapper, L'1', 0x02, true, L'!') &&
        ExpectMappedKey(mapper, L'2', 0x03, true, L'@') &&
        ExpectMappedKey(mapper, L'3', 0x04, true, L'#') &&
        ExpectMappedKey(mapper, L'4', 0x05, true, L'$') &&
        ExpectMappedKey(mapper, L'5', 0x06, true, L'%') &&
        ExpectMappedKey(mapper, L'6', 0x07, true, L'^') &&
        ExpectMappedKey(mapper, L'7', 0x08, true, L'&') &&
        ExpectMappedKey(mapper, L'8', 0x09, true, L'*') &&
        ExpectMappedKey(mapper, L'9', 0x0A, true, L'(') &&
        ExpectMappedKey(mapper, L'0', 0x0B, true, L')') &&
        ExpectMappedKey(mapper, VK_OEM_MINUS, 0x0C, false, L'-') &&
        ExpectMappedKey(mapper, VK_OEM_MINUS, 0x0C, true, L'_') &&
        ExpectMappedKey(mapper, VK_OEM_PLUS, 0x0D, false, L'=') &&
        ExpectMappedKey(mapper, VK_OEM_PLUS, 0x0D, true, L'+') &&
        ExpectMappedKey(mapper, VK_OEM_4, 0x1A, false, L'[') &&
        ExpectMappedKey(mapper, VK_OEM_4, 0x1A, true, L'{') &&
        ExpectMappedKey(mapper, VK_OEM_6, 0x1B, false, L']') &&
        ExpectMappedKey(mapper, VK_OEM_6, 0x1B, true, L'}') &&
        ExpectMappedKey(mapper, VK_OEM_1, 0x27, false, L';') &&
        ExpectMappedKey(mapper, VK_OEM_1, 0x27, true, L':') &&
        ExpectMappedKey(mapper, VK_OEM_7, 0x28, false, L'\'') &&
        ExpectMappedKey(mapper, VK_OEM_7, 0x28, true, L'\"') &&
        ExpectMappedKey(mapper, VK_OEM_3, 0x29, false, L'`') &&
        ExpectMappedKey(mapper, VK_OEM_3, 0x29, true, L'~') &&
        ExpectMappedKey(mapper, VK_OEM_5, 0x2B, false, L'\\') &&
        ExpectMappedKey(mapper, VK_OEM_5, 0x2B, true, L'|') &&
        ExpectMappedKey(mapper, VK_OEM_COMMA, 0x33, false, L',') &&
        ExpectMappedKey(mapper, VK_OEM_COMMA, 0x33, true, L'<') &&
        ExpectMappedKey(mapper, VK_OEM_PERIOD, 0x34, false, L'.') &&
        ExpectMappedKey(mapper, VK_OEM_PERIOD, 0x34, true, L'>') &&
        ExpectMappedKey(mapper, VK_OEM_2, 0x35, false, L'/') &&
        ExpectMappedKey(mapper, VK_OEM_2, 0x35, true, L'?');
    bool japaneseSymbolMap =
        ExpectJapaneseSymbol(L'\\', L"￥") &&
        ExpectJapaneseSymbol(L'!', L"！") &&
        ExpectJapaneseSymbol(L'"', L"”") &&
        ExpectJapaneseSymbol(L'#', L"＃") &&
        ExpectJapaneseSymbol(L'$', L"＄") &&
        ExpectJapaneseSymbol(L'%', L"％") &&
        ExpectJapaneseSymbol(L'&', L"＆") &&
        ExpectJapaneseSymbol(L'\'', L"’") &&
        ExpectJapaneseSymbol(L'(', L"（") &&
        ExpectJapaneseSymbol(L')', L"）") &&
        ExpectJapaneseSymbol(L'[', L"「") &&
        ExpectJapaneseSymbol(L']', L"」") &&
        ExpectJapaneseSymbol(L'{', L"｛") &&
        ExpectJapaneseSymbol(L'}', L"｝") &&
        ExpectJapaneseSymbol(L':', L"：") &&
        ExpectJapaneseSymbol(L';', L"；") &&
        ExpectJapaneseSymbol(L'~', L"〜") &&
        ExpectJapaneseSymbol(L'^', L"＾") &&
        ExpectJapaneseSymbol(L'-', L"ー") &&
        ExpectJapaneseSymbol(L'/', L"・") &&
        ExpectJapaneseSymbol(L',', L"、") &&
        ExpectJapaneseSymbol(L'.', L"。");
    KeyroIME::InputSettingsSnapshot imeKeySnapshot;
    imeKeySnapshot.inputMode = KeyroIME::SharedInputMode::Hiragana;
    imeKeySnapshot.charWidth = KeyroIME::SharedCharWidth::HalfWidth;
    imeKeySnapshot = KeyroIME::CycleKanaWidthSetting(imeKeySnapshot);
    bool jisFunctionKeyContract =
        KeyroIME::IsImeModeToggleKey(VK_KANJI) &&
        KeyroIME::IsImeModeToggleKey(VK_IME_ON) &&
        KeyroIME::IsImeModeToggleKey(VK_IME_OFF) &&
        KeyroIME::IsKanaModeKey(VK_KANA) &&
        KeyroIME::IsConvertKey(VK_CONVERT) &&
        KeyroIME::IsNonConvertKey(VK_NONCONVERT) &&
        imeKeySnapshot.inputMode == KeyroIME::SharedInputMode::Katakana &&
        imeKeySnapshot.charWidth == KeyroIME::SharedCharWidth::FullWidth;
    if (!jisMapped || !ansiMapped || !commaMapped || !shiftedCommaMapped ||
        !ansiPositionMap || !jisPositionMap || !ansiFullPositionMap ||
        !japaneseSymbolMap || !jisFunctionKeyContract) {
        std::cerr << "keyboard layout mapping failed\n";
        return 1;
    }

    HMENU menu = tray.CreateContextMenu();
    if (!menu) {
        std::cerr << "menu creation failed\n";
        return 1;
    }

    bool ok = GetMenuItemCount(menu) == 6 &&
        MenuTextEquals(menu, 0, L"入力モード設定") &&
        MenuTextEquals(menu, 1, L"文字幅設定") &&
        MenuTextEquals(menu, 2, L"キーボード配列設定") &&
        MenuTextEquals(menu, 3, L"句読点設定") &&
        MenuTextEquals(menu, 4, L"拡張機能（v1.0では利用できません）") &&
        MenuTextEquals(menu, 5, L"ヘルプと情報") &&
        HasMenuBitmapByPosition(menu, 0) &&
        !HasMenuBitmapByPosition(menu, 1) &&
        !HasMenuBitmapByPosition(menu, 2) &&
        !HasMenuBitmapByPosition(menu, 3);

    HMENU inputMode = GetSubMenu(menu, 0);
    HMENU charWidth = GetSubMenu(menu, 1);
    HMENU keyboardLayout = GetSubMenu(menu, 2);
    HMENU punctuation = GetSubMenu(menu, 3);
    HMENU help = GetSubMenu(menu, 5);
    ok = ok && inputMode && charWidth && keyboardLayout && punctuation && help &&
        IsCheckedByPosition(inputMode, 1) &&
        IsCheckedByPosition(charWidth, 1) &&
        IsCheckedByPosition(keyboardLayout, 1) &&
        IsCheckedByPosition(punctuation, 1) &&
        MenuTextEquals(inputMode, 1, L"カタカナ") &&
        MenuTextEquals(inputMode, 2, L"英語\t[Alt + ~]") &&
        MenuTextEquals(charWidth, 1, L"全角\t[Shift + Caps]") &&
        MenuTextEquals(keyboardLayout, 1, L"ANSI配列\t[Alt + ;]") &&
        MenuTextEquals(punctuation, 1, L", . / (欧文)\t[Shift]") &&
        GetMenuItemCount(help) == 2 &&
        MenuTextEquals(help, 0, L"アップデートを確認") &&
        MenuTextEquals(help, 1, L"KeyroIME について") &&
        !HasMenuBitmapByPosition(help, 0);

    UINT extensionState = GetMenuState(menu, 4, MF_BYPOSITION);
    ok = ok && extensionState != static_cast<UINT>(-1) &&
        (extensionState & (MF_DISABLED | MF_GRAYED)) != 0;

    DestroyMenu(menu);
    if (!ok) {
        std::cerr << "menu contract failed\n";
        return 1;
    }

    tray.SetUpdateBadgeForTest(true);
    HMENU badgeMenu = tray.CreateContextMenu();
    HMENU badgeHelp = badgeMenu ? GetSubMenu(badgeMenu, 5) : nullptr;
    bool badgeMenuPassed = badgeMenu && badgeHelp &&
        MenuTextEquals(badgeMenu, 5, L"ヘルプと情報") &&
        MenuTextEquals(badgeHelp, 0, L"アップデートを確認") &&
        HasMenuBitmapByPosition(badgeMenu, 5) &&
        HasMenuBitmapByPosition(badgeHelp, 0);
    if (badgeMenu) {
        DestroyMenu(badgeMenu);
    }
    tray.AcknowledgeUpdateReminderForTest();
    HMENU resetMenu = tray.CreateContextMenu();
    HMENU resetHelp = resetMenu ? GetSubMenu(resetMenu, 5) : nullptr;
    badgeMenuPassed = badgeMenuPassed && resetMenu && resetHelp &&
        MenuTextEquals(resetHelp, 0, L"アップデートを確認") &&
        !HasMenuBitmapByPosition(resetMenu, 5) &&
        !HasMenuBitmapByPosition(resetHelp, 0);
    if (resetMenu) {
        DestroyMenu(resetMenu);
    }
    if (!badgeMenuPassed) {
        std::cerr << "update badge menu contract failed\n";
        return 1;
    }

    if (!DialogWindowContract(tray)) {
        std::cerr << "about/license dialog contract failed\n";
        return 1;
    }

    tray.SetKeyboardLayout(KeyroIME::TrayKeyboardLayout::Jis);
    PumpMessagesFor(200);
    HMENU jisMenu = tray.CreateContextMenu();
    HMENU jisInputMode = jisMenu ? GetSubMenu(jisMenu, 0) : nullptr;
    HMENU jisCharWidth = jisMenu ? GetSubMenu(jisMenu, 1) : nullptr;
    bool jisMenuPassed = jisMenu && jisInputMode && jisCharWidth &&
        MenuTextEquals(jisInputMode, 0, L"ひらがな") &&
        MenuTextEquals(jisInputMode, 2, L"英語") &&
        MenuTextEquals(jisCharWidth, 0, L"半角") &&
        MenuTextEquals(jisCharWidth, 1, L"全角") &&
        HasMenuBitmapByPosition(jisMenu, 0);
    if (jisMenu) {
        DestroyMenu(jisMenu);
    }
    if (!jisMenuPassed) {
        std::cerr << "JIS menu shortcut visibility contract failed\n";
        return 1;
    }

    if (!shortcutWriter.Write(originalSettings)) {
        std::cerr << "original settings restore failed\n";
        return 1;
    }
    PumpMessagesFor(200);

    std::cout << "tray menu contract passed\n";
    if (uninitializeCom) {
        CoUninitialize();
    }
    return 0;
}
