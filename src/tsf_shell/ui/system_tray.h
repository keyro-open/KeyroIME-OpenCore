// Copyright (C) 2025-2026 株式会社LocalPro (LocalPro Co., Ltd.). All rights reserved.
// Brand Official Website: https://keyro.jp
//
// This file is part of KeyroIME (キーロ) OpenCore.
// It is source-available under the KeyroIME OpenCore Non-Commercial Source
// License 1.0. See LICENSE. Commercial use requires a separate written license
// from 株式会社LocalPro.
// system_tray.h
// KeyroIME TSF シェル - システムトレイと右クリックメニュー
// Windows 通知領域に常駐し、日本語 UI を提供します。

#pragma once
#include <msctf.h>
#include <windows.h>
#include <string>
#include <vector>

#include "../ipc/named_pipe_client.h"
#include "../tsf_core/shared_settings.h"
#include "status_osd.h"

namespace KeyroIME {

/// 入力モード列挙型
using InputMode = SharedInputMode;

/// 文字幅列挙型
using CharWidth = SharedCharWidth;

/// キーボード配列列挙型
using TrayKeyboardLayout = SharedKeyboardLayout;
using PunctuationStyle = SharedPunctuationStyle;
using EnglishCase = SharedEnglishCase;

/// システムトレイマネージャー
class SystemTray {
public:
    SystemTray();
    ~SystemTray();

    /// 初期化システムトレイ
    bool Initialize(HINSTANCE hInstance, HWND hParent);

    /// トレイアイコンを更新します（日本語モード [あ] または英語モード [A]）。
    void UpdateIcon(InputMode mode);

    /// トレイアイコンを表示します。
    void Show();

    /// トレイアイコンを隠します。
    void Hide();

    /// 現在の入力モードを取得します。
    InputMode GetInputMode() const { return m_inputMode; }

    /// 現在の入力モードを設定します。
    void SetInputMode(InputMode mode);

    /// 現在の文字幅を取得します。
    CharWidth GetCharWidth() const { return m_charWidth; }

    /// 現在の文字幅を設定します。
    void SetCharWidth(CharWidth width);

    /// 現在のキーボード配列を取得します。
    TrayKeyboardLayout GetKeyboardLayout() const { return m_keyboardLayout; }

    /// 現在のキーボード配列を設定します。
    void SetKeyboardLayout(TrayKeyboardLayout layout);

    PunctuationStyle GetPunctuationStyle() const { return m_punctuationStyle; }
    void SetPunctuationStyle(PunctuationStyle style);

    EnglishCase GetEnglishCase() const { return m_englishCase; }
    void SetEnglishCase(EnglishCase englishCase);

    /// ネイティブメニューを構築します。呼び出し元が DestroyMenu を担当し、トレイ表示とローカルスモークテストで使用します。
    HMENU CreateContextMenu() const;

#ifdef KEYRO_TRAY_TESTING
    void SetActiveTipForTest(bool active);
    void SetUpdateReminderDueForTest(bool due);
    void AcknowledgeUpdateReminderForTest();
    bool IsVisibleForTest() const { return m_isVisible; }
    bool QueryActiveTipForTest(bool& active) const { return TryGetKeyroImeActive(active); }
    bool IsStatusOsdVisibleForTest() const { return m_statusOsd.IsVisible(); }
    const std::wstring& StatusOsdTextForTest() const { return m_statusOsd.CurrentText(); }
    void SetUpdateBadgeForTest(bool visible);
    bool IsUpdateBadgeVisibleForTest() const { return m_updateBadgeVisible; }
    void ShowAboutDialogForTest() { ShowAboutDialog(); }
    void ShowLicenseWindowForTest() { ShowLicenseWindow(); }
#endif

private:
    HINSTANCE m_hInstance;
    HWND m_hParent;
    HWND m_messageWindow;
    NOTIFYICONDATAW m_nid;
    bool m_isVisible;
    UINT m_taskbarCreatedMessage;
    SharedInputSettings m_sharedSettings;
    NamedPipeClient m_pipeClient;
    bool m_pipeStarted;
    StatusOsd m_statusOsd;
    mutable std::vector<HBITMAP> m_menuBitmaps;
    ITfInputProcessorProfileMgr* m_profileManager;
    ITfInputProcessorProfiles* m_inputProcessorProfiles;
    bool m_keyroImeActive;
    bool m_updateBadgeVisible;
    ULONGLONG m_nextUpdateReminderUtc;

    // 現在状態
    InputMode m_inputMode;
    CharWidth m_charWidth;
    TrayKeyboardLayout m_keyboardLayout;
    PunctuationStyle m_punctuationStyle;
    EnglishCase m_englishCase;

    // メニューコマンド ID
    static constexpr UINT WM_TRAYICON = WM_USER + 1;
    static constexpr UINT ID_MENU_HIRAGANA = 1001;
    static constexpr UINT ID_MENU_KATAKANA = 1002;
    static constexpr UINT ID_MENU_ENGLISH = 1003;
    static constexpr UINT ID_MENU_HALFWIDTH = 1004;
    static constexpr UINT ID_MENU_FULLWIDTH = 1005;
    static constexpr UINT ID_MENU_JIS = 1006;
    static constexpr UINT ID_MENU_ANSI = 1007;
    static constexpr UINT ID_MENU_ABOUT = 1008;
    static constexpr UINT ID_MENU_PUNCTUATION_JAPANESE = 1009;
    static constexpr UINT ID_MENU_PUNCTUATION_WESTERN = 1010;
    static constexpr UINT ID_MENU_UPDATE_CHECK = 1011;
    static constexpr UINT_PTR TIMER_SETTINGS_SYNC = 1;
    static constexpr UINT_PTR TIMER_ACTIVE_TIP = 2;

    /// トレイアイコンを作成します。
    HICON CreateTrayIcon(InputMode mode);
    void ClearMenuBitmaps() const;
    HBITMAP CreateModeMenuBitmap(int iconSize) const;
    HBITMAP CreateRedDotMenuBitmap(int iconSize) const;
    void SetMenuItemBitmap(HMENU menu, UINT item, bool byPosition, HBITMAP bitmap) const;
    void SetMenuItemModeIcon(HMENU menu, UINT item, bool byPosition) const;
    void SetMenuItemRedDot(HMENU menu, UINT item, bool byPosition) const;

    /// トレイメッセージを処理します。
    void OnTrayMessage(WPARAM wParam, LPARAM lParam);

    /// 左クリック時にショートカットヒントバルーンを表示します。
    void OnLeftClick();

    /// 右クリック：表示コンテキストメニュー
    void OnRightClick();

    /// メニューコマンドを処理します。
    void OnMenuCommand(UINT commandId);

    /// 通知バルーンを表示します。
    void ShowNotification(const std::wstring& title, const std::wstring& message);

    /// 現在の物理キーボード配列を検出します。
    TrayKeyboardLayout DetectPhysicalKeyboardLayout();

    /// About ダイアログを表示します。
    void ShowAboutDialog();
    void ShowLicenseWindow();
    void OpenUrl(const wchar_t* url) const;
    void HandleUpdateCheckCommand();

    void PublishSettings();
    void PublishSettingsToService();
    void RestoreTrayIcon();
    void OnTimer(UINT_PTR timerId);
    void SyncSettingsFromShared();
    void RefreshActiveTipVisibility();
    void RefreshUpdateReminder();
    void ResetUpdateReminder();
    void LoadUpdateReminderState();
    void SaveUpdateReminderState() const;
    bool TryGetKeyroImeActive(bool& active) const;
    void ShowCurrentStatus();
    std::wstring FormatCurrentStatus() const;
    std::wstring FormatHelpMenuText() const;
    std::wstring FormatUpdateMenuText() const;

    friend LRESULT CALLBACK TrayWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

} // namespace KeyroIME
