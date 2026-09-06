// Copyright (C) 2025-2026 株式会社LocalPro (LocalPro Co., Ltd.). All rights reserved.
// Brand Official Website: https://keyro.jp
//
// This file is part of KeyroIME (キーロ) OpenCore.
// GNU GPLv3に基づいて配布されます。LICENSE（英語正文）を参照してください。
// text_service.h
// Minimal COM object for the first TSF shell registration slice.

#pragma once

#include <msctf.h>
#include <unknwn.h>

#include <string>
#include <vector>

#include "../ipc/named_pipe_client.h"
#include "keyboard_layout.h"
#include "shared_settings.h"

namespace KeyroIME {

class CandidateWindow;
class CandidateWindowEditSession;
class CommitEditSession;
class CompositionEditSession;

void DllAddRef();
void DllRelease();

class KeyroTextService final :
    public ITfTextInputProcessor,
    public ITfKeyEventSink,
    public ITfCompositionSink {
public:
    KeyroTextService();
    ~KeyroTextService();

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override;
    ULONG STDMETHODCALLTYPE AddRef() override;
    ULONG STDMETHODCALLTYPE Release() override;

    HRESULT STDMETHODCALLTYPE Activate(ITfThreadMgr* pThreadMgr, TfClientId clientId) override;
    HRESULT STDMETHODCALLTYPE Deactivate() override;

    HRESULT STDMETHODCALLTYPE OnSetFocus(BOOL foreground) override;
    HRESULT STDMETHODCALLTYPE OnTestKeyDown(
        ITfContext* context,
        WPARAM wParam,
        LPARAM lParam,
        BOOL* eaten) override;
    HRESULT STDMETHODCALLTYPE OnKeyDown(
        ITfContext* context,
        WPARAM wParam,
        LPARAM lParam,
        BOOL* eaten) override;
    HRESULT STDMETHODCALLTYPE OnTestKeyUp(
        ITfContext* context,
        WPARAM wParam,
        LPARAM lParam,
        BOOL* eaten) override;
    HRESULT STDMETHODCALLTYPE OnKeyUp(
        ITfContext* context,
        WPARAM wParam,
        LPARAM lParam,
        BOOL* eaten) override;
    HRESULT STDMETHODCALLTYPE OnPreservedKey(
        ITfContext* context,
        REFGUID guid,
        BOOL* eaten) override;

    HRESULT STDMETHODCALLTYPE OnCompositionTerminated(
        TfEditCookie editCookie,
        ITfComposition* composition) override;

private:
    struct PendingCandidates;

    static constexpr UINT WM_KEYRO_TSF_CANDIDATES_READY = WM_APP + 0x241;

    volatile LONG m_refCount;
    ITfThreadMgr* m_threadMgr;
    TfClientId m_clientId;
    bool m_keyEventSinkAdvised;
    bool m_ansiLayoutShortcutPreserved;
    bool m_jisLayoutShortcutPreserved;
    bool m_kanjiKeyPreserved;
    bool m_imeOnKeyPreserved;
    bool m_imeOffKeyPreserved;
    bool m_kanaKeyPreserved;
    bool m_convertKeyPreserved;
    bool m_nonConvertKeyPreserved;
    ITfComposition* m_composition;
    HWND m_messageWindow;
    DWORD m_tsfThreadId;
    std::string m_inputBuffer;
    std::vector<std::wstring> m_currentCandidates;
    uint32_t m_currentPage;
    uint16_t m_totalPages;
    size_t m_highlightIndex;
    bool m_hasNavigated;
    bool m_shiftTracking;
    bool m_shiftChordUsed;
    bool m_altTracking;
    bool m_pipeStarted;
    bool m_backendHealthy;
    InputSettingsSnapshot m_runtimeSettings;
    NamedPipeClient m_pipeClient;
    CandidateWindow* m_candidateWindow;
    mutable SharedInputSettings m_sharedSettings;
    KeyboardLayoutMapper m_keyboardLayoutMapper;

    HRESULT AdviseKeyEventSink(ITfThreadMgr* threadMgr);
    void UnadviseKeyEventSink();

    HRESULT CreateMessageWindow();
    void DestroyMessageWindow();
    static LRESULT CALLBACK MessageWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    bool IsHandledKey(WPARAM wParam, LPARAM lParam) const;
    bool AppendKeyToBuffer(WPARAM wParam);
    bool IsDirectPrintableKey(WPARAM wParam) const;
    bool IsAltPressed(LPARAM lParam) const;
    bool IsSettingsShortcutKey(WPARAM wParam, LPARAM lParam) const;
    bool HandleSettingsShortcut(
        ITfContext* context,
        WPARAM wParam,
        LPARAM lParam,
        InputSettingsSnapshot settings);
    bool IsNavigationKey(WPARAM wParam) const;
    bool IsPagePreviousKey(WPARAM wParam) const;
    bool IsPageNextKey(WPARAM wParam) const;
    bool HasCandidatePages() const;
    void MoveHighlight(int delta);
    void ResetBuffer();
    void HideCandidateWindow();
    std::wstring BufferAsWide() const;
    std::wstring LocalFallbackFor(const std::string& input) const;
    std::vector<std::wstring> BuildLocalCandidatesFor(const std::string& input) const;
    std::wstring ApplyCurrentInputSettings(const std::wstring& text, bool kanaSource) const;
    std::wstring BuildDirectEnglishKey(WPARAM wParam, LPARAM lParam) const;
    InputSettingsSnapshot CurrentInputSettings() const;
    void PublishInputSettings(const InputSettingsSnapshot& snapshot);
    std::wstring SelectedCandidateOrFallback() const;
    HRESULT RequestCompositionUpdate(ITfContext* context);
    HRESULT RequestCompositionCancel(ITfContext* context);
    HRESULT RequestCandidateWindowUpdate(ITfContext* context, const std::vector<std::wstring>& candidates);
    void ShowCandidateWindowAtFallbackPosition(const std::vector<std::wstring>& candidates);
    void RequestCandidatesAsync(ITfContext* context);
    void ShowLocalCandidatesAndMarkBackend(ITfContext* context, const std::string& query);
    void RecordCandidateSelection(const std::wstring& candidate);
    HRESULT RequestCommit(ITfContext* context, const std::wstring& text);
    void RefreshCompositionAndCandidates(ITfContext* context);

    HRESULT UpdateCompositionInSession(TfEditCookie editCookie, ITfContext* context, const std::wstring& text);
    HRESULT CommitTextInSession(TfEditCookie editCookie, ITfContext* context, const std::wstring& text);
    HRESULT UpdateCandidateWindowInSession(
        TfEditCookie editCookie,
        ITfContext* context,
        const std::vector<std::wstring>& candidates);

    friend class CandidateWindowEditSession;
    friend class CommitEditSession;
    friend class CompositionEditSession;
};

} // namespace KeyroIME
