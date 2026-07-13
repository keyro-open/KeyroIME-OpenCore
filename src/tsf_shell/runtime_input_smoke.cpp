// Copyright (C) 2025-2026 株式会社LocalPro (LocalPro Co., Ltd.). All rights reserved.
// Brand Official Website: https://keyro.jp
//
// This file is part of KeyroIME (キーロ) OpenCore.
// It is source-available under the KeyroIME OpenCore Non-Commercial Source
// License 1.0. See LICENSE. Commercial use requires a separate written license
// from 株式会社LocalPro.
#include <msctf.h>
#include <windows.h>

#include <iostream>
#include <string>

#include "tsf_core/tip_guid.h"
#include "tsf_core/shared_settings.h"

namespace {

const GUID kPreservedKanjiKey = {
    0x4e4dd778, 0x969f, 0x41a8, {0x8f, 0xa3, 0x6f, 0xc3, 0x19, 0x67, 0x3c, 0xa1}
};
const GUID kPreservedKanaKey = {
    0xd2d91ae8, 0xc972, 0x468f, {0x87, 0x6e, 0xae, 0x93, 0x64, 0x9c, 0xac, 0x02}
};
const GUID kPreservedConvertKey = {
    0xed6f12de, 0x7d5e, 0x4798, {0xa0, 0x9d, 0x2d, 0x23, 0x54, 0xb2, 0x6f, 0xf0}
};
const GUID kPreservedNonConvertKey = {
    0x654e3c3f, 0x17c2, 0x46f0, {0x9b, 0x6a, 0xa3, 0xf7, 0x3e, 0xf5, 0x80, 0x6b}
};

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

bool SendInputs(INPUT* inputs, UINT count)
{
    return SendInput(count, inputs, sizeof(INPUT)) == count;
}

bool SendTrayCommand(HWND tray, UINT commandId)
{
    DWORD_PTR result = 0;
    return SendMessageTimeoutW(
        tray,
        WM_COMMAND,
        MAKEWPARAM(commandId, 0),
        0,
        SMTO_ABORTIFHUNG,
        2000,
        &result) != 0;
}

bool SendShiftTwo()
{
    INPUT inputs[4] = {};
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_SHIFT;
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = L'2';
    inputs[2] = inputs[1];
    inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;
    inputs[3] = inputs[0];
    inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;
    return SendInputs(inputs, ARRAYSIZE(inputs));
}

bool SendShiftOne()
{
    INPUT inputs[4] = {};
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_SHIFT;
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = L'1';
    inputs[2] = inputs[1];
    inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;
    inputs[3] = inputs[0];
    inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;
    return SendInputs(inputs, ARRAYSIZE(inputs));
}

bool SendVirtualKey(WORD vkey)
{
    INPUT inputs[2] = {};
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = vkey;
    inputs[1] = inputs[0];
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
    return SendInputs(inputs, ARRAYSIZE(inputs));
}

bool SendAltSemicolon()
{
    INPUT inputs[4] = {};
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_MENU;
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wScan = 0x27;
    inputs[1].ki.dwFlags = KEYEVENTF_SCANCODE;
    inputs[2] = inputs[1];
    inputs[2].ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
    inputs[3] = inputs[0];
    inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;
    return SendInputs(inputs, ARRAYSIZE(inputs));
}

bool SendLetters(const wchar_t* letters)
{
    for (const wchar_t* cursor = letters; cursor && *cursor; ++cursor) {
        wchar_t ch = *cursor;
        if (ch >= L'a' && ch <= L'z') {
            ch = static_cast<wchar_t>(ch - L'a' + L'A');
        }
        if (ch < L'A' || ch > L'Z' || !SendVirtualKey(static_cast<WORD>(ch))) {
            return false;
        }
        PumpMessagesFor(30);
    }
    return true;
}

std::wstring WindowText(HWND window)
{
    int length = GetWindowTextLengthW(window);
    std::wstring text(static_cast<size_t>(length) + 1, L'\0');
    if (length > 0) {
        int copied = GetWindowTextW(window, text.data(), length + 1);
        text.resize(static_cast<size_t>(copied));
    } else {
        text.clear();
    }
    return text;
}

bool FocusEditWindow(HWND window, HWND edit)
{
    DWORD currentThread = GetCurrentThreadId();
    HWND foreground = GetForegroundWindow();
    DWORD foregroundThread = foreground
        ? GetWindowThreadProcessId(foreground, nullptr)
        : 0;
    bool attached = foregroundThread != 0 && foregroundThread != currentThread &&
        AttachThreadInput(currentThread, foregroundThread, TRUE) != FALSE;
    ShowWindow(window, SW_RESTORE);
    BringWindowToTop(window);
    SetForegroundWindow(window);
    SetActiveWindow(window);
    SetFocus(edit);
    if (attached) {
        AttachThreadInput(currentThread, foregroundThread, FALSE);
    }
    PumpMessagesFor(100);
    return GetForegroundWindow() == window && GetFocus() == edit;
}

bool WaitForWindowText(HWND window, const wchar_t* expected, DWORD timeoutMs)
{
    ULONGLONG deadline = GetTickCount64() + timeoutMs;
    do {
        if (WindowText(window) == expected) {
            return true;
        }
        PumpMessagesFor(25);
    } while (GetTickCount64() < deadline);
    return WindowText(window) == expected;
}

bool WaitForSettings(
    KeyroIME::SharedInputSettings& reader,
    KeyroIME::SharedInputMode inputMode,
    KeyroIME::SharedCharWidth charWidth,
    KeyroIME::SharedKeyboardLayout keyboardLayout,
    KeyroIME::InputSettingsSnapshot& snapshot,
    DWORD timeoutMs)
{
    ULONGLONG deadline = GetTickCount64() + timeoutMs;
    do {
        snapshot = reader.Read();
        if (snapshot.inputMode == inputMode &&
            snapshot.charWidth == charWidth &&
            snapshot.keyboardLayout == keyboardLayout) {
            return true;
        }
        PumpMessagesFor(25);
    } while (GetTickCount64() < deadline);
    return false;
}

bool WaitForSettingsWithPunctuation(
    KeyroIME::SharedInputSettings& reader,
    KeyroIME::SharedInputMode inputMode,
    KeyroIME::SharedCharWidth charWidth,
    KeyroIME::SharedKeyboardLayout keyboardLayout,
    KeyroIME::SharedPunctuationStyle punctuationStyle,
    KeyroIME::InputSettingsSnapshot& snapshot,
    DWORD timeoutMs)
{
    ULONGLONG deadline = GetTickCount64() + timeoutMs;
    do {
        snapshot = reader.Read();
        if (snapshot.inputMode == inputMode &&
            snapshot.charWidth == charWidth &&
            snapshot.keyboardLayout == keyboardLayout &&
            snapshot.punctuationStyle == punctuationStyle) {
            return true;
        }
        PumpMessagesFor(25);
    } while (GetTickCount64() < deadline);
    return false;
}

bool WaitForOsdVisibility(HWND osd, bool visible, DWORD timeoutMs)
{
    ULONGLONG deadline = GetTickCount64() + timeoutMs;
    do {
        if ((IsWindowVisible(osd) != FALSE) == visible) {
            return true;
        }
        PumpMessagesFor(25);
    } while (GetTickCount64() < deadline);
    return (IsWindowVisible(osd) != FALSE) == visible;
}

bool WaitForCandidateWindowVisible(bool visible, DWORD timeoutMs)
{
    ULONGLONG deadline = GetTickCount64() + timeoutMs;
    do {
        HWND candidate = FindWindowW(L"KeyroIME_CandidateWindow", nullptr);
        if (((candidate && IsWindowVisible(candidate)) != FALSE) == visible) {
            return true;
        }
        PumpMessagesFor(25);
    } while (GetTickCount64() < deadline);
    HWND candidate = FindWindowW(L"KeyroIME_CandidateWindow", nullptr);
    return ((candidate && IsWindowVisible(candidate)) != FALSE) == visible;
}

ITfContext* FocusedContext(ITfThreadMgr* threadManager)
{
    if (!threadManager) {
        return nullptr;
    }

    ITfDocumentMgr* documentManager = nullptr;
    if (FAILED(threadManager->GetFocus(&documentManager)) || !documentManager) {
        return nullptr;
    }

    ITfContext* context = nullptr;
    documentManager->GetTop(&context);
    documentManager->Release();
    return context;
}

bool SimulatePreservedKey(ITfThreadMgr* threadManager, ITfContext* context, REFGUID guid)
{
    if (!threadManager || !context) {
        return false;
    }

    ITfKeystrokeMgr* keystrokeManager = nullptr;
    HRESULT hr = threadManager->QueryInterface(
        IID_ITfKeystrokeMgr,
        reinterpret_cast<void**>(&keystrokeManager));
    if (FAILED(hr) || !keystrokeManager) {
        return false;
    }

    BOOL eaten = FALSE;
    hr = keystrokeManager->SimulatePreservedKey(context, guid, &eaten);
    keystrokeManager->Release();
    PumpMessagesFor(100);
    return SUCCEEDED(hr) && eaten;
}

} // namespace

int wmain()
{
    HRESULT comResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    bool uninitializeCom = comResult == S_OK || comResult == S_FALSE;
    if (FAILED(comResult) && comResult != RPC_E_CHANGED_MODE) {
        return 1;
    }

    ITfThreadMgr* threadManager = nullptr;
    TfClientId clientId = TF_CLIENTID_NULL;
    HRESULT result = CoCreateInstance(
        CLSID_TF_ThreadMgr,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_ITfThreadMgr,
        reinterpret_cast<void**>(&threadManager));
    if (FAILED(result) || !threadManager || FAILED(threadManager->Activate(&clientId))) {
        if (threadManager) threadManager->Release();
        if (uninitializeCom) CoUninitialize();
        return 2;
    }

    ITfInputProcessorProfileMgr* profileManager = nullptr;
    result = CoCreateInstance(
        CLSID_TF_InputProcessorProfiles,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_ITfInputProcessorProfileMgr,
        reinterpret_cast<void**>(&profileManager));
    TF_INPUTPROCESSORPROFILE originalProfile = {};
    bool haveOriginalProfile = profileManager && SUCCEEDED(profileManager->GetActiveProfile(
        GUID_TFCAT_TIP_KEYBOARD,
        &originalProfile));
    HRESULT activateResult = profileManager ? profileManager->ActivateProfile(
            TF_PROFILETYPE_INPUTPROCESSOR,
            0x0411,
            KeyroIME::CLSID_KeyroTextService,
            KeyroIME::GUID_KeyroProfile,
            nullptr,
            TF_IPPMF_FORPROCESS | TF_IPPMF_DONTCARECURRENTINPUTLANGUAGE) : E_NOINTERFACE;
    if (FAILED(result) || !profileManager || FAILED(activateResult)) {
        std::wcerr << L"profile activation failed: 0x"
                   << std::hex << static_cast<unsigned long>(activateResult) << std::endl;
        if (profileManager) profileManager->Release();
        threadManager->Deactivate();
        threadManager->Release();
        if (uninitializeCom) CoUninitialize();
        return 3;
    }

    HWND window = CreateWindowExW(
        WS_EX_TOOLWINDOW,
        L"STATIC",
        L"KeyroIME Runtime Input Smoke",
        WS_OVERLAPPEDWINDOW,
        100,
        100,
        500,
        160,
        nullptr,
        nullptr,
        GetModuleHandleW(nullptr),
        nullptr);
    HWND edit = window ? CreateWindowExW(
        0,
        L"EDIT",
        L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        20,
        30,
        440,
        32,
        window,
        nullptr,
        GetModuleHandleW(nullptr),
        nullptr) : nullptr;
    if (!window || !edit) {
        if (window) DestroyWindow(window);
        profileManager->Release();
        threadManager->Deactivate();
        threadManager->Release();
        if (uninitializeCom) CoUninitialize();
        return 4;
    }

    ShowWindow(window, SW_SHOWNORMAL);
    SetForegroundWindow(window);
    SetFocus(edit);
    PumpMessagesFor(1200);

    HWND tray = FindWindowW(L"KeyroIME_Tray_MessageWindow", L"KeyroIME Tray");
    if (!tray) {
        DestroyWindow(window);
        profileManager->Release();
        threadManager->Deactivate();
        threadManager->Release();
        if (uninitializeCom) CoUninitialize();
        return 5;
    }

    constexpr UINT kMenuHiragana = 1001;
    constexpr UINT kMenuEnglish = 1003;
    constexpr UINT kMenuHalfWidth = 1004;
    constexpr UINT kMenuJis = 1006;
    constexpr UINT kMenuPunctuationJapanese = 1009;
    KeyroIME::SharedInputSettings settingsReader;
    bool settingsReadable = settingsReader.Initialize(false);
    KeyroIME::InputSettingsSnapshot originalSettings = settingsReader.Read();
    KeyroIME::InputSettingsSnapshot menuSettings;
    bool menuSelectionPassed =
        SendTrayCommand(tray, kMenuEnglish) &&
        SendTrayCommand(tray, kMenuHalfWidth) &&
        SendTrayCommand(tray, kMenuJis) &&
        settingsReadable &&
        WaitForSettings(
            settingsReader,
            KeyroIME::SharedInputMode::English,
            KeyroIME::SharedCharWidth::HalfWidth,
            KeyroIME::SharedKeyboardLayout::Jis,
            menuSettings,
            2000);
    bool focusReady = FocusEditWindow(window, edit);
    HWND osd = FindWindowW(L"KeyroIME_StatusOsd", nullptr);
    bool menuOsdVisible = osd && IsWindowVisible(osd);

    bool sent = focusReady && SendShiftTwo();
    bool jisTextReady = sent && WaitForWindowText(edit, L"\"", 2500);
    std::wstring afterJis = WindowText(edit);
    bool menuOsdFaded = osd && WaitForOsdVisibility(osd, false, 2500);

    sent = sent && FocusEditWindow(window, edit) && SendAltSemicolon();
    KeyroIME::InputSettingsSnapshot afterShortcutSettings;
    bool shortcutSettingsReady = sent && WaitForSettings(
        settingsReader,
        KeyroIME::SharedInputMode::English,
        KeyroIME::SharedCharWidth::HalfWidth,
        KeyroIME::SharedKeyboardLayout::Ansi,
        afterShortcutSettings,
        2500);
    bool shortcutOsdVisible = osd && WaitForOsdVisibility(osd, true, 750);
    sent = sent && FocusEditWindow(window, edit) && SendShiftTwo();
    bool ansiTextReady = sent && WaitForWindowText(edit, L"\"@", 2500);
    std::wstring afterAnsi = WindowText(edit);

    menuSelectionPassed = SendTrayCommand(tray, kMenuJis) && menuSelectionPassed;
    KeyroIME::InputSettingsSnapshot afterMenuSettings;
    bool menuRestoreReady = WaitForSettings(
        settingsReader,
        KeyroIME::SharedInputMode::English,
        KeyroIME::SharedCharWidth::HalfWidth,
        KeyroIME::SharedKeyboardLayout::Jis,
        afterMenuSettings,
        2000);
    sent = sent && FocusEditWindow(window, edit) && SendShiftTwo();
    bool restoredTextReady = sent && WaitForWindowText(edit, L"\"@\"", 2500);
    std::wstring afterMenuRestore = WindowText(edit);

    SetWindowTextW(edit, L"");
    menuSelectionPassed =
        SendTrayCommand(tray, kMenuHiragana) &&
        SendTrayCommand(tray, kMenuHalfWidth) &&
        SendTrayCommand(tray, kMenuJis) &&
        SendTrayCommand(tray, kMenuPunctuationJapanese) &&
        menuSelectionPassed;
    KeyroIME::InputSettingsSnapshot jisFunctionBaseSettings;
    bool jisFunctionBaseReady = WaitForSettingsWithPunctuation(
        settingsReader,
        KeyroIME::SharedInputMode::Hiragana,
        KeyroIME::SharedCharWidth::HalfWidth,
        KeyroIME::SharedKeyboardLayout::Jis,
        KeyroIME::SharedPunctuationStyle::Japanese,
        jisFunctionBaseSettings,
        2000);

    SetWindowTextW(edit, L"");
    sent = sent && FocusEditWindow(window, edit) && SendLetters(L"ka") && SendVirtualKey(VK_RETURN);
    bool hiraganaCompositionReady = sent && WaitForWindowText(edit, L"か", 2500);
    std::wstring afterHiraganaComposition = WindowText(edit);

    SetWindowTextW(edit, L"");
    menuSelectionPassed = SendTrayCommand(tray, kMenuHiragana) &&
        SendTrayCommand(tray, kMenuHalfWidth) && menuSelectionPassed;
    sent = sent && FocusEditWindow(window, edit) && SendLetters(L"ka");
    ITfContext* fullContext = FocusedContext(threadManager);
    bool nonConvertFullEaten = fullContext &&
        SimulatePreservedKey(threadManager, fullContext, kPreservedNonConvertKey);
    KeyroIME::InputSettingsSnapshot afterNonConvertFull;
    bool nonConvertFullReady = sent && nonConvertFullEaten && WaitForSettings(
        settingsReader,
        KeyroIME::SharedInputMode::Katakana,
        KeyroIME::SharedCharWidth::FullWidth,
        KeyroIME::SharedKeyboardLayout::Jis,
        afterNonConvertFull,
        2000);
    sent = sent && SendVirtualKey(VK_RETURN);
    bool fullKatakanaReady = sent && WaitForWindowText(edit, L"カ", 2500);
    std::wstring afterFullKatakana = WindowText(edit);
    if (fullContext) {
        fullContext->Release();
        fullContext = nullptr;
    }

    SetWindowTextW(edit, L"");
    menuSelectionPassed = SendTrayCommand(tray, kMenuHiragana) &&
        SendTrayCommand(tray, kMenuHalfWidth) && menuSelectionPassed;
    sent = sent && FocusEditWindow(window, edit) && SendLetters(L"ka");
    ITfContext* halfContext = FocusedContext(threadManager);
    bool nonConvertHalfEaten = halfContext &&
        SimulatePreservedKey(threadManager, halfContext, kPreservedNonConvertKey) &&
        SimulatePreservedKey(threadManager, halfContext, kPreservedNonConvertKey);
    KeyroIME::InputSettingsSnapshot afterNonConvertHalf;
    bool nonConvertHalfReady = sent && nonConvertHalfEaten && WaitForSettings(
        settingsReader,
        KeyroIME::SharedInputMode::Katakana,
        KeyroIME::SharedCharWidth::HalfWidth,
        KeyroIME::SharedKeyboardLayout::Jis,
        afterNonConvertHalf,
        2000);
    sent = sent && SendVirtualKey(VK_RETURN);
    bool halfKatakanaReady = sent && WaitForWindowText(edit, L"ｶ", 2500);
    std::wstring afterHalfKatakana = WindowText(edit);
    if (halfContext) {
        halfContext->Release();
        halfContext = nullptr;
    }

    SetWindowTextW(edit, L"");
    menuSelectionPassed = SendTrayCommand(tray, kMenuHiragana) &&
        SendTrayCommand(tray, kMenuHalfWidth) && menuSelectionPassed;
    sent = sent && FocusEditWindow(window, edit) && SendLetters(L"ka");
    ITfContext* kanaContext = FocusedContext(threadManager);
    bool kanaKeyEaten = kanaContext &&
        SimulatePreservedKey(threadManager, kanaContext, kPreservedNonConvertKey) &&
        SimulatePreservedKey(threadManager, kanaContext, kPreservedKanaKey);
    sent = sent && SendVirtualKey(VK_RETURN);
    bool kanaKeyReady = sent && kanaKeyEaten && WaitForWindowText(edit, L"か", 2500);
    std::wstring afterKanaKey = WindowText(edit);
    if (kanaContext) {
        kanaContext->Release();
        kanaContext = nullptr;
    }

    SetWindowTextW(edit, L"");
    menuSelectionPassed = SendTrayCommand(tray, kMenuHiragana) &&
        SendTrayCommand(tray, kMenuHalfWidth) && menuSelectionPassed;
    KeyroIME::InputSettingsSnapshot beforeConvertSettings;
    bool convertBaseReady = WaitForSettings(
        settingsReader,
        KeyroIME::SharedInputMode::Hiragana,
        KeyroIME::SharedCharWidth::HalfWidth,
        KeyroIME::SharedKeyboardLayout::Jis,
        beforeConvertSettings,
        2000);
    sent = sent && convertBaseReady && FocusEditWindow(window, edit) && SendLetters(L"koukan");
    PumpMessagesFor(150);
    ITfContext* convertContext = FocusedContext(threadManager);
    bool convertKeyEaten = SendVirtualKey(VK_CONVERT);
    bool convertCandidateReady = sent && convertKeyEaten && WaitForCandidateWindowVisible(true, 2500);
    if (!convertCandidateReady && convertContext) {
        convertKeyEaten = SimulatePreservedKey(threadManager, convertContext, kPreservedConvertKey);
        convertCandidateReady = sent && convertKeyEaten && WaitForCandidateWindowVisible(true, 2500);
    }
    sent = sent && SendVirtualKey(VK_ESCAPE);
    bool compositionCleared = sent && WaitForWindowText(edit, L"", 2500) &&
        WaitForCandidateWindowVisible(false, 2500);
    if (convertContext) {
        convertContext->Release();
        convertContext = nullptr;
    }

    sent = sent && FocusEditWindow(window, edit);
    ITfContext* modeContext = FocusedContext(threadManager);
    bool kanjiOffEaten = modeContext &&
        SimulatePreservedKey(threadManager, modeContext, kPreservedKanjiKey);
    KeyroIME::InputSettingsSnapshot afterKanjiOffSettings;
    bool kanjiOffReady = sent && kanjiOffEaten && WaitForSettings(
        settingsReader,
        KeyroIME::SharedInputMode::English,
        KeyroIME::SharedCharWidth::HalfWidth,
        KeyroIME::SharedKeyboardLayout::Jis,
        afterKanjiOffSettings,
        2000);
    bool kanjiOnEaten = modeContext &&
        SimulatePreservedKey(threadManager, modeContext, kPreservedKanjiKey);
    KeyroIME::InputSettingsSnapshot afterKanjiOnSettings;
    bool kanjiOnReady = sent && kanjiOnEaten && WaitForSettings(
        settingsReader,
        KeyroIME::SharedInputMode::Hiragana,
        KeyroIME::SharedCharWidth::HalfWidth,
        KeyroIME::SharedKeyboardLayout::Jis,
        afterKanjiOnSettings,
        2000);
    if (modeContext) {
        modeContext->Release();
        modeContext = nullptr;
    }

    SetWindowTextW(edit, L"");
    sent = sent && FocusEditWindow(window, edit) && SendShiftOne();
    bool japaneseSymbolReady = sent && WaitForWindowText(edit, L"！", 2500);
    std::wstring afterJapaneseSymbol = WindowText(edit);

    KeyroIME::SharedInputSettings settingsRestorer;
    bool settingsRestored = settingsRestorer.Initialize(true) &&
        settingsRestorer.Write(originalSettings);
    bool osdFaded = osd && WaitForOsdVisibility(osd, false, 2500);

    DestroyWindow(window);
    if (haveOriginalProfile) {
        profileManager->ActivateProfile(
            originalProfile.dwProfileType,
            originalProfile.langid,
            originalProfile.clsid,
            originalProfile.guidProfile,
            originalProfile.hkl,
            TF_IPPMF_FORPROCESS | TF_IPPMF_DONTCARECURRENTINPUTLANGUAGE);
    }
    profileManager->Release();
    threadManager->Deactivate();
    threadManager->Release();
    if (uninitializeCom) CoUninitialize();

    std::wcout << L"menu_osd_visible=" << menuOsdVisible
               << L" shortcut_osd_visible=" << shortcutOsdVisible
               << L" osd_faded=" << osdFaded
               << L" shortcut_layout=" << static_cast<int>(afterShortcutSettings.keyboardLayout)
               << L" menu_layout=" << static_cast<int>(afterMenuSettings.keyboardLayout)
               << L" after_jis=" << afterJis
               << L" after_ansi=" << afterAnsi
               << L" after_menu_restore=" << afterMenuRestore
               << L" after_hiragana=" << afterHiraganaComposition
               << L" after_full_katakana=" << afterFullKatakana
               << L" after_half_katakana=" << afterHalfKatakana
               << L" after_kana_key=" << afterKanaKey
               << L" after_japanese_symbol=" << afterJapaneseSymbol << std::endl;
    bool passed = sent && settingsReadable && settingsRestored && menuSelectionPassed && menuOsdVisible &&
        jisTextReady && menuOsdFaded && shortcutSettingsReady && ansiTextReady &&
        menuRestoreReady && restoredTextReady && jisFunctionBaseReady &&
        kanjiOffReady && kanjiOnReady &&
        hiraganaCompositionReady && fullKatakanaReady && nonConvertFullReady &&
        halfKatakanaReady && nonConvertHalfReady && kanaKeyReady &&
        convertBaseReady && convertCandidateReady && compositionCleared && japaneseSymbolReady &&
        shortcutOsdVisible && osdFaded &&
        afterShortcutSettings.keyboardLayout == KeyroIME::SharedKeyboardLayout::Ansi &&
        afterMenuSettings.keyboardLayout == KeyroIME::SharedKeyboardLayout::Jis &&
        afterJis == L"\"" && afterAnsi == L"\"@" && afterMenuRestore == L"\"@\"" &&
        afterJapaneseSymbol == L"！";
    if (!passed) {
        std::wcerr << L"checks sent=" << sent
                   << L" settings_readable=" << settingsReadable
                   << L" settings_restored=" << settingsRestored
                   << L" menu_selection=" << menuSelectionPassed
                   << L" jis_function_base=" << jisFunctionBaseReady
                   << L" kanji_off=" << kanjiOffReady
                   << L" kanji_on=" << kanjiOnReady
                   << L" hiragana_composition=" << hiraganaCompositionReady
                   << L" full_katakana=" << fullKatakanaReady
                   << L" nonconvert_full=" << nonConvertFullReady
                   << L" half_katakana=" << halfKatakanaReady
                   << L" nonconvert_half=" << nonConvertHalfReady
                   << L" kana_key=" << kanaKeyReady
                   << L" convert_base=" << convertBaseReady
                   << L" convert_candidate=" << convertCandidateReady
                   << L" composition_cleared=" << compositionCleared
                   << L" japanese_symbol=" << japaneseSymbolReady << std::endl;
        return 6;
    }
    std::wcout << L"registered runtime input smoke passed." << std::endl;
    return 0;
}
