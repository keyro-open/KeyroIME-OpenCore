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
// text_service.cpp

#include "text_service.h"

#include "local_romaji.h"
#include "module_path.h"
#include "../ui/candidate_window.h"

#include <algorithm>
#include <new>
#include <utility>
#include <windows.h>

namespace KeyroIME {

namespace {

constexpr wchar_t kMessageWindowClassName[] = L"KeyroIME_TSF_MessageWindow";
constexpr wchar_t kJapaneseLocaleName[] = L"ja-JP";
const GUID kPreservedAnsiLayoutShortcut = {
    0x21592c48, 0x3449, 0x4a08, {0xa6, 0x7f, 0x42, 0xcc, 0x31, 0x70, 0xa9, 0xe2}
};
const GUID kPreservedJisLayoutShortcut = {
    0xb4bb6744, 0xee07, 0x47de, {0x82, 0x79, 0x58, 0x32, 0xef, 0x9d, 0xa7, 0x74}
};
const GUID kPreservedKanjiKey = {
    0x4e4dd778, 0x969f, 0x41a8, {0x8f, 0xa3, 0x6f, 0xc3, 0x19, 0x67, 0x3c, 0xa1}
};
const GUID kPreservedImeOnKey = {
    0x92ed90eb, 0x9ddb, 0x4062, {0x86, 0x93, 0x33, 0x88, 0xed, 0x72, 0xe8, 0x9d}
};
const GUID kPreservedImeOffKey = {
    0x57ff257a, 0xd731, 0x4e7d, {0xb4, 0x3b, 0x7a, 0xd1, 0xe7, 0x7d, 0x65, 0x16}
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
constexpr TF_PRESERVEDKEY kAnsiLayoutPreservedKey = {VK_OEM_1, TF_MOD_ALT};
constexpr TF_PRESERVEDKEY kJisLayoutPreservedKey = {VK_OEM_PLUS, TF_MOD_ALT};
constexpr TF_PRESERVEDKEY kKanjiPreservedKey = {VK_KANJI, 0};
constexpr TF_PRESERVEDKEY kImeOnPreservedKey = {VK_IME_ON, 0};
constexpr TF_PRESERVEDKEY kImeOffPreservedKey = {VK_IME_OFF, 0};
constexpr TF_PRESERVEDKEY kKanaPreservedKey = {VK_KANA, 0};
constexpr TF_PRESERVEDKEY kConvertPreservedKey = {VK_CONVERT, 0};
constexpr TF_PRESERVEDKEY kNonConvertPreservedKey = {VK_NONCONVERT, 0};

void ClearSystemCapsLockToggle()
{
    BYTE keyboardState[256] = {};
    if (GetKeyboardState(keyboardState)) {
        keyboardState[VK_CAPITAL] &= 0x80;
        SetKeyboardState(keyboardState);
    }
}

template <typename T>
void SafeRelease(T*& ptr)
{
    if (ptr) {
        ptr->Release();
        ptr = nullptr;
    }
}

std::wstring StripCandidateTag(const std::wstring& candidate)
{
    if (candidate.empty() || candidate.front() != L'[') {
        return candidate;
    }

    size_t closingBracket = candidate.find(L']');
    if (closingBracket == std::wstring::npos) {
        return candidate;
    }

    size_t contentStart = closingBracket + 1;
    while (contentStart < candidate.size() && candidate[contentStart] == L' ') {
        ++contentStart;
    }
    return candidate.substr(contentStart);
}

std::string WideToUtf8(const std::wstring& value)
{
    if (value.empty()) {
        return std::string();
    }
    int length = WideCharToMultiByte(
        CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    if (length <= 0) {
        return std::string();
    }
    std::string result(static_cast<size_t>(length), '\0');
    WideCharToMultiByte(
        CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), length, nullptr, nullptr);
    return result;
}

std::wstring MapJapaneseText(const std::wstring& text, DWORD flags)
{
    if (text.empty() || flags == 0) {
        return text;
    }

    int length = LCMapStringEx(
        kJapaneseLocaleName,
        flags,
        text.data(),
        static_cast<int>(text.size()),
        nullptr,
        0,
        nullptr,
        nullptr,
        0);
    if (length <= 0) {
        return text;
    }

    std::wstring result(static_cast<size_t>(length), L'\0');
    if (LCMapStringEx(
            kJapaneseLocaleName,
            flags,
            text.data(),
            static_cast<int>(text.size()),
            result.data(),
            length,
            nullptr,
            nullptr,
            0) <= 0) {
        return text;
    }
    return result;
}

HRESULT SetSelectionToRangeEnd(
    TfEditCookie editCookie,
    ITfContext* context,
    ITfRange* textRange)
{
    if (!context || !textRange) {
        return E_INVALIDARG;
    }

    ITfRange* cursorRange = nullptr;
    HRESULT hr = textRange->Clone(&cursorRange);
    if (FAILED(hr) || !cursorRange) {
        return FAILED(hr) ? hr : E_FAIL;
    }

    ITfRangeACP* acpRange = nullptr;
    hr = cursorRange->QueryInterface(
        IID_ITfRangeACP,
        reinterpret_cast<void**>(&acpRange));
    if (SUCCEEDED(hr) && acpRange) {
        LONG acpStart = 0;
        LONG acpLength = 0;
        hr = acpRange->GetExtent(&acpStart, &acpLength);
        if (SUCCEEDED(hr)) {
            hr = acpRange->SetExtent(acpStart + acpLength, 0);
        }
        acpRange->Release();
    } else {
        hr = cursorRange->Collapse(editCookie, TF_ANCHOR_END);
    }

    if (SUCCEEDED(hr)) {
        TF_SELECTION selection = {};
        selection.range = cursorRange;
        selection.style.ase = TF_AE_END;
        selection.style.fInterimChar = FALSE;
        hr = context->SetSelection(editCookie, 1, &selection);
    }

    cursorRange->Release();
    return hr;
}

class EditSessionBase : public ITfEditSession {
public:
    EditSessionBase(KeyroTextService* service, ITfContext* context)
        : m_refCount(1)
        , m_service(service)
        , m_context(context)
    {
        if (m_service) {
            m_service->AddRef();
        }
        if (m_context) {
            m_context->AddRef();
        }
    }

    ~EditSessionBase()
    {
        if (m_context) {
            m_context->Release();
            m_context = nullptr;
        }
        if (m_service) {
            m_service->Release();
            m_service = nullptr;
        }
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override
    {
        if (!ppvObject) {
            return E_POINTER;
        }

        *ppvObject = nullptr;

        if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfEditSession)) {
            *ppvObject = static_cast<ITfEditSession*>(this);
            AddRef();
            return S_OK;
        }

        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override
    {
        return static_cast<ULONG>(InterlockedIncrement(&m_refCount));
    }

    ULONG STDMETHODCALLTYPE Release() override
    {
        ULONG refCount = static_cast<ULONG>(InterlockedDecrement(&m_refCount));
        if (refCount == 0) {
            delete this;
        }
        return refCount;
    }

protected:
    volatile LONG m_refCount;
    KeyroTextService* m_service;
    ITfContext* m_context;
};

} // namespace

class CompositionEditSession final : public EditSessionBase {
public:
    CompositionEditSession(KeyroTextService* service, ITfContext* context, std::wstring text)
        : EditSessionBase(service, context)
        , m_text(std::move(text))
    {
    }

    HRESULT STDMETHODCALLTYPE DoEditSession(TfEditCookie editCookie) override
    {
        return m_service ? m_service->UpdateCompositionInSession(editCookie, m_context, m_text) : E_FAIL;
    }

private:
    std::wstring m_text;
};

class CommitEditSession final : public EditSessionBase {
public:
    CommitEditSession(KeyroTextService* service, ITfContext* context, std::wstring text)
        : EditSessionBase(service, context)
        , m_text(std::move(text))
    {
    }

    HRESULT STDMETHODCALLTYPE DoEditSession(TfEditCookie editCookie) override
    {
        return m_service ? m_service->CommitTextInSession(editCookie, m_context, m_text) : E_FAIL;
    }

private:
    std::wstring m_text;
};

class CandidateWindowEditSession final : public EditSessionBase {
public:
    CandidateWindowEditSession(
        KeyroTextService* service,
        ITfContext* context,
        std::vector<std::wstring> candidates)
        : EditSessionBase(service, context)
        , m_candidates(std::move(candidates))
    {
    }

    HRESULT STDMETHODCALLTYPE DoEditSession(TfEditCookie editCookie) override
    {
        return m_service
            ? m_service->UpdateCandidateWindowInSession(editCookie, m_context, m_candidates)
            : E_FAIL;
    }

private:
    std::vector<std::wstring> m_candidates;
};

struct KeyroTextService::PendingCandidates {
    KeyroTextService* service;
    ITfContext* context;
    std::string query;
    uint32_t page;
    uint16_t totalPages;
    std::vector<std::wstring> candidates;
    bool failed;
};

KeyroTextService::KeyroTextService()
    : m_refCount(1)
    , m_threadMgr(nullptr)
    , m_clientId(TF_CLIENTID_NULL)
    , m_keyEventSinkAdvised(false)
    , m_ansiLayoutShortcutPreserved(false)
    , m_jisLayoutShortcutPreserved(false)
    , m_kanjiKeyPreserved(false)
    , m_imeOnKeyPreserved(false)
    , m_imeOffKeyPreserved(false)
    , m_kanaKeyPreserved(false)
    , m_convertKeyPreserved(false)
    , m_nonConvertKeyPreserved(false)
    , m_composition(nullptr)
    , m_messageWindow(nullptr)
    , m_tsfThreadId(0)
    , m_currentPage(0)
    , m_totalPages(1)
    , m_highlightIndex(0)
    , m_hasNavigated(false)
    , m_shiftTracking(false)
    , m_shiftChordUsed(false)
    , m_altTracking(false)
    , m_pipeStarted(false)
    , m_backendHealthy(false)
    , m_candidateWindow(nullptr)
{
    DllAddRef();
}

KeyroTextService::~KeyroTextService()
{
    Deactivate();
    DllRelease();
}

HRESULT STDMETHODCALLTYPE KeyroTextService::QueryInterface(REFIID riid, void** ppvObject)
{
    if (!ppvObject) {
        return E_POINTER;
    }

    *ppvObject = nullptr;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfTextInputProcessor)) {
        *ppvObject = static_cast<ITfTextInputProcessor*>(this);
    } else if (IsEqualIID(riid, IID_ITfKeyEventSink)) {
        *ppvObject = static_cast<ITfKeyEventSink*>(this);
    } else if (IsEqualIID(riid, IID_ITfCompositionSink)) {
        *ppvObject = static_cast<ITfCompositionSink*>(this);
    } else {
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE KeyroTextService::AddRef()
{
    return static_cast<ULONG>(InterlockedIncrement(&m_refCount));
}

ULONG STDMETHODCALLTYPE KeyroTextService::Release()
{
    ULONG refCount = static_cast<ULONG>(InterlockedDecrement(&m_refCount));
    if (refCount == 0) {
        delete this;
    }
    return refCount;
}

HRESULT STDMETHODCALLTYPE KeyroTextService::Activate(ITfThreadMgr* pThreadMgr, TfClientId clientId)
{
    // TSF activation is a host-process boundary. Optional UI/IPC failures must
    // never reject activation or let a C++ exception escape through COM.
    try {
        if (!pThreadMgr || m_threadMgr) {
            return S_OK;
        }

        m_clientId = clientId;
        m_threadMgr = pThreadMgr;
        m_threadMgr->AddRef();
        m_tsfThreadId = GetCurrentThreadId();
        if (!m_sharedSettings.Initialize(true)) {
            m_sharedSettings.Initialize(false);
        }
        m_runtimeSettings = m_sharedSettings.Read();

        HMODULE moduleHandle = DllModuleHandle();
        CandidateWindow* candidateWindow = new (std::nothrow) CandidateWindow();
        if (candidateWindow && moduleHandle && candidateWindow->Initialize(moduleHandle)) {
            m_candidateWindow = candidateWindow;
        } else {
            delete candidateWindow;
        }

        if (FAILED(CreateMessageWindow())) {
            m_messageWindow = nullptr;
        }

        // A failure here leaves the TIP resident but inert instead of causing
        // Windows to immediately switch back to another input profile.
        AdviseKeyEventSink(pThreadMgr);

        if (m_messageWindow) {
            m_pipeStarted = m_pipeClient.Start();
            m_backendHealthy = m_pipeStarted;
            if (m_pipeStarted) {
                PublishInputSettings(CurrentInputSettings());
            }
        }
    } catch (...) {
        // Local composition remains available when optional initialization
        // fails. Activate must always succeed for host stability.
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE KeyroTextService::Deactivate()
{
    try {
        if (m_pipeStarted) {
            m_pipeClient.Stop();
            m_pipeStarted = false;
        }
    } catch (...) {
    }

    try {
        UnadviseKeyEventSink();
        DestroyMessageWindow();
        HideCandidateWindow();

        delete m_candidateWindow;
        m_candidateWindow = nullptr;

        SafeRelease(m_composition);

        if (m_threadMgr) {
            m_threadMgr->Release();
            m_threadMgr = nullptr;
        }

        m_clientId = TF_CLIENTID_NULL;
        m_tsfThreadId = 0;
        ResetBuffer();
        m_shiftTracking = false;
        m_shiftChordUsed = false;
        m_altTracking = false;
        m_backendHealthy = false;
        m_sharedSettings.Close();
    } catch (...) {
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE KeyroTextService::OnSetFocus(BOOL foreground)
{
    if (!foreground) {
        m_shiftTracking = false;
        m_shiftChordUsed = false;
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE KeyroTextService::OnTestKeyDown(
    ITfContext*,
    WPARAM wParam,
    LPARAM lParam,
    BOOL* eaten)
{
    if (!eaten) {
        return E_POINTER;
    }

    if (wParam == VK_MENU) {
        m_altTracking = true;
    }

    if (m_shiftTracking && wParam != VK_SHIFT) {
        m_shiftChordUsed = true;
    }
    *eaten = IsHandledKey(wParam, lParam) ? TRUE : FALSE;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE KeyroTextService::OnKeyDown(
    ITfContext* context,
    WPARAM wParam,
    LPARAM lParam,
    BOOL* eaten)
{
    if (!eaten) {
        return E_POINTER;
    }

    *eaten = FALSE;
    try {
        if (!context) {
            return S_OK;
        }
        if (wParam == VK_MENU) {
            m_altTracking = true;
        }
        if (wParam == VK_SHIFT) {
            m_shiftTracking = true;
            m_shiftChordUsed = false;
            *eaten = TRUE;
            return S_OK;
        }
        if (!IsHandledKey(wParam, lParam)) {
            return S_OK;
        }
        if (m_shiftTracking) {
            m_shiftChordUsed = true;
        }

        InputSettingsSnapshot settings = CurrentInputSettings();
        if (HandleSettingsShortcut(context, wParam, lParam, settings)) {
            *eaten = TRUE;
            return S_OK;
        }

        m_keyboardLayoutMapper.SetLayout(
            settings.keyboardLayout == SharedKeyboardLayout::Ansi ? LAYOUT_US : LAYOUT_JIS);
        WPARAM translatedKey = wParam;
        BYTE scanCode = 0;
        m_keyboardLayoutMapper.TranslateKeyLayout(wParam, translatedKey, scanCode);
        wParam = translatedKey;

        bool shiftPressed = m_shiftTracking ||
            (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
        if (shiftPressed && wParam >= L'A' && wParam <= L'Z') {
            std::wstring commitText;
            if (!m_inputBuffer.empty()) {
                commitText = LocalFallbackFor(m_inputBuffer);
                ResetBuffer();
                HideCandidateWindow();
            }
            commitText.append(BuildDirectEnglishKey(wParam, lParam));
            RequestCommit(context, commitText);
            *eaten = TRUE;
            return S_OK;
        }

        if (settings.inputMode == SharedInputMode::English &&
            ((wParam >= L'A' && wParam <= L'Z') ||
             IsDirectPrintableKey(wParam))) {
            RequestCommit(context, BuildDirectEnglishKey(wParam, lParam));
            *eaten = TRUE;
            return S_OK;
        }

        if (m_inputBuffer.empty() && IsDirectPrintableKey(wParam)) {
            std::wstring directText = BuildDirectEnglishKey(wParam, lParam);
            if (!directText.empty()) {
                RequestCommit(context, directText);
                *eaten = TRUE;
            }
            return S_OK;
        }

        if (m_inputBuffer.empty() && wParam == VK_SPACE) {
            std::wstring directText = BuildDirectEnglishKey(wParam, lParam);
            if (directText.empty()) {
                directText = ApplyCurrentInputSettings(L" ", false);
            }
            RequestCommit(context, directText);
            *eaten = TRUE;
            return S_OK;
        }

        if (wParam == VK_BACK) {
            if (!m_inputBuffer.empty()) {
                RemoveLastRomajiUnit(m_inputBuffer);
                m_currentPage = 0;
                m_highlightIndex = 0;
                m_hasNavigated = false;
                m_currentCandidates.clear();
                HideCandidateWindow();
                if (m_inputBuffer.empty()) {
                    RequestCompositionCancel(context);
                } else {
                    RequestCompositionUpdate(context);
                    RequestCandidatesAsync(context);
                }
                *eaten = TRUE;
            }
            return S_OK;
        }

        if (wParam == VK_ESCAPE) {
            ResetBuffer();
            RequestCompositionCancel(context);
            HideCandidateWindow();
            *eaten = TRUE;
            return S_OK;
        }

        if (!m_inputBuffer.empty() &&
            IsRomajiLongVowelKey(wParam, lParam, shiftPressed)) {
            m_inputBuffer.push_back('-');
            m_currentPage = 0;
            m_totalPages = 1;
            m_highlightIndex = 0;
            m_hasNavigated = false;
            m_currentCandidates.clear();
            HideCandidateWindow();
            RequestCompositionUpdate(context);
            RequestCandidatesAsync(context);
            *eaten = TRUE;
            return S_OK;
        }

        if (!shiftPressed && !m_inputBuffer.empty() &&
            wParam >= L'1' && wParam <= L'5') {
            size_t candidateIndex = static_cast<size_t>(wParam - L'1');
            if (candidateIndex < m_currentCandidates.size()) {
                RecordCandidateSelection(m_currentCandidates[candidateIndex]);
            }
            std::wstring commitText = candidateIndex < m_currentCandidates.size()
                ? StripCandidateTag(m_currentCandidates[candidateIndex])
                : BufferAsWide();
            RequestCommit(context, commitText);
            ResetBuffer();
            HideCandidateWindow();
            *eaten = TRUE;
            return S_OK;
        }

        if (!m_inputBuffer.empty() && (wParam == VK_UP || wParam == VK_DOWN || wParam == VK_TAB)) {
            MoveHighlight(
                wParam == VK_UP || (wParam == VK_TAB && shiftPressed) ? -1 : 1);
            m_hasNavigated = true;
            if (m_candidateWindow) {
                m_candidateWindow->UpdateHighlight(static_cast<int>(m_highlightIndex));
            }
            *eaten = TRUE;
            return S_OK;
        }

        if (!m_inputBuffer.empty() && IsPagePreviousKey(wParam)) {
            if (m_currentPage > 0) {
                --m_currentPage;
            }
            m_highlightIndex = 0;
            m_hasNavigated = true;
            RequestCandidatesAsync(context);
            *eaten = TRUE;
            return S_OK;
        }

        if (!m_inputBuffer.empty() && IsPageNextKey(wParam)) {
            if (m_currentPage + 1 < m_totalPages) {
                ++m_currentPage;
            }
            m_highlightIndex = 0;
            m_hasNavigated = true;
            RequestCandidatesAsync(context);
            *eaten = TRUE;
            return S_OK;
        }

        if (!m_inputBuffer.empty() && IsDirectPrintableKey(wParam)) {
            std::wstring directText = BuildDirectEnglishKey(wParam, lParam);
            if (!directText.empty()) {
                std::wstring commitText = LocalFallbackFor(m_inputBuffer);
                commitText.append(directText);
                RequestCommit(context, commitText);
                ResetBuffer();
                HideCandidateWindow();
                *eaten = TRUE;
            }
            return S_OK;
        }

        if (!m_inputBuffer.empty() &&
            (wParam == VK_RETURN || wParam == VK_SPACE || wParam == VK_RIGHT)) {
            std::wstring commitText;
            if (wParam == VK_RETURN && !m_hasNavigated) {
                commitText = LocalFallbackFor(m_inputBuffer);
            } else {
                if (m_highlightIndex < m_currentCandidates.size()) {
                    RecordCandidateSelection(m_currentCandidates[m_highlightIndex]);
                }
                commitText = SelectedCandidateOrFallback();
            }
            RequestCommit(context, commitText);
            ResetBuffer();
            HideCandidateWindow();
            *eaten = TRUE;
            return S_OK;
        }

        if (AppendKeyToBuffer(wParam)) {
            m_currentPage = 0;
            m_totalPages = 1;
            m_highlightIndex = 0;
            m_hasNavigated = false;
            m_currentCandidates.clear();
            HideCandidateWindow();
            RequestCompositionUpdate(context);
            RequestCandidatesAsync(context);
            *eaten = TRUE;
        }
    } catch (...) {
        // Never propagate exceptions into the host's TSF callback stack.
        *eaten = FALSE;
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE KeyroTextService::OnTestKeyUp(
    ITfContext*,
    WPARAM wParam,
    LPARAM,
    BOOL* eaten)
{
    if (!eaten) {
        return E_POINTER;
    }
    if (wParam == VK_MENU) {
        m_altTracking = false;
    }
    *eaten = (wParam == VK_SHIFT && m_shiftTracking) ||
        IsSettingsShortcutKey(wParam, 0) ? TRUE : FALSE;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE KeyroTextService::OnKeyUp(
    ITfContext*,
    WPARAM wParam,
    LPARAM,
    BOOL* eaten)
{
    if (!eaten) {
        return E_POINTER;
    }
    *eaten = FALSE;
    if (wParam == VK_MENU) {
        m_altTracking = false;
    }
    if (IsSettingsShortcutKey(wParam, 0) && wParam != VK_SHIFT) {
        *eaten = TRUE;
        return S_OK;
    }
    if (wParam == VK_SHIFT && m_shiftTracking) {
        bool togglePunctuation = !m_shiftChordUsed;
        m_shiftTracking = false;
        m_shiftChordUsed = false;
        if (togglePunctuation) {
            InputSettingsSnapshot settings = CurrentInputSettings();
            settings.punctuationStyle =
                settings.punctuationStyle == SharedPunctuationStyle::Japanese
                ? SharedPunctuationStyle::Western
                : SharedPunctuationStyle::Japanese;
            PublishInputSettings(settings);
        }
        *eaten = TRUE;
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE KeyroTextService::OnPreservedKey(
    ITfContext* context,
    REFGUID guid,
    BOOL* eaten)
{
    if (!eaten) {
        return E_POINTER;
    }
    if (!IsEqualGUID(guid, kPreservedAnsiLayoutShortcut) &&
        !IsEqualGUID(guid, kPreservedJisLayoutShortcut) &&
        !IsEqualGUID(guid, kPreservedKanjiKey) &&
        !IsEqualGUID(guid, kPreservedImeOnKey) &&
        !IsEqualGUID(guid, kPreservedImeOffKey) &&
        !IsEqualGUID(guid, kPreservedKanaKey) &&
        !IsEqualGUID(guid, kPreservedConvertKey) &&
        !IsEqualGUID(guid, kPreservedNonConvertKey)) {
        *eaten = FALSE;
        return S_OK;
    }

    WPARAM preservedKey = 0;
    if (IsEqualGUID(guid, kPreservedKanjiKey)) {
        preservedKey = VK_KANJI;
    } else if (IsEqualGUID(guid, kPreservedImeOnKey)) {
        preservedKey = VK_IME_ON;
    } else if (IsEqualGUID(guid, kPreservedImeOffKey)) {
        preservedKey = VK_IME_OFF;
    } else if (IsEqualGUID(guid, kPreservedKanaKey)) {
        preservedKey = VK_KANA;
    } else if (IsEqualGUID(guid, kPreservedConvertKey)) {
        preservedKey = VK_CONVERT;
    } else if (IsEqualGUID(guid, kPreservedNonConvertKey)) {
        preservedKey = VK_NONCONVERT;
    }
    if (preservedKey != 0) {
        *eaten = HandleSettingsShortcut(context, preservedKey, 0, CurrentInputSettings())
            ? TRUE
            : FALSE;
        return S_OK;
    }

    if (context && !m_inputBuffer.empty()) {
        RequestCommit(context, LocalFallbackFor(m_inputBuffer));
        ResetBuffer();
        HideCandidateWindow();
    }
    PublishInputSettings(ToggleKeyboardLayoutSetting(CurrentInputSettings()));
    *eaten = TRUE;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE KeyroTextService::OnCompositionTerminated(
    TfEditCookie,
    ITfComposition* composition)
{
    if (composition && composition == m_composition) {
        SafeRelease(m_composition);
    }
    ResetBuffer();
    HideCandidateWindow();
    return S_OK;
}

HRESULT KeyroTextService::AdviseKeyEventSink(ITfThreadMgr* threadMgr)
{
    if (!threadMgr || m_clientId == TF_CLIENTID_NULL || m_keyEventSinkAdvised) {
        return threadMgr ? S_OK : E_INVALIDARG;
    }

    ITfKeystrokeMgr* keystrokeMgr = nullptr;
    HRESULT hr = threadMgr->QueryInterface(
        IID_ITfKeystrokeMgr,
        reinterpret_cast<void**>(&keystrokeMgr));
    if (FAILED(hr)) {
        return hr;
    }

    hr = keystrokeMgr->AdviseKeyEventSink(
        m_clientId,
        static_cast<ITfKeyEventSink*>(this),
        TRUE);
    if (SUCCEEDED(hr)) {
        m_keyEventSinkAdvised = true;
        static constexpr wchar_t kDescription[] = L"KeyroIME JIS/ANSI layout toggle";
        m_ansiLayoutShortcutPreserved = SUCCEEDED(keystrokeMgr->PreserveKey(
            m_clientId,
            kPreservedAnsiLayoutShortcut,
            &kAnsiLayoutPreservedKey,
            kDescription,
            ARRAYSIZE(kDescription) - 1));
        m_jisLayoutShortcutPreserved = SUCCEEDED(keystrokeMgr->PreserveKey(
            m_clientId,
            kPreservedJisLayoutShortcut,
            &kJisLayoutPreservedKey,
            kDescription,
            ARRAYSIZE(kDescription) - 1));
        m_kanjiKeyPreserved = SUCCEEDED(keystrokeMgr->PreserveKey(
            m_clientId,
            kPreservedKanjiKey,
            &kKanjiPreservedKey,
            kDescription,
            ARRAYSIZE(kDescription) - 1));
        m_imeOnKeyPreserved = SUCCEEDED(keystrokeMgr->PreserveKey(
            m_clientId,
            kPreservedImeOnKey,
            &kImeOnPreservedKey,
            kDescription,
            ARRAYSIZE(kDescription) - 1));
        m_imeOffKeyPreserved = SUCCEEDED(keystrokeMgr->PreserveKey(
            m_clientId,
            kPreservedImeOffKey,
            &kImeOffPreservedKey,
            kDescription,
            ARRAYSIZE(kDescription) - 1));
        m_kanaKeyPreserved = SUCCEEDED(keystrokeMgr->PreserveKey(
            m_clientId,
            kPreservedKanaKey,
            &kKanaPreservedKey,
            kDescription,
            ARRAYSIZE(kDescription) - 1));
        m_convertKeyPreserved = SUCCEEDED(keystrokeMgr->PreserveKey(
            m_clientId,
            kPreservedConvertKey,
            &kConvertPreservedKey,
            kDescription,
            ARRAYSIZE(kDescription) - 1));
        m_nonConvertKeyPreserved = SUCCEEDED(keystrokeMgr->PreserveKey(
            m_clientId,
            kPreservedNonConvertKey,
            &kNonConvertPreservedKey,
            kDescription,
            ARRAYSIZE(kDescription) - 1));
    }
    keystrokeMgr->Release();
    return hr;
}

void KeyroTextService::UnadviseKeyEventSink()
{
    if (!m_threadMgr || !m_keyEventSinkAdvised || m_clientId == TF_CLIENTID_NULL) {
        return;
    }

    ITfKeystrokeMgr* keystrokeMgr = nullptr;
    HRESULT hr = m_threadMgr->QueryInterface(
        IID_ITfKeystrokeMgr,
        reinterpret_cast<void**>(&keystrokeMgr));
    if (SUCCEEDED(hr)) {
        if (m_ansiLayoutShortcutPreserved) {
            keystrokeMgr->UnpreserveKey(
                kPreservedAnsiLayoutShortcut,
                &kAnsiLayoutPreservedKey);
        }
        if (m_jisLayoutShortcutPreserved) {
            keystrokeMgr->UnpreserveKey(
                kPreservedJisLayoutShortcut,
                &kJisLayoutPreservedKey);
        }
        if (m_kanjiKeyPreserved) {
            keystrokeMgr->UnpreserveKey(
                kPreservedKanjiKey,
                &kKanjiPreservedKey);
        }
        if (m_imeOnKeyPreserved) {
            keystrokeMgr->UnpreserveKey(
                kPreservedImeOnKey,
                &kImeOnPreservedKey);
        }
        if (m_imeOffKeyPreserved) {
            keystrokeMgr->UnpreserveKey(
                kPreservedImeOffKey,
                &kImeOffPreservedKey);
        }
        if (m_kanaKeyPreserved) {
            keystrokeMgr->UnpreserveKey(
                kPreservedKanaKey,
                &kKanaPreservedKey);
        }
        if (m_convertKeyPreserved) {
            keystrokeMgr->UnpreserveKey(
                kPreservedConvertKey,
                &kConvertPreservedKey);
        }
        if (m_nonConvertKeyPreserved) {
            keystrokeMgr->UnpreserveKey(
                kPreservedNonConvertKey,
                &kNonConvertPreservedKey);
        }
        keystrokeMgr->UnadviseKeyEventSink(m_clientId);
        keystrokeMgr->Release();
    }

    m_keyEventSinkAdvised = false;
    m_ansiLayoutShortcutPreserved = false;
    m_jisLayoutShortcutPreserved = false;
    m_kanjiKeyPreserved = false;
    m_imeOnKeyPreserved = false;
    m_imeOffKeyPreserved = false;
    m_kanaKeyPreserved = false;
    m_convertKeyPreserved = false;
    m_nonConvertKeyPreserved = false;
}

HRESULT KeyroTextService::CreateMessageWindow()
{
    if (m_messageWindow) {
        return S_OK;
    }

    HINSTANCE instance = DllModuleHandle();
    if (!instance) {
        return E_UNEXPECTED;
    }
    WNDCLASSW windowClass = {};
    windowClass.lpfnWndProc = MessageWindowProc;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = kMessageWindowClassName;
    if (RegisterClassW(&windowClass) == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    m_messageWindow = CreateWindowExW(
        0,
        kMessageWindowClassName,
        L"",
        0,
        0,
        0,
        0,
        0,
        HWND_MESSAGE,
        nullptr,
        instance,
        this);

    if (!m_messageWindow) {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    return S_OK;
}

void KeyroTextService::DestroyMessageWindow()
{
    if (m_messageWindow) {
        DestroyWindow(m_messageWindow);
        m_messageWindow = nullptr;
    }

    HINSTANCE instance = DllModuleHandle();
    if (instance) {
        UnregisterClassW(kMessageWindowClassName, instance);
    }
}

LRESULT CALLBACK KeyroTextService::MessageWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_NCCREATE) {
        CREATESTRUCTW* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
    }

    if (msg == WM_KEYRO_TSF_CANDIDATES_READY) {
        PendingCandidates* pending = reinterpret_cast<PendingCandidates*>(lParam);
        if (pending) {
            if (pending->service &&
                pending->query == pending->service->m_inputBuffer &&
                pending->page == pending->service->m_currentPage) {
                if (pending->failed) {
                    pending->service->ShowLocalCandidatesAndMarkBackend(
                        pending->context,
                        pending->query);
                } else {
                    pending->service->m_backendHealthy = true;
                    pending->service->m_currentCandidates = pending->candidates;
                    pending->service->m_totalPages = std::max<uint16_t>(pending->totalPages, 1);
                    if (pending->service->m_highlightIndex >= pending->candidates.size()) {
                        pending->service->m_highlightIndex = 0;
                    }
                    pending->service->RequestCandidateWindowUpdate(
                        pending->context,
                        pending->candidates);
                }
            }
            if (pending->context) {
                pending->context->Release();
            }
            if (pending->service) {
                pending->service->Release();
            }
            delete pending;
        }
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

bool KeyroTextService::IsHandledKey(WPARAM wParam, LPARAM lParam) const
{
    bool controlPressed = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
    bool altPressed = IsAltPressed(lParam);
    if (controlPressed || (altPressed && !IsSettingsShortcutKey(wParam, lParam))) {
        return false;
    }

    if (IsSettingsShortcutKey(wParam, lParam)) {
        return true;
    }

    if (wParam == VK_SHIFT || wParam == VK_SPACE ||
        (wParam >= L'A' && wParam <= L'Z')) {
        return true;
    }

    if (IsDirectPrintableKey(wParam) &&
        (CurrentInputSettings().inputMode == SharedInputMode::English ||
         m_inputBuffer.empty() ||
         m_shiftTracking ||
         (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0 ||
         (wParam >= L'1' && wParam <= L'5'))) {
        return true;
    }

    if (m_inputBuffer.empty()) {
        return false;
    }

    bool shiftPressed = m_shiftTracking ||
        (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
    if (IsRomajiLongVowelKey(wParam, lParam, shiftPressed)) {
        return true;
    }

    return (wParam >= L'1' && wParam <= L'5') ||
           wParam == VK_BACK ||
           wParam == VK_ESCAPE ||
           wParam == VK_RETURN ||
           wParam == VK_SPACE ||
           wParam == VK_RIGHT ||
           IsNavigationKey(wParam) ||
           IsPagePreviousKey(wParam) ||
           IsPageNextKey(wParam);
}

bool KeyroTextService::AppendKeyToBuffer(WPARAM wParam)
{
    if (wParam >= L'A' && wParam <= L'Z') {
        m_inputBuffer.push_back(static_cast<char>(wParam - L'A' + 'a'));
        return true;
    }

    return false;
}

bool KeyroTextService::IsSettingsShortcutKey(WPARAM wParam, LPARAM lParam) const
{
    bool altPressed = IsAltPressed(lParam);
    return wParam == VK_CAPITAL ||
        IsImeModeToggleKey(wParam) ||
        IsKanaModeKey(wParam) ||
        IsConvertKey(wParam) ||
        IsNonConvertKey(wParam) ||
        (altPressed && wParam == VK_OEM_3) ||
        IsKeyboardLayoutToggleShortcut(wParam, lParam, altPressed);
}

bool KeyroTextService::HandleSettingsShortcut(
    ITfContext* context,
    WPARAM wParam,
    LPARAM lParam,
    InputSettingsSnapshot settings)
{
    bool altPressed = IsAltPressed(lParam);
    bool shiftPressed = m_shiftTracking ||
        (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
    bool handled = false;
    bool settingsChanged = false;
    bool refreshComposition = false;

    if (IsConvertKey(wParam)) {
        if (!m_inputBuffer.empty()) {
            if (!m_currentCandidates.empty()) {
                MoveHighlight(1);
                m_hasNavigated = true;
                if (m_candidateWindow) {
                    m_candidateWindow->UpdateHighlight(static_cast<int>(m_highlightIndex));
                }
                RequestCandidateWindowUpdate(context, m_currentCandidates);
            } else {
                RefreshCompositionAndCandidates(context);
            }
        } else if (settings.inputMode == SharedInputMode::English) {
            settings.inputMode = SharedInputMode::Hiragana;
            PublishInputSettings(settings);
        }
        return true;
    }

    if (IsNonConvertKey(wParam)) {
        settings = CycleKanaWidthSetting(settings);
        handled = true;
        settingsChanged = true;
        refreshComposition = true;
    } else if (wParam == VK_CAPITAL && (shiftPressed || altPressed)) {
        settings = ToggleCharacterWidthSetting(settings);
        handled = true;
        settingsChanged = true;
        refreshComposition = true;
    } else if (wParam == VK_CAPITAL) {
        settings = ToggleCapsLockSetting(settings);
        handled = true;
        settingsChanged = true;
    } else if (IsKeyboardLayoutToggleShortcut(wParam, lParam, altPressed)) {
        settings = ToggleKeyboardLayoutSetting(settings);
        handled = true;
        settingsChanged = true;
    } else if ((altPressed && wParam == VK_OEM_3) || IsImeModeToggleKey(wParam)) {
        if (wParam == VK_IME_ON) {
            settings.inputMode = SharedInputMode::Hiragana;
        } else if (wParam == VK_IME_OFF) {
            settings.inputMode = SharedInputMode::English;
        } else {
            settings.inputMode = settings.inputMode == SharedInputMode::English
                ? SharedInputMode::Hiragana
                : SharedInputMode::English;
        }
        handled = true;
        settingsChanged = true;
    } else if (IsKanaModeKey(wParam)) {
        if (IsExplicitHiraganaModeKey(wParam)) {
            settings.inputMode = SharedInputMode::Hiragana;
        } else if (IsExplicitKatakanaModeKey(wParam)) {
            settings.inputMode = SharedInputMode::Katakana;
            settings.charWidth = SharedCharWidth::FullWidth;
        } else {
            settings.inputMode = settings.inputMode == SharedInputMode::Katakana
                ? SharedInputMode::Hiragana
                : SharedInputMode::Katakana;
        }
        handled = true;
        settingsChanged = true;
        refreshComposition = true;
    }

    if (!handled) {
        return false;
    }

    if (wParam == VK_CAPITAL) {
        ClearSystemCapsLockToggle();
    }
    if (context && !m_inputBuffer.empty()) {
        if (refreshComposition) {
            PublishInputSettings(settings);
            RefreshCompositionAndCandidates(context);
            return true;
        }
        RequestCommit(context, LocalFallbackFor(m_inputBuffer));
        ResetBuffer();
        HideCandidateWindow();
    }
    if (settingsChanged) {
        PublishInputSettings(settings);
    }
    return true;
}

bool KeyroTextService::IsAltPressed(LPARAM lParam) const
{
    constexpr ULONG_PTR kAltContextMask = static_cast<ULONG_PTR>(1) << 29;
    return m_altTracking ||
        (GetAsyncKeyState(VK_MENU) & 0x8000) != 0 ||
        (static_cast<ULONG_PTR>(lParam) & kAltContextMask) != 0;
}

bool KeyroTextService::IsDirectPrintableKey(WPARAM wParam) const
{
    if (wParam >= L'0' && wParam <= L'9') {
        return true;
    }

    switch (wParam) {
        case VK_OEM_1:
        case VK_OEM_PLUS:
        case VK_OEM_COMMA:
        case VK_OEM_MINUS:
        case VK_OEM_PERIOD:
        case VK_OEM_2:
        case VK_OEM_3:
        case VK_OEM_4:
        case VK_OEM_5:
        case VK_OEM_6:
        case VK_OEM_7:
        case VK_OEM_102:
            return true;
        default:
            return false;
    }
}

bool KeyroTextService::IsNavigationKey(WPARAM wParam) const
{
    return wParam == VK_UP || wParam == VK_DOWN || wParam == VK_TAB;
}

bool KeyroTextService::IsPagePreviousKey(WPARAM wParam) const
{
    return wParam == VK_PRIOR || wParam == VK_OEM_COMMA;
}

bool KeyroTextService::IsPageNextKey(WPARAM wParam) const
{
    return wParam == VK_NEXT || wParam == VK_OEM_PERIOD;
}

void KeyroTextService::MoveHighlight(int delta)
{
    size_t count = m_currentCandidates.empty()
        ? static_cast<size_t>(5)
        : m_currentCandidates.size();
    if (count == 0) {
        m_highlightIndex = 0;
        return;
    }

    int next = static_cast<int>(m_highlightIndex) + delta;
    if (next < 0) {
        next = static_cast<int>(count) - 1;
    } else if (next >= static_cast<int>(count)) {
        next = 0;
    }
    m_highlightIndex = static_cast<size_t>(next);
}

void KeyroTextService::ResetBuffer()
{
    m_inputBuffer.clear();
    m_currentCandidates.clear();
    m_currentPage = 0;
    m_totalPages = 1;
    m_highlightIndex = 0;
    m_hasNavigated = false;
}

void KeyroTextService::HideCandidateWindow()
{
    if (m_candidateWindow) {
        m_candidateWindow->Hide();
    }
}

std::wstring KeyroTextService::BufferAsWide() const
{
    bool kanaSource = m_inputBuffer.empty() ||
        (m_inputBuffer.front() != 'q' && m_inputBuffer.front() != 'v');
    return ApplyCurrentInputSettings(
        ConvertRomajiBufferToHiragana(m_inputBuffer),
        kanaSource);
}

std::wstring KeyroTextService::LocalFallbackFor(const std::string& input) const
{
    bool kanaSource = input.empty() || (input.front() != 'q' && input.front() != 'v');
    return ApplyCurrentInputSettings(BuildLocalFallbackCommit(input), kanaSource);
}

std::vector<std::wstring> KeyroTextService::BuildLocalCandidatesFor(const std::string& input) const
{
    std::vector<std::wstring> candidates;
    std::wstring fallback = LocalFallbackFor(input);
    if (!fallback.empty()) {
        candidates.push_back(fallback);
    }
    return candidates;
}

std::wstring KeyroTextService::ApplyCurrentInputSettings(
    const std::wstring& text,
    bool kanaSource) const
{
    InputSettingsSnapshot settings = CurrentInputSettings();
    if (kanaSource && settings.inputMode == SharedInputMode::Katakana) {
        DWORD flags = LCMAP_KATAKANA;
        flags |= settings.charWidth == SharedCharWidth::HalfWidth
            ? LCMAP_HALFWIDTH
            : LCMAP_FULLWIDTH;
        return MapJapaneseText(text, flags);
    }

    if (!kanaSource || settings.inputMode == SharedInputMode::English) {
        return MapJapaneseText(
            text,
            settings.charWidth == SharedCharWidth::FullWidth
                ? LCMAP_FULLWIDTH
                : LCMAP_HALFWIDTH);
    }
    return text;
}

std::wstring KeyroTextService::BuildDirectEnglishKey(WPARAM wParam, LPARAM lParam) const
{
    bool shiftPressed = m_shiftTracking ||
        (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
    bool effectiveShift = shiftPressed;
    InputSettingsSnapshot settings = CurrentInputSettings();
    if (wParam >= L'A' && wParam <= L'Z') {
        effectiveShift = shiftPressed || settings.englishCase == SharedEnglishCase::Upper;
    }

    wchar_t character = 0;
    UINT scanCode = static_cast<UINT>((static_cast<ULONG_PTR>(lParam) >> 16) & 0xFF);
    if (!m_keyboardLayoutMapper.TranslatePrintableKey(
            wParam,
            effectiveShift,
            character,
            scanCode)) {
        return std::wstring();
    }
    if (settings.inputMode != SharedInputMode::English) {
        if (settings.punctuationStyle == SharedPunctuationStyle::Japanese) {
            std::wstring japaneseSymbol = JapaneseStyleSymbolFor(character);
            if (!japaneseSymbol.empty()) {
                return japaneseSymbol;
            }
        } else if (!effectiveShift) {
            if (character == L',' || character == L'.' || character == L'/') {
                return std::wstring(1, character);
            }
        }
    }
    return ApplyCurrentInputSettings(std::wstring(1, character), false);
}

InputSettingsSnapshot KeyroTextService::CurrentInputSettings() const
{
    if (!m_sharedSettings.IsInitialized()) {
        if (!m_sharedSettings.Initialize(true) && !m_sharedSettings.Initialize(false)) {
            return m_runtimeSettings;
        }
    }
    return m_sharedSettings.Read();
}

void KeyroTextService::PublishInputSettings(const InputSettingsSnapshot& snapshot)
{
    m_runtimeSettings = snapshot;
    if (!m_sharedSettings.Write(snapshot)) {
        m_sharedSettings.Close();
        if (m_sharedSettings.Initialize(true)) {
            m_sharedSettings.Write(snapshot);
        }
    }
    if (m_pipeStarted) {
        m_pipeClient.UpdateSettingsAsync(
            static_cast<int>(snapshot.inputMode),
            static_cast<int>(snapshot.charWidth),
            static_cast<int>(snapshot.keyboardLayout),
            static_cast<int>(snapshot.punctuationStyle),
            static_cast<int>(snapshot.englishCase));
    }
}

std::wstring KeyroTextService::SelectedCandidateOrFallback() const
{
    if (m_highlightIndex < m_currentCandidates.size()) {
        return StripCandidateTag(m_currentCandidates[m_highlightIndex]);
    }
    return LocalFallbackFor(m_inputBuffer);
}

void KeyroTextService::RefreshCompositionAndCandidates(ITfContext* context)
{
    if (!context || m_inputBuffer.empty()) {
        return;
    }

    m_currentPage = 0;
    m_totalPages = 1;
    m_highlightIndex = 0;
    m_hasNavigated = false;
    m_currentCandidates = BuildLocalCandidatesFor(m_inputBuffer);
    HideCandidateWindow();
    RequestCompositionUpdate(context);
    if (!m_currentCandidates.empty()) {
        RequestCandidateWindowUpdate(context, m_currentCandidates);
    }
    RequestCandidatesAsync(context);
}

HRESULT KeyroTextService::RequestCompositionUpdate(ITfContext* context)
{
    if (!context) {
        return E_INVALIDARG;
    }

    CompositionEditSession* editSession =
        new (std::nothrow) CompositionEditSession(this, context, BufferAsWide());
    if (!editSession) {
        return E_OUTOFMEMORY;
    }

    HRESULT sessionHr = S_OK;
    HRESULT hr = context->RequestEditSession(
        m_clientId,
        editSession,
        TF_ES_SYNC | TF_ES_READWRITE,
        &sessionHr);
    if (FAILED(hr) || FAILED(sessionHr)) {
        sessionHr = S_OK;
        hr = context->RequestEditSession(
            m_clientId,
            editSession,
            TF_ES_ASYNC | TF_ES_READWRITE,
            &sessionHr);
    }

    editSession->Release();
    return FAILED(hr) ? hr : sessionHr;
}

HRESULT KeyroTextService::RequestCompositionCancel(ITfContext* context)
{
    if (!context) {
        return E_INVALIDARG;
    }

    CompositionEditSession* editSession =
        new (std::nothrow) CompositionEditSession(this, context, std::wstring());
    if (!editSession) {
        return E_OUTOFMEMORY;
    }

    HRESULT sessionHr = S_OK;
    HRESULT hr = context->RequestEditSession(
        m_clientId,
        editSession,
        TF_ES_ASYNC | TF_ES_READWRITE,
        &sessionHr);

    editSession->Release();
    return FAILED(hr) ? hr : sessionHr;
}

HRESULT KeyroTextService::RequestCandidateWindowUpdate(
    ITfContext* context,
    const std::vector<std::wstring>& candidates)
{
    if (!context || candidates.empty()) {
        HideCandidateWindow();
        return S_OK;
    }

    CandidateWindowEditSession* editSession =
        new (std::nothrow) CandidateWindowEditSession(this, context, candidates);
    if (!editSession) {
        return E_OUTOFMEMORY;
    }

    HRESULT sessionHr = S_OK;
    HRESULT hr = context->RequestEditSession(
        m_clientId,
        editSession,
        TF_ES_SYNC | TF_ES_READ,
        &sessionHr);
    if (FAILED(hr) || FAILED(sessionHr)) {
        sessionHr = S_OK;
        hr = context->RequestEditSession(
            m_clientId,
            editSession,
            TF_ES_ASYNC | TF_ES_READ,
            &sessionHr);
        if (SUCCEEDED(hr)) {
            ShowCandidateWindowAtFallbackPosition(candidates);
        }
    }

    editSession->Release();
    return FAILED(hr) ? hr : sessionHr;
}

void KeyroTextService::ShowCandidateWindowAtFallbackPosition(
    const std::vector<std::wstring>& candidates)
{
    if (!m_candidateWindow || candidates.empty()) {
        return;
    }

    POINT caretPoint = {};
    HWND focusWindow = GetFocus();
    if (focusWindow && GetCaretPos(&caretPoint)) {
        ClientToScreen(focusWindow, &caretPoint);
    } else {
        HWND foreground = GetForegroundWindow();
        RECT windowRect = {};
        if (foreground && GetWindowRect(foreground, &windowRect)) {
            caretPoint.x = windowRect.left + 24;
            caretPoint.y = windowRect.top + 72;
        }
    }

    m_candidateWindow->Show(
        caretPoint,
        candidates,
        static_cast<int>(m_highlightIndex),
        static_cast<int>(m_currentPage),
        static_cast<int>(std::max<uint16_t>(m_totalPages, 1)));
}

void KeyroTextService::RequestCandidatesAsync(ITfContext* context)
{
    if (!context || m_inputBuffer.empty()) {
        return;
    }
    if (!m_messageWindow || !m_pipeStarted) {
        ShowLocalCandidatesAndMarkBackend(context, m_inputBuffer);
        return;
    }

    context->AddRef();
    AddRef();

    HWND messageWindow = m_messageWindow;
    std::string query = m_inputBuffer;
    uint32_t page = m_currentPage;

    bool queued = m_pipeClient.RequestCandidatesAsync(
        query,
        page,
        [this, context, messageWindow, query, page](const IpcCandidateResponse& response) {
        if (!IsWindow(messageWindow)) {
            context->Release();
            Release();
            return;
        }

        PendingCandidates* pending = new (std::nothrow) PendingCandidates{
            this,
            context,
            query,
            page,
            response.totalPages,
            response.candidates,
            !response.ok || response.candidates.empty()
        };

        if (!pending || !PostMessageW(messageWindow, WM_KEYRO_TSF_CANDIDATES_READY, 0, reinterpret_cast<LPARAM>(pending))) {
            if (pending) {
                delete pending;
            }
            context->Release();
            Release();
        }
    });

    if (!queued) {
        ShowLocalCandidatesAndMarkBackend(context, query);
        context->Release();
        Release();
    }
}

void KeyroTextService::ShowLocalCandidatesAndMarkBackend(
    ITfContext* context,
    const std::string& query)
{
    m_backendHealthy = false;
    m_currentCandidates = BuildLocalCandidatesFor(query);
    m_totalPages = 1;
    m_highlightIndex = 0;
    if (m_currentCandidates.empty()) {
        HideCandidateWindow();
    } else {
        RequestCandidateWindowUpdate(context, m_currentCandidates);
    }
}

void KeyroTextService::RecordCandidateSelection(const std::wstring& candidate)
{
    if (!m_pipeStarted || m_inputBuffer.empty() || candidate.empty() ||
        m_inputBuffer.front() == 'q' || m_inputBuffer.front() == 'v') {
        return;
    }
    std::string text = WideToUtf8(StripCandidateTag(candidate));
    if (!text.empty()) {
        m_pipeClient.RecordSelectionAsync(m_inputBuffer, text);
    }
}

HRESULT KeyroTextService::RequestCommit(ITfContext* context, const std::wstring& text)
{
    if (!context || text.empty()) {
        return S_OK;
    }

    CommitEditSession* editSession = new (std::nothrow) CommitEditSession(this, context, text);
    if (!editSession) {
        return E_OUTOFMEMORY;
    }

    HRESULT sessionHr = S_OK;
    HRESULT hr = context->RequestEditSession(
        m_clientId,
        editSession,
        TF_ES_ASYNC | TF_ES_READWRITE,
        &sessionHr);

    editSession->Release();
    return FAILED(hr) ? hr : sessionHr;
}

HRESULT KeyroTextService::UpdateCompositionInSession(
    TfEditCookie editCookie,
    ITfContext* context,
    const std::wstring& text)
{
    if (!context) {
        return E_INVALIDARG;
    }

    if (text.empty()) {
        if (m_composition) {
            ITfComposition* composition = m_composition;
            m_composition = nullptr;
            composition->EndComposition(editCookie);
            composition->Release();
        }
        return S_OK;
    }

    if (m_composition) {
        ITfRange* range = nullptr;
        HRESULT hr = m_composition->GetRange(&range);
        if (SUCCEEDED(hr) && range) {
            hr = range->SetText(editCookie, 0, text.c_str(), static_cast<LONG>(text.size()));
            if (SUCCEEDED(hr)) {
                hr = SetSelectionToRangeEnd(editCookie, context, range);
            }
            range->Release();
        }
        return hr;
    }

    ITfInsertAtSelection* insertAtSelection = nullptr;
    HRESULT hr = context->QueryInterface(IID_ITfInsertAtSelection, reinterpret_cast<void**>(&insertAtSelection));
    if (FAILED(hr)) {
        return hr;
    }

    ITfRange* range = nullptr;
    hr = insertAtSelection->InsertTextAtSelection(
        editCookie,
        TF_IAS_QUERYONLY,
        text.c_str(),
        static_cast<LONG>(text.size()),
        &range);
    insertAtSelection->Release();

    if (FAILED(hr) || !range) {
        return FAILED(hr) ? hr : E_FAIL;
    }

    ITfContextComposition* contextComposition = nullptr;
    hr = context->QueryInterface(IID_ITfContextComposition, reinterpret_cast<void**>(&contextComposition));
    if (SUCCEEDED(hr)) {
        hr = contextComposition->StartComposition(
            editCookie,
            range,
            static_cast<ITfCompositionSink*>(this),
            &m_composition);
        contextComposition->Release();
    }

    if (SUCCEEDED(hr)) {
        hr = range->SetText(editCookie, 0, text.c_str(), static_cast<LONG>(text.size()));
        if (SUCCEEDED(hr)) {
            hr = SetSelectionToRangeEnd(editCookie, context, range);
        }
    }

    range->Release();
    return hr;
}

HRESULT KeyroTextService::CommitTextInSession(
    TfEditCookie editCookie,
    ITfContext* context,
    const std::wstring& text)
{
    if (!context || text.empty()) {
        return S_OK;
    }

    if (m_composition) {
        ITfComposition* composition = m_composition;
        m_composition = nullptr;

        ITfRange* range = nullptr;
        HRESULT hr = composition->GetRange(&range);
        if (SUCCEEDED(hr) && range) {
            hr = range->SetText(editCookie, 0, text.c_str(), static_cast<LONG>(text.size()));
        }

        HRESULT endHr = composition->EndComposition(editCookie);
        if (SUCCEEDED(hr) && range) {
            HRESULT selectionHr = SetSelectionToRangeEnd(editCookie, context, range);
            if (FAILED(selectionHr)) {
                hr = selectionHr;
            }
        }
        if (SUCCEEDED(hr) && FAILED(endHr)) {
            hr = endHr;
        }
        if (range) {
            range->Release();
        }
        composition->Release();
        return hr;
    }

    ITfInsertAtSelection* insertAtSelection = nullptr;
    HRESULT hr = context->QueryInterface(IID_ITfInsertAtSelection, reinterpret_cast<void**>(&insertAtSelection));
    if (FAILED(hr)) {
        return hr;
    }

    ITfRange* insertedRange = nullptr;
    hr = insertAtSelection->InsertTextAtSelection(
        editCookie,
        0,
        text.c_str(),
        static_cast<LONG>(text.size()),
        &insertedRange);
    if (SUCCEEDED(hr) && insertedRange) {
        HRESULT selectionHr = SetSelectionToRangeEnd(editCookie, context, insertedRange);
        if (FAILED(selectionHr)) {
            hr = selectionHr;
        }
    }
    if (insertedRange) {
        insertedRange->Release();
    }

    insertAtSelection->Release();
    return hr;
}

HRESULT KeyroTextService::UpdateCandidateWindowInSession(
    TfEditCookie editCookie,
    ITfContext* context,
    const std::vector<std::wstring>& candidates)
{
    if (!context || !m_candidateWindow || candidates.empty()) {
        return S_OK;
    }

    ITfRange* range = nullptr;
    if (m_composition) {
        m_composition->GetRange(&range);
    }

    if (!range) {
        TF_SELECTION selection = {};
        ULONG fetched = 0;
        HRESULT selectionHr = context->GetSelection(editCookie, TF_DEFAULT_SELECTION, 1, &selection, &fetched);
        if (SUCCEEDED(selectionHr) && fetched > 0) {
            range = selection.range;
        }
    }

    bool shown = false;
    HRESULT hr = S_OK;
    ITfContextView* view = nullptr;
    hr = range ? context->GetActiveView(&view) : S_FALSE;
    if (range && SUCCEEDED(hr) && view) {
        RECT textRect = {};
        BOOL clipped = FALSE;
        hr = view->GetTextExt(editCookie, range, &textRect, &clipped);
        if (SUCCEEDED(hr)) {
            POINT caretPoint = { textRect.left, textRect.bottom };
            m_candidateWindow->Show(
                caretPoint,
                candidates,
                static_cast<int>(m_highlightIndex),
                static_cast<int>(m_currentPage),
                static_cast<int>(std::max<uint16_t>(m_totalPages, 1)));
            shown = true;
        }
        view->Release();
    }

    if (!shown) {
        POINT caretPoint = {};
        HWND focusWindow = GetFocus();
        if (focusWindow && GetCaretPos(&caretPoint)) {
            ClientToScreen(focusWindow, &caretPoint);
        } else {
            HWND foreground = GetForegroundWindow();
            RECT windowRect = {};
            if (foreground && GetWindowRect(foreground, &windowRect)) {
                caretPoint.x = windowRect.left + 24;
                caretPoint.y = windowRect.top + 72;
            }
        }
        m_candidateWindow->Show(
            caretPoint,
            candidates,
            static_cast<int>(m_highlightIndex),
            static_cast<int>(m_currentPage),
            static_cast<int>(std::max<uint16_t>(m_totalPages, 1)));
    }

    if (range) {
        range->Release();
    }
    return hr;
}

} // namespace KeyroIME
