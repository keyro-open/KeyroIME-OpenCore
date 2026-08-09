// Copyright (C) 2025-2026 株式会社LocalPro (LocalPro Co., Ltd.). All rights reserved.
// Brand Official Website: https://keyro.jp
//
// This file is part of KeyroIME (キーロ) OpenCore.
// GNU GPLv3に基づいて配布されます。LICENSE（英語正文）を参照してください。
// system_tray.cpp

#include "system_tray.h"

#include <commctrl.h>
#include <shellapi.h>
#include <strsafe.h>
#include <vector>

#include "../resource.h"
#include "../tsf_core/tip_guid.h"
#include "keyro_version.h"

namespace KeyroIME {

namespace {

constexpr wchar_t kTrayWindowClassName[] = L"KeyroIME_Tray_MessageWindow";
constexpr wchar_t kAboutWindowClassName[] = L"KeyroIME_AboutWindow";
constexpr wchar_t kLicenseWindowClassName[] = L"KeyroIME_LicenseWindow";
constexpr wchar_t kOfficialUrl[] = L"https://keyro.jp";
constexpr wchar_t kSupportUrl[] = L"https://keyro.jp/keyroime_opencore/support";
constexpr wchar_t kRegistrySubkey[] = L"Software\\KeyroIME\\OpenCore";
constexpr wchar_t kNextUpdateReminderUtcValue[] = L"NextUpdateReminderUtc";
constexpr ULONGLONG kFileTimeTicksPerSecond = 10000000ULL;
constexpr ULONGLONG kUpdateReminderIntervalUtc =
    14ULL * 24ULL * 60ULL * 60ULL * kFileTimeTicksPerSecond;
constexpr int kLicenseEditId = 2001;
constexpr int kLicenseTitleId = 2002;
constexpr int kLicenseSubtitleId = 2003;
constexpr int kLicenseOfficialLinkId = 2004;
constexpr int kLicenseCloseId = 2005;
constexpr int kAboutTitleId = 2101;
constexpr int kAboutVersionId = 2102;
constexpr int kAboutBodyId = 2103;
constexpr int kAboutOfficialLinkId = 2104;
constexpr int kAboutSupportLinkId = 2105;
constexpr int kAboutLicenseLinkId = 2106;
constexpr int kAboutCloseId = 2107;
#ifdef KEYRO_TRAY_TESTING
const GUID kTrayGuid = {
    0xc6477ad6, 0xc2c4, 0x4dbc, {0x92, 0x79, 0xc4, 0x6d, 0x7b, 0xee, 0x8d, 0x52}
};
#else
const GUID kTrayGuid = {
    0x7a30bc62, 0xc70d, 0x4c2d, {0xa0, 0xea, 0xa6, 0xd3, 0x5c, 0x91, 0x4c, 0x1f}
};
#endif

bool SettingsEqual(
    const InputSettingsSnapshot& left,
    const InputSettingsSnapshot& right)
{
    return left.inputMode == right.inputMode &&
        left.charWidth == right.charWidth &&
        left.keyboardLayout == right.keyboardLayout &&
        left.punctuationStyle == right.punctuationStyle &&
        left.englishCase == right.englishCase;
}

ULONGLONG FileTimeToUInt64(const FILETIME& fileTime)
{
    return (static_cast<ULONGLONG>(fileTime.dwHighDateTime) << 32) |
        static_cast<ULONGLONG>(fileTime.dwLowDateTime);
}

ULONGLONG CurrentUtcFileTime()
{
    FILETIME fileTime = {};
    GetSystemTimeAsFileTime(&fileTime);
    return FileTimeToUInt64(fileTime);
}

std::wstring Utf8ToWide(const char* data, int byteCount)
{
    if (!data || byteCount <= 0) {
        return std::wstring();
    }

    int wideCount = MultiByteToWideChar(CP_UTF8, 0, data, byteCount, nullptr, 0);
    if (wideCount <= 0) {
        return std::wstring();
    }

    std::wstring text(static_cast<size_t>(wideCount), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, data, byteCount, text.data(), wideCount);
    return text;
}

std::wstring ReadUtf8TextFile(const std::wstring& path)
{
    HANDLE file = CreateFileW(
        path.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return std::wstring();
    }

    LARGE_INTEGER size = {};
    if (!GetFileSizeEx(file, &size) || size.QuadPart <= 0 || size.QuadPart > 1024 * 1024) {
        CloseHandle(file);
        return std::wstring();
    }

    std::vector<char> bytes(static_cast<size_t>(size.QuadPart));
    DWORD bytesRead = 0;
    BOOL readOk = ReadFile(
        file,
        bytes.data(),
        static_cast<DWORD>(bytes.size()),
        &bytesRead,
        nullptr);
    CloseHandle(file);
    if (!readOk || bytesRead == 0) {
        return std::wstring();
    }

    return Utf8ToWide(bytes.data(), static_cast<int>(bytesRead));
}

std::wstring BuildExecutableSiblingPath(const wchar_t* fileName)
{
    wchar_t modulePath[MAX_PATH] = {};
    DWORD length = GetModuleFileNameW(nullptr, modulePath, ARRAYSIZE(modulePath));
    if (length == 0 || length >= ARRAYSIZE(modulePath)) {
        return std::wstring();
    }

    wchar_t* slash = wcsrchr(modulePath, L'\\');
    if (!slash) {
        return std::wstring(fileName);
    }
    *(slash + 1) = L'\0';

    std::wstring path(modulePath);
    path.append(fileName);
    return path;
}

LRESULT CALLBACK AboutWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK LicenseWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

struct InfoWindowState {
    HFONT titleFont = nullptr;
    HFONT headingFont = nullptr;
    HFONT bodyFont = nullptr;
    HFONT smallFont = nullptr;
    HBRUSH backgroundBrush = nullptr;
    HBRUSH editBrush = nullptr;
};

int WindowDpi(HWND hwnd)
{
    HDC dc = GetDC(hwnd);
    int dpi = dc ? GetDeviceCaps(dc, LOGPIXELSY) : 96;
    if (dc) {
        ReleaseDC(hwnd, dc);
    }
    return dpi > 0 ? dpi : 96;
}

int ScaleForWindow(HWND hwnd, int value)
{
    return MulDiv(value, WindowDpi(hwnd), 96);
}

HFONT CreateUiFont(HWND hwnd, int pointSize, LONG weight)
{
    LOGFONTW font = {};
    font.lfHeight = -MulDiv(pointSize, WindowDpi(hwnd), 72);
    font.lfWeight = weight;
    font.lfQuality = CLEARTYPE_QUALITY;
    StringCchCopyW(font.lfFaceName, ARRAYSIZE(font.lfFaceName), L"Segoe UI");
    return CreateFontIndirectW(&font);
}

InfoWindowState* CreateInfoWindowState(HWND hwnd)
{
    auto* state = new InfoWindowState();
    state->titleFont = CreateUiFont(hwnd, 20, 700);
    state->headingFont = CreateUiFont(hwnd, 12, 600);
    state->bodyFont = CreateUiFont(hwnd, 10, 400);
    state->smallFont = CreateUiFont(hwnd, 9, 400);
    state->backgroundBrush = CreateSolidBrush(RGB(248, 250, 252));
    state->editBrush = CreateSolidBrush(RGB(255, 255, 255));
    return state;
}

void DestroyInfoWindowState(InfoWindowState* state)
{
    if (!state) {
        return;
    }
    if (state->titleFont) DeleteObject(state->titleFont);
    if (state->headingFont) DeleteObject(state->headingFont);
    if (state->bodyFont) DeleteObject(state->bodyFont);
    if (state->smallFont) DeleteObject(state->smallFont);
    if (state->backgroundBrush) DeleteObject(state->backgroundBrush);
    if (state->editBrush) DeleteObject(state->editBrush);
    delete state;
}

InfoWindowState* WindowState(HWND hwnd)
{
    return reinterpret_cast<InfoWindowState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
}

void ApplyFont(HWND hwnd, int controlId, HFONT font)
{
    HWND control = GetDlgItem(hwnd, controlId);
    if (control && font) {
        SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    }
}

void OpenWindowUrl(HWND hwnd, const wchar_t* url)
{
    if (!url || url[0] == L'\0') {
        return;
    }
    ShellExecuteW(hwnd, L"open", url, nullptr, nullptr, SW_SHOWNORMAL);
}

HWND CreateClickableLink(
    HWND parent,
    int controlId,
    const wchar_t* linkText,
    const wchar_t* fallbackText,
    HINSTANCE hInstance)
{
    HWND control = CreateWindowExW(
        0,
        WC_LINK,
        linkText,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        0,
        0,
        0,
        0,
        parent,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(controlId)),
        hInstance,
        nullptr);
    if (control) {
        return control;
    }

    return CreateWindowExW(
        0,
        L"BUTTON",
        fallbackText,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_FLAT | BS_LEFT,
        0,
        0,
        0,
        0,
        parent,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(controlId)),
        hInstance,
        nullptr);
}

std::wstring NormalizeLineBreaks(const std::wstring& text)
{
    std::wstring result;
    result.reserve(text.size() + 32);
    for (size_t index = 0; index < text.size(); ++index) {
        wchar_t ch = text[index];
        if (ch == L'\r') {
            result.append(L"\r\n");
            if (index + 1 < text.size() && text[index + 1] == L'\n') {
                ++index;
            }
        } else if (ch == L'\n') {
            result.append(L"\r\n");
        } else {
            result.push_back(ch);
        }
    }
    return result;
}

std::wstring BuildLicenseDisplayText()
{
    std::wstring text;
    std::wstring ja = ReadUtf8TextFile(BuildExecutableSiblingPath(L"LICENSE_ja.txt"));
    std::wstring en = ReadUtf8TextFile(BuildExecutableSiblingPath(L"LICENSE_en.txt"));
    if (!ja.empty()) {
        text.append(ja);
        text.append(L"\r\n\r\n");
    }
    if (!en.empty()) {
        text.append(en);
    }
    if (text.empty()) {
        text =
            L"LICENSE_ja.txt / LICENSE_en.txt が見つかりません。\r\n"
            L"release パッケージまたはインストール先の KeyroIME フォルダーを確認してください。";
    }
    return NormalizeLineBreaks(text);
}

bool RegisterInfoWindowClass(
    HINSTANCE hInstance,
    const wchar_t* className,
    WNDPROC windowProc)
{
    WNDCLASSW windowClass = {};
    windowClass.lpfnWndProc = windowProc;
    windowClass.hInstance = hInstance;
    windowClass.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
    windowClass.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_KEYROIME));
    windowClass.lpszClassName = className;
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    if (RegisterClassW(&windowClass) == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return false;
    }
    return true;
}

void CenterWindowOnOwner(HWND window, HWND owner);

void ShowLicenseWindowFromOwner(HWND owner)
{
    HINSTANCE hInstance = reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(owner, GWLP_HINSTANCE));
    if (!hInstance) {
        hInstance = GetModuleHandleW(nullptr);
    }
    if (!RegisterInfoWindowClass(hInstance, kLicenseWindowClassName, LicenseWindowProc)) {
        return;
    }

    std::wstring text = BuildLicenseDisplayText();
    HWND window = CreateWindowExW(
        WS_EX_APPWINDOW,
        kLicenseWindowClassName,
        L"KeyroIME OpenCore GNU GPLv3",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        ScaleForWindow(owner, 880),
        ScaleForWindow(owner, 700),
        owner,
        nullptr,
        hInstance,
        &text);
    if (window) {
        CenterWindowOnOwner(window, owner);
        ShowWindow(window, SW_SHOWNORMAL);
        UpdateWindow(window);
    }
}

void CenterWindowOnOwner(HWND window, HWND owner)
{
    RECT windowRect = {};
    GetWindowRect(window, &windowRect);
    int width = windowRect.right - windowRect.left;
    int height = windowRect.bottom - windowRect.top;

    HMONITOR monitor = MonitorFromWindow(owner ? owner : window, MONITOR_DEFAULTTONEAREST);
    MONITORINFO info = {};
    info.cbSize = sizeof(info);
    if (!GetMonitorInfoW(monitor, &info)) {
        return;
    }
    int x = info.rcWork.left + ((info.rcWork.right - info.rcWork.left) - width) / 2;
    int y = info.rcWork.top + ((info.rcWork.bottom - info.rcWork.top) - height) / 2;
    SetWindowPos(window, nullptr, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
}

void ResizeAboutWindow(HWND hwnd)
{
    RECT client = {};
    GetClientRect(hwnd, &client);
    int width = client.right - client.left;
    int height = client.bottom - client.top;
    int margin = ScaleForWindow(hwnd, 32);
    int contentWidth = width - margin * 2;
    int y = ScaleForWindow(hwnd, 24);

    MoveWindow(GetDlgItem(hwnd, kAboutTitleId), margin, y, contentWidth, ScaleForWindow(hwnd, 34), TRUE);
    y += ScaleForWindow(hwnd, 42);
    MoveWindow(GetDlgItem(hwnd, kAboutVersionId), margin, y, contentWidth, ScaleForWindow(hwnd, 24), TRUE);
    y += ScaleForWindow(hwnd, 42);
    MoveWindow(GetDlgItem(hwnd, kAboutBodyId), margin, y, contentWidth, ScaleForWindow(hwnd, 112), TRUE);
    y += ScaleForWindow(hwnd, 132);
    MoveWindow(GetDlgItem(hwnd, kAboutOfficialLinkId), margin, y, contentWidth, ScaleForWindow(hwnd, 28), TRUE);
    y += ScaleForWindow(hwnd, 34);
    MoveWindow(GetDlgItem(hwnd, kAboutSupportLinkId), margin, y, contentWidth, ScaleForWindow(hwnd, 28), TRUE);
    y += ScaleForWindow(hwnd, 34);
    MoveWindow(GetDlgItem(hwnd, kAboutLicenseLinkId), margin, y, contentWidth, ScaleForWindow(hwnd, 28), TRUE);

    int buttonWidth = ScaleForWindow(hwnd, 112);
    int buttonHeight = ScaleForWindow(hwnd, 32);
    MoveWindow(
        GetDlgItem(hwnd, kAboutCloseId),
        width - margin - buttonWidth,
        height - margin - buttonHeight,
        buttonWidth,
        buttonHeight,
        TRUE);
}

void ResizeLicenseWindow(HWND hwnd)
{
    RECT client = {};
    GetClientRect(hwnd, &client);
    int width = client.right - client.left;
    int height = client.bottom - client.top;
    int margin = ScaleForWindow(hwnd, 28);
    int contentWidth = width - margin * 2;
    int y = ScaleForWindow(hwnd, 20);

    MoveWindow(GetDlgItem(hwnd, kLicenseTitleId), margin, y, contentWidth, ScaleForWindow(hwnd, 32), TRUE);
    y += ScaleForWindow(hwnd, 38);
    MoveWindow(GetDlgItem(hwnd, kLicenseSubtitleId), margin, y, contentWidth, ScaleForWindow(hwnd, 28), TRUE);
    y += ScaleForWindow(hwnd, 34);
    MoveWindow(GetDlgItem(hwnd, kLicenseOfficialLinkId), margin, y, contentWidth, ScaleForWindow(hwnd, 26), TRUE);
    y += ScaleForWindow(hwnd, 34);

    int buttonWidth = ScaleForWindow(hwnd, 112);
    int buttonHeight = ScaleForWindow(hwnd, 32);
    int editBottom = height - margin - buttonHeight - ScaleForWindow(hwnd, 16);
    int editHeight = editBottom > y ? editBottom - y : ScaleForWindow(hwnd, 240);
    MoveWindow(GetDlgItem(hwnd, kLicenseEditId), margin, y, contentWidth, editHeight, TRUE);
    MoveWindow(
        GetDlgItem(hwnd, kLicenseCloseId),
        width - margin - buttonWidth,
        height - margin - buttonHeight,
        buttonWidth,
        buttonHeight,
        TRUE);
}

void HandleInfoWindowLink(HWND hwnd, LPARAM lParam)
{
    auto* link = reinterpret_cast<NMLINK*>(lParam);
    const wchar_t* url = link->item.szUrl;
    if (wcscmp(url, L"keyro://license") == 0) {
        ShowLicenseWindowFromOwner(hwnd);
    } else {
        OpenWindowUrl(hwnd, url);
    }
}

void CreateAboutControls(HWND hwnd)
{
    auto* state = WindowState(hwnd);
    HINSTANCE hInstance = reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(hwnd, GWLP_HINSTANCE));
    CreateWindowExW(
        0,
        L"STATIC",
        L"KeyroIME OpenCore（キーロ入力）",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        0,
        0,
        0,
        0,
        hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kAboutTitleId)),
        hInstance,
        nullptr);
    CreateWindowExW(
        0,
        L"STATIC",
        L"Version v" KEYROIME_PRODUCT_VERSION_W L"  |  OpenCore",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        0,
        0,
        0,
        0,
        hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kAboutVersionId)),
        hInstance,
        nullptr);
    CreateWindowExW(
        0,
        L"STATIC",
        L"GNU GPLv3 に基づく自由なオープンソース日本語入力ソフトウェアです。\n"
        L"純 C++17 TSF シェルと Rust 製ローカルコアサービスを基盤に、"
        L"ローカル辞書と、遅延を抑えたプロセス分離アーキテクチャを備えています。\n"
        L"個人利用、検証、開発用途で安心して使える OpenCore 版です。",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        0,
        0,
        0,
        0,
        hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kAboutBodyId)),
        hInstance,
        nullptr);
    CreateClickableLink(
        hwnd,
        kAboutOfficialLinkId,
        L"公式サイト　<a href=\"https://keyro.jp\">https://keyro.jp</a>",
        L"公式サイト  https://keyro.jp",
        hInstance);
    CreateClickableLink(
        hwnd,
        kAboutSupportLinkId,
        L"使用ヘルプ　<a href=\"https://keyro.jp/keyroime_opencore/support\">https://keyro.jp/keyroime_opencore/support</a>",
        L"使用ヘルプ  https://keyro.jp/keyroime_opencore/support",
        hInstance);
    CreateClickableLink(
        hwnd,
        kAboutLicenseLinkId,
        L"ライセンス　<a href=\"keyro://license\">GNU GPLv3 を表示</a>",
        L"ライセンス  GNU GPLv3 を表示",
        hInstance);
    CreateWindowExW(
        0,
        L"BUTTON",
        L"閉じる",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
        0,
        0,
        0,
        0,
        hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kAboutCloseId)),
        hInstance,
        nullptr);

    ApplyFont(hwnd, kAboutTitleId, state->titleFont);
    ApplyFont(hwnd, kAboutVersionId, state->headingFont);
    ApplyFont(hwnd, kAboutBodyId, state->bodyFont);
    ApplyFont(hwnd, kAboutOfficialLinkId, state->bodyFont);
    ApplyFont(hwnd, kAboutSupportLinkId, state->bodyFont);
    ApplyFont(hwnd, kAboutLicenseLinkId, state->bodyFont);
    ApplyFont(hwnd, kAboutCloseId, state->bodyFont);
}

void CreateLicenseControls(HWND hwnd, const std::wstring& text)
{
    auto* state = WindowState(hwnd);
    HINSTANCE hInstance = reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(hwnd, GWLP_HINSTANCE));
    CreateWindowExW(
        0,
        L"STATIC",
        L"KeyroIME OpenCore GNU GPLv3",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        0,
        0,
        0,
        0,
        hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kLicenseTitleId)),
        hInstance,
        nullptr);
    CreateWindowExW(
        0,
        L"STATIC",
        L"著作権と GNU GPLv3 の利用条件に関する重要な情報です。",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        0,
        0,
        0,
        0,
        hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kLicenseSubtitleId)),
        hInstance,
        nullptr);
    CreateClickableLink(
        hwnd,
        kLicenseOfficialLinkId,
        L"公式サイト　<a href=\"https://keyro.jp\">keyro.jp</a>　 / 　企業情報　<a href=\"https://localpro.jp\">localpro.jp</a>",
        L"公式サイト  keyro.jp / localpro.jp",
        hInstance);
    HWND edit = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        text.c_str(),
        WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY |
            WS_VSCROLL | ES_AUTOVSCROLL | ES_WANTRETURN,
        0,
        0,
        0,
        0,
        hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kLicenseEditId)),
        hInstance,
        nullptr);
    CreateWindowExW(
        0,
        L"BUTTON",
        L"閉じる",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
        0,
        0,
        0,
        0,
        hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kLicenseCloseId)),
        hInstance,
        nullptr);

    ApplyFont(hwnd, kLicenseTitleId, state->titleFont);
    ApplyFont(hwnd, kLicenseSubtitleId, state->bodyFont);
    ApplyFont(hwnd, kLicenseOfficialLinkId, state->bodyFont);
    ApplyFont(hwnd, kLicenseEditId, state->smallFont);
    ApplyFont(hwnd, kLicenseCloseId, state->bodyFont);
    if (edit) {
        SendMessageW(edit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(14, 14));
    }
}

LRESULT PaintControlBackground(HWND hwnd, HDC dc)
{
    auto* state = WindowState(hwnd);
    if (!state) {
        return 0;
    }
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(31, 41, 55));
    return reinterpret_cast<LRESULT>(state->backgroundBrush);
}

LRESULT PaintLicenseEditBackground(HWND hwnd, HDC dc)
{
    auto* state = WindowState(hwnd);
    SetBkColor(dc, RGB(255, 255, 255));
    SetTextColor(dc, RGB(31, 41, 55));
    return reinterpret_cast<LRESULT>(state ? state->editBrush : nullptr);
}

LRESULT CALLBACK AboutWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_CREATE: {
            auto* state = CreateInfoWindowState(hwnd);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
            CreateAboutControls(hwnd);
            ResizeAboutWindow(hwnd);
            return 0;
        }
        case WM_SIZE:
            ResizeAboutWindow(hwnd);
            return 0;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case kAboutOfficialLinkId:
                    OpenWindowUrl(hwnd, kOfficialUrl);
                    return 0;
                case kAboutSupportLinkId:
                    OpenWindowUrl(hwnd, kSupportUrl);
                    return 0;
                case kAboutLicenseLinkId:
                    ShowLicenseWindowFromOwner(hwnd);
                    return 0;
                case kAboutCloseId:
                    DestroyWindow(hwnd);
                    return 0;
                default:
                    break;
            }
            break;
        case WM_NOTIFY: {
            auto* header = reinterpret_cast<NMHDR*>(lParam);
            if (header && (header->code == NM_CLICK || header->code == NM_RETURN)) {
                HandleInfoWindowLink(hwnd, lParam);
                return 0;
            }
            break;
        }
        case WM_CTLCOLORSTATIC:
            return PaintControlBackground(hwnd, reinterpret_cast<HDC>(wParam));
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            DestroyInfoWindowState(WindowState(hwnd));
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK LicenseWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_CREATE: {
            auto* state = CreateInfoWindowState(hwnd);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
            auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
            auto* text = reinterpret_cast<std::wstring*>(create->lpCreateParams);
            CreateLicenseControls(hwnd, text ? *text : std::wstring());
            ResizeLicenseWindow(hwnd);
            return 0;
        }
        case WM_SIZE:
            ResizeLicenseWindow(hwnd);
            return 0;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case kLicenseOfficialLinkId:
                    OpenWindowUrl(hwnd, kOfficialUrl);
                    return 0;
                case kLicenseCloseId:
                    DestroyWindow(hwnd);
                    return 0;
                default:
                    break;
            }
            break;
        case WM_NOTIFY: {
            auto* header = reinterpret_cast<NMHDR*>(lParam);
            if (header && (header->code == NM_CLICK || header->code == NM_RETURN)) {
                HandleInfoWindowLink(hwnd, lParam);
                return 0;
            }
            break;
        }
        case WM_CTLCOLORSTATIC:
            if (reinterpret_cast<HWND>(lParam) == GetDlgItem(hwnd, kLicenseEditId)) {
                return PaintLicenseEditBackground(hwnd, reinterpret_cast<HDC>(wParam));
            }
            return PaintControlBackground(hwnd, reinterpret_cast<HDC>(wParam));
        case WM_CTLCOLOREDIT: {
            return PaintLicenseEditBackground(hwnd, reinterpret_cast<HDC>(wParam));
        }
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            DestroyInfoWindowState(WindowState(hwnd));
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

} // namespace

SystemTray::SystemTray()
    : m_hInstance(nullptr)
    , m_hParent(nullptr)
    , m_messageWindow(nullptr)
    , m_isVisible(false)
    , m_taskbarCreatedMessage(0)
    , m_pipeStarted(false)
    , m_profileManager(nullptr)
    , m_inputProcessorProfiles(nullptr)
    , m_keyroImeActive(true)
    , m_updateBadgeVisible(false)
    , m_nextUpdateReminderUtc(0)
    , m_inputMode(InputMode::Hiragana)
    , m_charWidth(CharWidth::HalfWidth)
    , m_keyboardLayout(TrayKeyboardLayout::Jis)
    , m_punctuationStyle(PunctuationStyle::Japanese)
    , m_englishCase(EnglishCase::Lower)
{
    ZeroMemory(&m_nid, sizeof(m_nid));
}

SystemTray::~SystemTray()
{
    if (m_messageWindow) {
        KillTimer(m_messageWindow, TIMER_SETTINGS_SYNC);
        KillTimer(m_messageWindow, TIMER_ACTIVE_TIP);
    }
    m_statusOsd.Hide();
    if (m_pipeStarted) {
        m_pipeClient.Stop();
        m_pipeStarted = false;
    }
    Hide();
    if (m_nid.hIcon) {
        DestroyIcon(m_nid.hIcon);
        m_nid.hIcon = nullptr;
    }
    ClearMenuBitmaps();
    if (m_messageWindow) {
        DestroyWindow(m_messageWindow);
        m_messageWindow = nullptr;
    }
    if (m_profileManager) {
        m_profileManager->Release();
        m_profileManager = nullptr;
    }
    if (m_inputProcessorProfiles) {
        m_inputProcessorProfiles->Release();
        m_inputProcessorProfiles = nullptr;
    }
    if (m_hInstance) {
        UnregisterClassW(kTrayWindowClassName, m_hInstance);
    }
}

bool SystemTray::Initialize(HINSTANCE hInstance, HWND hParent)
{
    if (!hInstance) {
        return false;
    }

    m_hInstance = hInstance;
    m_hParent = hParent;

    INITCOMMONCONTROLSEX commonControls = {};
    commonControls.dwSize = sizeof(commonControls);
    commonControls.dwICC = ICC_STANDARD_CLASSES | ICC_LINK_CLASS;
    InitCommonControlsEx(&commonControls);

    WNDCLASSW windowClass = {};
    windowClass.lpfnWndProc = TrayWindowProc;
    windowClass.hInstance = hInstance;
    windowClass.lpszClassName = kTrayWindowClassName;
    if (RegisterClassW(&windowClass) == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return false;
    }

    m_messageWindow = CreateWindowExW(
        0,
        kTrayWindowClassName,
        L"KeyroIME Tray",
        WS_OVERLAPPED,
        0,
        0,
        0,
        0,
        nullptr,
        nullptr,
        hInstance,
        this);
    if (!m_messageWindow) {
        return false;
    }

    m_taskbarCreatedMessage = RegisterWindowMessageW(L"TaskbarCreated");
    m_statusOsd.Initialize(hInstance);
    if (m_sharedSettings.Initialize(true)) {
        InputSettingsSnapshot snapshot = m_sharedSettings.Read();
        m_inputMode = snapshot.inputMode;
        m_charWidth = snapshot.charWidth;
        m_keyboardLayout = snapshot.keyboardLayout;
        m_punctuationStyle = snapshot.punctuationStyle;
        m_englishCase = snapshot.englishCase;
    }
    LoadUpdateReminderState();

    CoCreateInstance(
        CLSID_TF_InputProcessorProfiles,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_ITfInputProcessorProfileMgr,
        reinterpret_cast<void**>(&m_profileManager));
    CoCreateInstance(
        CLSID_TF_InputProcessorProfiles,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_ITfInputProcessorProfiles,
        reinterpret_cast<void**>(&m_inputProcessorProfiles));
    bool active = true;
    if (TryGetKeyroImeActive(active)) {
        m_keyroImeActive = active;
    }

    m_nid.cbSize = sizeof(NOTIFYICONDATAW);
    m_nid.hWnd = m_messageWindow;
    m_nid.uID = 1;
    m_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_GUID;
    m_nid.uCallbackMessage = WM_TRAYICON;
    m_nid.guidItem = kTrayGuid;
    StringCchCopyW(m_nid.szTip, ARRAYSIZE(m_nid.szTip), L"KeyroIME");
    m_nid.hIcon = CreateTrayIcon(m_inputMode);
    m_pipeStarted = m_pipeClient.Start();
    PublishSettingsToService();
    RefreshUpdateReminder();
    SetTimer(m_messageWindow, TIMER_SETTINGS_SYNC, 75, nullptr);
    SetTimer(m_messageWindow, TIMER_ACTIVE_TIP, 250, nullptr);
    return m_nid.hIcon != nullptr;
}

void SystemTray::Show()
{
    if (m_isVisible || !m_messageWindow || !m_nid.hIcon) {
        return;
    }

    if (Shell_NotifyIconW(NIM_ADD, &m_nid)) {
        m_nid.uVersion = NOTIFYICON_VERSION_4;
        Shell_NotifyIconW(NIM_SETVERSION, &m_nid);
        m_isVisible = true;
    }
}

void SystemTray::Hide()
{
    if (m_isVisible) {
        Shell_NotifyIconW(NIM_DELETE, &m_nid);
        m_isVisible = false;
    }
}

#ifdef KEYRO_TRAY_TESTING
void SystemTray::SetActiveTipForTest(bool active)
{
    m_keyroImeActive = active;
    if (!active) {
        m_statusOsd.Hide();
    }
    Show();
}

void SystemTray::SetUpdateReminderDueForTest(bool due)
{
    ULONGLONG now = CurrentUtcFileTime();
    m_nextUpdateReminderUtc = due ? now - 1 : now + kUpdateReminderIntervalUtc;
    RefreshUpdateReminder();
}

void SystemTray::AcknowledgeUpdateReminderForTest()
{
    ResetUpdateReminder();
}

void SystemTray::SetUpdateBadgeForTest(bool visible)
{
    SetUpdateReminderDueForTest(visible);
}
#endif

void SystemTray::UpdateIcon(InputMode mode)
{
    m_inputMode = mode;
    HICON nextIcon = CreateTrayIcon(mode);
    if (!nextIcon) {
        return;
    }

    HICON previousIcon = m_nid.hIcon;
    m_nid.hIcon = nextIcon;
    if (m_isVisible) {
        Shell_NotifyIconW(NIM_MODIFY, &m_nid);
    }
    if (previousIcon) {
        DestroyIcon(previousIcon);
    }
}

void SystemTray::SetInputMode(InputMode mode)
{
    UpdateIcon(mode);
    PublishSettings();
}

void SystemTray::SetCharWidth(CharWidth width)
{
    m_charWidth = width;
    PublishSettings();
}

void SystemTray::SetKeyboardLayout(TrayKeyboardLayout layout)
{
    m_keyboardLayout = layout;
    PublishSettings();
}

void SystemTray::SetPunctuationStyle(PunctuationStyle style)
{
    m_punctuationStyle = style;
    PublishSettings();
}

void SystemTray::SetEnglishCase(EnglishCase englishCase)
{
    m_englishCase = englishCase;
    PublishSettings();
}

HICON SystemTray::CreateTrayIcon(InputMode mode)
{
    UNREFERENCED_PARAMETER(mode);

    const int smallIcon = GetSystemMetrics(SM_CXSMICON);
    HICON icon = static_cast<HICON>(LoadImageW(
        m_hInstance,
        MAKEINTRESOURCEW(IDI_KEYROIME),
        IMAGE_ICON,
        smallIcon,
        smallIcon,
        LR_DEFAULTCOLOR));
    if (icon) {
        return icon;
    }

    return static_cast<HICON>(LoadImageW(
        m_hInstance,
        MAKEINTRESOURCEW(IDI_KEYROIME),
        IMAGE_ICON,
        16,
        16,
        LR_DEFAULTCOLOR));
}

void SystemTray::ClearMenuBitmaps() const
{
    for (HBITMAP bitmap : m_menuBitmaps) {
        if (bitmap) {
            DeleteObject(bitmap);
        }
    }
    m_menuBitmaps.clear();
}

HBITMAP SystemTray::CreateModeMenuBitmap(int iconSize) const
{
    HDC screenDc = GetDC(nullptr);
    if (!screenDc) {
        return nullptr;
    }

    HDC memoryDc = CreateCompatibleDC(screenDc);
    HBITMAP bitmap = CreateCompatibleBitmap(screenDc, iconSize, iconSize);
    if (!memoryDc || !bitmap) {
        if (bitmap) {
            DeleteObject(bitmap);
        }
        if (memoryDc) {
            DeleteDC(memoryDc);
        }
        ReleaseDC(nullptr, screenDc);
        return nullptr;
    }

    HBITMAP previousBitmap = static_cast<HBITMAP>(SelectObject(memoryDc, bitmap));
    RECT rect = {0, 0, iconSize, iconSize};
    HBRUSH menuBrush = CreateSolidBrush(GetSysColor(COLOR_MENU));
    FillRect(
        memoryDc,
        &rect,
        menuBrush ? menuBrush : static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)));
    if (menuBrush) {
        DeleteObject(menuBrush);
    }

    HPEN framePen = CreatePen(PS_SOLID, 1, RGB(80, 96, 116));
    HGDIOBJ previousPen = SelectObject(
        memoryDc,
        framePen ? static_cast<HGDIOBJ>(framePen) : GetStockObject(BLACK_PEN));
    HGDIOBJ previousBrush = SelectObject(memoryDc, GetStockObject(NULL_BRUSH));
    RoundRect(memoryDc, 2, 2, iconSize - 2, iconSize - 2, 5, 5);
    SelectObject(memoryDc, previousBrush);
    SelectObject(memoryDc, previousPen);
    if (framePen) {
        DeleteObject(framePen);
    }

    int fontHeight = -MulDiv(11, GetDeviceCaps(screenDc, LOGPIXELSY), 72);
    HFONT font = CreateFontW(
        fontHeight,
        0,
        0,
        0,
        FW_SEMIBOLD,
        FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        L"Yu Gothic UI");
    HGDIOBJ previousFont = font ? SelectObject(memoryDc, font) : nullptr;
    SetBkMode(memoryDc, TRANSPARENT);
    SetTextColor(memoryDc, GetSysColor(COLOR_MENUTEXT));
    RECT textRect = {1, 0, iconSize - 1, iconSize - 1};
    const wchar_t* label = m_inputMode == InputMode::English ? L"A" : L"あ";
    DrawTextW(memoryDc, label, -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    if (previousFont) {
        SelectObject(memoryDc, previousFont);
    }
    if (font) {
        DeleteObject(font);
    }

    SelectObject(memoryDc, previousBitmap);
    DeleteDC(memoryDc);
    ReleaseDC(nullptr, screenDc);
    return bitmap;
}

HBITMAP SystemTray::CreateRedDotMenuBitmap(int iconSize) const
{
    HDC screenDc = GetDC(nullptr);
    if (!screenDc) {
        return nullptr;
    }

    HDC memoryDc = CreateCompatibleDC(screenDc);
    HBITMAP bitmap = CreateCompatibleBitmap(screenDc, iconSize, iconSize);
    if (!memoryDc || !bitmap) {
        if (bitmap) {
            DeleteObject(bitmap);
        }
        if (memoryDc) {
            DeleteDC(memoryDc);
        }
        ReleaseDC(nullptr, screenDc);
        return nullptr;
    }

    HBITMAP previousBitmap = static_cast<HBITMAP>(SelectObject(memoryDc, bitmap));
    RECT rect = {0, 0, iconSize, iconSize};
    HBRUSH menuBrush = CreateSolidBrush(GetSysColor(COLOR_MENU));
    FillRect(
        memoryDc,
        &rect,
        menuBrush ? menuBrush : static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)));
    if (menuBrush) {
        DeleteObject(menuBrush);
    }

    int dotSize = iconSize >= 20 ? 10 : iconSize / 2;
    int offset = (iconSize - dotSize) / 2;
    HBRUSH dotBrush = CreateSolidBrush(RGB(220, 38, 38));
    HPEN dotPen = CreatePen(PS_SOLID, 1, RGB(185, 28, 28));
    HGDIOBJ previousBrush = SelectObject(
        memoryDc,
        dotBrush ? static_cast<HGDIOBJ>(dotBrush) : GetStockObject(BLACK_BRUSH));
    HGDIOBJ previousPen = SelectObject(
        memoryDc,
        dotPen ? static_cast<HGDIOBJ>(dotPen) : GetStockObject(BLACK_PEN));
    Ellipse(memoryDc, offset, offset, offset + dotSize, offset + dotSize);
    SelectObject(memoryDc, previousPen);
    SelectObject(memoryDc, previousBrush);
    if (dotPen) {
        DeleteObject(dotPen);
    }
    if (dotBrush) {
        DeleteObject(dotBrush);
    }

    SelectObject(memoryDc, previousBitmap);
    DeleteDC(memoryDc);
    ReleaseDC(nullptr, screenDc);
    return bitmap;
}

void SystemTray::SetMenuItemBitmap(HMENU menu, UINT item, bool byPosition, HBITMAP bitmap) const
{
    if (!bitmap) {
        return;
    }

    const UINT flags = byPosition ? MF_BYPOSITION : MF_BYCOMMAND;
    if (SetMenuItemBitmaps(menu, item, flags, bitmap, bitmap)) {
        m_menuBitmaps.push_back(bitmap);
    } else {
        DeleteObject(bitmap);
    }
}

void SystemTray::SetMenuItemModeIcon(HMENU menu, UINT item, bool byPosition) const
{
    constexpr int kMenuIconSize = 24;
    SetMenuItemBitmap(menu, item, byPosition, CreateModeMenuBitmap(kMenuIconSize));
}

void SystemTray::SetMenuItemRedDot(HMENU menu, UINT item, bool byPosition) const
{
    constexpr int kMenuIconSize = 24;
    SetMenuItemBitmap(menu, item, byPosition, CreateRedDotMenuBitmap(kMenuIconSize));
}

void SystemTray::OnTrayMessage(WPARAM, LPARAM lParam)
{
    switch (LOWORD(lParam)) {
        case NIN_SELECT:
        case WM_LBUTTONUP:
            OnLeftClick();
            break;
        case WM_CONTEXTMENU:
        case WM_RBUTTONUP:
            OnRightClick();
            break;
        default:
            break;
    }
}

void SystemTray::OnLeftClick()
{
    SetInputMode(m_inputMode == InputMode::English ? InputMode::Hiragana : InputMode::English);
}

void SystemTray::OnRightClick()
{
    POINT point = {};
    if (!GetPhysicalCursorPos(&point) && !GetCursorPos(&point)) {
        return;
    }

    RefreshUpdateReminder();
    HMENU menu = CreateContextMenu();
    if (!menu) {
        return;
    }

    SetForegroundWindow(m_messageWindow);
    UINT commandId = TrackPopupMenu(
        menu,
        TPM_RIGHTBUTTON | TPM_RETURNCMD | TPM_NONOTIFY,
        point.x,
        point.y,
        0,
        m_messageWindow,
        nullptr);
    if (commandId != 0) {
        OnMenuCommand(commandId);
    }
    DestroyMenu(menu);
    ClearMenuBitmaps();
    PostMessageW(m_messageWindow, WM_NULL, 0, 0);
}

HMENU SystemTray::CreateContextMenu() const
{
    ClearMenuBitmaps();

    HMENU root = CreatePopupMenu();
    HMENU inputMode = CreatePopupMenu();
    HMENU charWidth = CreatePopupMenu();
    HMENU keyboardLayout = CreatePopupMenu();
    HMENU punctuation = CreatePopupMenu();
    HMENU help = CreatePopupMenu();
    if (!root || !inputMode || !charWidth || !keyboardLayout || !punctuation || !help) {
        if (inputMode) DestroyMenu(inputMode);
        if (charWidth) DestroyMenu(charWidth);
        if (keyboardLayout) DestroyMenu(keyboardLayout);
        if (punctuation) DestroyMenu(punctuation);
        if (help) DestroyMenu(help);
        if (root) DestroyMenu(root);
        return nullptr;
    }

    bool showAnsiShortcuts = m_keyboardLayout == TrayKeyboardLayout::Ansi;
    AppendMenuW(
        inputMode,
        MF_STRING,
        ID_MENU_HIRAGANA,
        showAnsiShortcuts ? L"ひらがな\t[Alt + ~]" : L"ひらがな");
    AppendMenuW(inputMode, MF_STRING, ID_MENU_KATAKANA, L"カタカナ");
    AppendMenuW(
        inputMode,
        MF_STRING,
        ID_MENU_ENGLISH,
        showAnsiShortcuts ? L"英語\t[Alt + ~]" : L"英語");
    CheckMenuRadioItem(
        inputMode,
        ID_MENU_HIRAGANA,
        ID_MENU_ENGLISH,
        m_inputMode == InputMode::Hiragana ? ID_MENU_HIRAGANA :
            (m_inputMode == InputMode::Katakana ? ID_MENU_KATAKANA : ID_MENU_ENGLISH),
        MF_BYCOMMAND);

    AppendMenuW(
        charWidth,
        MF_STRING,
        ID_MENU_HALFWIDTH,
        showAnsiShortcuts ? L"半角\t[Shift + Caps]" : L"半角");
    AppendMenuW(
        charWidth,
        MF_STRING,
        ID_MENU_FULLWIDTH,
        showAnsiShortcuts ? L"全角\t[Shift + Caps]" : L"全角");
    CheckMenuRadioItem(
        charWidth,
        ID_MENU_HALFWIDTH,
        ID_MENU_FULLWIDTH,
        m_charWidth == CharWidth::HalfWidth ? ID_MENU_HALFWIDTH : ID_MENU_FULLWIDTH,
        MF_BYCOMMAND);

    AppendMenuW(keyboardLayout, MF_STRING, ID_MENU_JIS, L"JIS配列\t[Alt + ;]");
    AppendMenuW(keyboardLayout, MF_STRING, ID_MENU_ANSI, L"ANSI配列\t[Alt + ;]");
    CheckMenuRadioItem(
        keyboardLayout,
        ID_MENU_JIS,
        ID_MENU_ANSI,
        m_keyboardLayout == TrayKeyboardLayout::Jis ? ID_MENU_JIS : ID_MENU_ANSI,
        MF_BYCOMMAND);

    AppendMenuW(
        punctuation,
        MF_STRING,
        ID_MENU_PUNCTUATION_JAPANESE,
        L"、 。 ・ (和文)\t[Shift]");
    AppendMenuW(
        punctuation,
        MF_STRING,
        ID_MENU_PUNCTUATION_WESTERN,
        L", . / (欧文)\t[Shift]");
    CheckMenuRadioItem(
        punctuation,
        ID_MENU_PUNCTUATION_JAPANESE,
        ID_MENU_PUNCTUATION_WESTERN,
        m_punctuationStyle == PunctuationStyle::Japanese
            ? ID_MENU_PUNCTUATION_JAPANESE
            : ID_MENU_PUNCTUATION_WESTERN,
        MF_BYCOMMAND);

    std::wstring updateText = FormatUpdateMenuText();
    AppendMenuW(help, MF_STRING, ID_MENU_UPDATE_CHECK, updateText.c_str());
    AppendMenuW(help, MF_STRING, ID_MENU_ABOUT, L"KeyroIME について");

    std::wstring helpText = FormatHelpMenuText();
    AppendMenuW(root, MF_POPUP, reinterpret_cast<UINT_PTR>(inputMode), L"入力モード設定");
    AppendMenuW(root, MF_POPUP, reinterpret_cast<UINT_PTR>(charWidth), L"文字幅設定");
    AppendMenuW(root, MF_POPUP, reinterpret_cast<UINT_PTR>(keyboardLayout), L"キーボード配列設定");
    AppendMenuW(root, MF_POPUP, reinterpret_cast<UINT_PTR>(punctuation), L"句読点設定");
    AppendMenuW(root, MF_STRING | MF_DISABLED | MF_GRAYED, 0, L"拡張機能（v1.0では利用できません）");
    AppendMenuW(root, MF_POPUP, reinterpret_cast<UINT_PTR>(help), helpText.c_str());
    SetMenuItemModeIcon(root, 0, true);
    if (m_updateBadgeVisible) {
        SetMenuItemRedDot(root, 5, true);
        SetMenuItemRedDot(help, ID_MENU_UPDATE_CHECK, false);
    }
    return root;
}

void SystemTray::OnMenuCommand(UINT commandId)
{
    switch (commandId) {
        case ID_MENU_HIRAGANA: SetInputMode(InputMode::Hiragana); break;
        case ID_MENU_KATAKANA: SetInputMode(InputMode::Katakana); break;
        case ID_MENU_ENGLISH: SetInputMode(InputMode::English); break;
        case ID_MENU_HALFWIDTH: SetCharWidth(CharWidth::HalfWidth); break;
        case ID_MENU_FULLWIDTH: SetCharWidth(CharWidth::FullWidth); break;
        case ID_MENU_JIS: SetKeyboardLayout(TrayKeyboardLayout::Jis); break;
        case ID_MENU_ANSI: SetKeyboardLayout(TrayKeyboardLayout::Ansi); break;
        case ID_MENU_PUNCTUATION_JAPANESE:
            SetPunctuationStyle(PunctuationStyle::Japanese);
            break;
        case ID_MENU_PUNCTUATION_WESTERN:
            SetPunctuationStyle(PunctuationStyle::Western);
            break;
        case ID_MENU_UPDATE_CHECK: HandleUpdateCheckCommand(); break;
        case ID_MENU_ABOUT: ShowAboutDialog(); break;
        default: break;
    }
}

void SystemTray::ShowNotification(const std::wstring& title, const std::wstring& message)
{
    if (!m_isVisible) {
        return;
    }
    m_nid.uFlags = NIF_INFO | NIF_GUID;
    StringCchCopyW(m_nid.szInfoTitle, ARRAYSIZE(m_nid.szInfoTitle), title.c_str());
    StringCchCopyW(m_nid.szInfo, ARRAYSIZE(m_nid.szInfo), message.c_str());
    m_nid.dwInfoFlags = NIIF_INFO;
    Shell_NotifyIconW(NIM_MODIFY, &m_nid);
    m_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_GUID;
}

TrayKeyboardLayout SystemTray::DetectPhysicalKeyboardLayout()
{
    return m_keyboardLayout;
}

void SystemTray::ShowAboutDialog()
{
    if (!RegisterInfoWindowClass(m_hInstance, kAboutWindowClassName, AboutWindowProc)) {
        return;
    }

    HWND window = CreateWindowExW(
        WS_EX_APPWINDOW,
        kAboutWindowClassName,
        L"KeyroIME について",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        ScaleForWindow(m_messageWindow, 640),
        ScaleForWindow(m_messageWindow, 500),
        m_messageWindow,
        nullptr,
        m_hInstance,
        nullptr);
    if (!window) {
        return;
    }
    CenterWindowOnOwner(window, m_messageWindow);
    ShowWindow(window, SW_SHOWNORMAL);
    UpdateWindow(window);
}

void SystemTray::ShowLicenseWindow()
{
    ShowLicenseWindowFromOwner(m_messageWindow);
}

void SystemTray::OpenUrl(const wchar_t* url) const
{
    if (!url || url[0] == L'\0') {
        return;
    }

    ShellExecuteW(
        m_hParent ? m_hParent : m_messageWindow,
        L"open",
        url,
        nullptr,
        nullptr,
        SW_SHOWNORMAL);
}

void SystemTray::HandleUpdateCheckCommand()
{
    ResetUpdateReminder();
    MessageBoxW(
        m_hParent ? m_hParent : m_messageWindow,
        L"KeyroIME OpenCore v" KEYROIME_PRODUCT_VERSION_W L"\n\n"
        L"現在のバージョンではオンライン更新確認を行いません。\n"
        L"2週間後に再度お知らせします。",
        L"アップデートを確認",
        MB_OK | MB_ICONINFORMATION);
}

void SystemTray::PublishSettings()
{
    InputSettingsSnapshot snapshot;
    snapshot.inputMode = m_inputMode;
    snapshot.charWidth = m_charWidth;
    snapshot.keyboardLayout = m_keyboardLayout;
    snapshot.punctuationStyle = m_punctuationStyle;
    snapshot.englishCase = m_englishCase;
    if (!m_sharedSettings.Write(snapshot)) {
        m_sharedSettings.Close();
        if (!m_sharedSettings.Initialize(true) || !m_sharedSettings.Write(snapshot)) {
            return;
        }
    }
    PublishSettingsToService();
    ShowCurrentStatus();
}

void SystemTray::PublishSettingsToService()
{
    if (!m_pipeStarted) {
        return;
    }
    m_pipeClient.UpdateSettingsAsync(
        static_cast<int>(m_inputMode),
        static_cast<int>(m_charWidth),
        static_cast<int>(m_keyboardLayout),
        static_cast<int>(m_punctuationStyle),
        static_cast<int>(m_englishCase));
}

void SystemTray::RestoreTrayIcon()
{
    m_isVisible = false;
    Show();
}

void SystemTray::OnTimer(UINT_PTR timerId)
{
    if (timerId == TIMER_SETTINGS_SYNC) {
        SyncSettingsFromShared();
    } else if (timerId == TIMER_ACTIVE_TIP) {
        RefreshActiveTipVisibility();
    }
}

void SystemTray::SyncSettingsFromShared()
{
    if (!m_sharedSettings.IsInitialized()) {
        return;
    }

    InputSettingsSnapshot current;
    current.inputMode = m_inputMode;
    current.charWidth = m_charWidth;
    current.keyboardLayout = m_keyboardLayout;
    current.punctuationStyle = m_punctuationStyle;
    current.englishCase = m_englishCase;

    InputSettingsSnapshot snapshot = m_sharedSettings.Read();
    if (SettingsEqual(current, snapshot)) {
        return;
    }

    bool iconChanged = snapshot.inputMode != m_inputMode;
    m_inputMode = snapshot.inputMode;
    m_charWidth = snapshot.charWidth;
    m_keyboardLayout = snapshot.keyboardLayout;
    m_punctuationStyle = snapshot.punctuationStyle;
    m_englishCase = snapshot.englishCase;
    if (iconChanged) {
        UpdateIcon(m_inputMode);
    }
    ShowCurrentStatus();
}

void SystemTray::RefreshActiveTipVisibility()
{
    bool active = true;
    if (!TryGetKeyroImeActive(active) || active == m_keyroImeActive) {
        return;
    }

    m_keyroImeActive = active;
    if (!active) {
        m_statusOsd.Hide();
    }
    Show();
}

void SystemTray::RefreshUpdateReminder()
{
    m_updateBadgeVisible = CurrentUtcFileTime() >= m_nextUpdateReminderUtc;
}

void SystemTray::ResetUpdateReminder()
{
    m_nextUpdateReminderUtc = CurrentUtcFileTime() + kUpdateReminderIntervalUtc;
    m_updateBadgeVisible = false;
    SaveUpdateReminderState();
}

void SystemTray::LoadUpdateReminderState()
{
    ULONGLONG now = CurrentUtcFileTime();
#ifdef KEYRO_TRAY_TESTING
    m_nextUpdateReminderUtc = now + kUpdateReminderIntervalUtc;
    m_updateBadgeVisible = false;
    return;
#else
    HKEY key = nullptr;
    LSTATUS status = RegCreateKeyExW(
        HKEY_CURRENT_USER,
        kRegistrySubkey,
        0,
        nullptr,
        REG_OPTION_NON_VOLATILE,
        KEY_QUERY_VALUE | KEY_SET_VALUE,
        nullptr,
        &key,
        nullptr);
    if (status != ERROR_SUCCESS) {
        m_nextUpdateReminderUtc = now + kUpdateReminderIntervalUtc;
        m_updateBadgeVisible = false;
        return;
    }

    ULONGLONG dueTime = 0;
    DWORD valueType = 0;
    DWORD valueSize = sizeof(dueTime);
    status = RegQueryValueExW(
        key,
        kNextUpdateReminderUtcValue,
        nullptr,
        &valueType,
        reinterpret_cast<BYTE*>(&dueTime),
        &valueSize);
    if (status != ERROR_SUCCESS || valueType != REG_QWORD ||
        valueSize != sizeof(dueTime) || dueTime == 0) {
        dueTime = now + kUpdateReminderIntervalUtc;
        RegSetValueExW(
            key,
            kNextUpdateReminderUtcValue,
            0,
            REG_QWORD,
            reinterpret_cast<const BYTE*>(&dueTime),
            sizeof(dueTime));
    }

    RegCloseKey(key);
    m_nextUpdateReminderUtc = dueTime;
    RefreshUpdateReminder();
#endif
}

void SystemTray::SaveUpdateReminderState() const
{
#ifdef KEYRO_TRAY_TESTING
    return;
#else
    HKEY key = nullptr;
    LSTATUS status = RegCreateKeyExW(
        HKEY_CURRENT_USER,
        kRegistrySubkey,
        0,
        nullptr,
        REG_OPTION_NON_VOLATILE,
        KEY_SET_VALUE,
        nullptr,
        &key,
        nullptr);
    if (status != ERROR_SUCCESS) {
        return;
    }

    RegSetValueExW(
        key,
        kNextUpdateReminderUtcValue,
        0,
        REG_QWORD,
        reinterpret_cast<const BYTE*>(&m_nextUpdateReminderUtc),
        sizeof(m_nextUpdateReminderUtc));
    RegCloseKey(key);
#endif
}

bool SystemTray::TryGetKeyroImeActive(bool& active) const
{
    bool profileManagerAnswered = false;
    if (m_profileManager) {
        TF_INPUTPROCESSORPROFILE profile = {};
        HRESULT result = m_profileManager->GetActiveProfile(
            GUID_TFCAT_TIP_KEYBOARD,
            &profile);
        if (SUCCEEDED(result)) {
            profileManagerAnswered = true;
            active = profile.dwProfileType == TF_PROFILETYPE_INPUTPROCESSOR &&
                IsEqualCLSID(profile.clsid, CLSID_KeyroTextService) &&
                IsEqualGUID(profile.guidProfile, GUID_KeyroProfile);
            if (active) {
                return true;
            }
        }
    }

    // GetActiveProfile can report the tray thread's keyboard profile on some
    // hosts. Query our text service directly as a second source so OSD gating
    // follows the foreground application's active TIP.
    if (m_inputProcessorProfiles) {
        LANGID languageId = 0;
        GUID profileGuid = GUID_NULL;
        HRESULT result = m_inputProcessorProfiles->GetActiveLanguageProfile(
            CLSID_KeyroTextService,
            &languageId,
            &profileGuid);
        if (result == S_OK) {
            active = languageId == 0x0411 && IsEqualGUID(profileGuid, GUID_KeyroProfile);
            return true;
        }
        if (result == S_FALSE) {
            active = false;
            return true;
        }
    }

    if (profileManagerAnswered) {
        active = false;
        return true;
    }
    active = true;
    return false;
}

void SystemTray::ShowCurrentStatus()
{
    m_statusOsd.ShowStatus(FormatCurrentStatus());
}

std::wstring SystemTray::FormatCurrentStatus() const
{
    std::wstring status;
    switch (m_inputMode) {
        case InputMode::Katakana: status = L"カタカナ"; break;
        case InputMode::English:
            status = m_englishCase == EnglishCase::Upper ? L"英語・大文字" : L"英語・小文字";
            break;
        case InputMode::Hiragana:
        default: status = L"ひらがな"; break;
    }

    status.append(m_charWidth == CharWidth::FullWidth ? L"  |  全角" : L"  |  半角");
    status.append(
        m_keyboardLayout == TrayKeyboardLayout::Ansi
            ? L"  |  ANSI (US)"
            : L"  |  JIS");
    status.append(
        m_punctuationStyle == PunctuationStyle::Japanese
            ? L"  |  、。・"
            : L"  |  ,./");
    return status;
}

std::wstring SystemTray::FormatHelpMenuText() const
{
    return L"ヘルプと情報";
}

std::wstring SystemTray::FormatUpdateMenuText() const
{
    return L"アップデートを確認";
}

LRESULT CALLBACK TrayWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_NCCREATE) {
        CREATESTRUCTW* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
    }

    SystemTray* tray = reinterpret_cast<SystemTray*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (tray) {
        if (msg == SystemTray::WM_TRAYICON) {
            tray->OnTrayMessage(wParam, lParam);
            return 0;
        }
        if (msg == WM_COMMAND) {
            tray->OnMenuCommand(LOWORD(wParam));
            return 0;
        }
        if (tray->m_taskbarCreatedMessage != 0 && msg == tray->m_taskbarCreatedMessage) {
            tray->RestoreTrayIcon();
            return 0;
        }
        if (msg == WM_TIMER) {
            tray->OnTimer(static_cast<UINT_PTR>(wParam));
            return 0;
        }
    }

    if (msg == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

} // namespace KeyroIME
