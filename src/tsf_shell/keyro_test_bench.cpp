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
// keyro_test_bench.cpp
// KeyroIME v1.0 スタンドアロン・サンドボックステストプログラム
// 名前付きパイプで keyro_service へ接続し、候補ウィンドウの更新を駆動します。

#include <windows.h>
#include <cctype>
#include <mutex>
#include <string>
#include <vector>
#include "ipc/named_pipe_client.h"
#include "ui/candidate_window.h"

using namespace KeyroIME;

constexpr UINT WM_KEYRO_CANDIDATES_READY = WM_APP + 101;

// グローバル変数
CandidateWindow* g_candidateWindow = nullptr;
NamedPipeClient* g_pipeClient = nullptr;
std::string g_romajiBuffer;
std::string g_kanaBuffer;
int g_currentPage = 0;
double g_lastLatencyMs = 0.0;
bool g_lastLookupOk = false;
std::string g_lastLookupError;
std::vector<std::wstring> g_pendingCandidates;
std::mutex g_candidateMutex;

void HideCandidateWindow()
{
    if (g_candidateWindow) {
        g_candidateWindow->Hide();
    }
}

void ShowCandidateWindow(HWND hwnd, const std::vector<std::wstring>& candidates)
{
    if (candidates.empty()) {
        HideCandidateWindow();
        return;
    }

    POINT caretPos;
    if (!GetCaretPos(&caretPos)) {
        caretPos.x = 20;
        caretPos.y = 60;
    }
    ClientToScreen(hwnd, &caretPos);

    if (g_candidateWindow) {
        g_candidateWindow->Show(caretPos, candidates, 0, g_currentPage, 1);
    }
}

void RequestCandidateLookup(HWND hwnd)
{
    if (g_romajiBuffer.empty()) {
        if (g_candidateWindow) {
            g_candidateWindow->Hide();
        }
        return;
    }

    g_kanaBuffer = g_romajiBuffer;

    if (!g_pipeClient) {
        return;
    }

    g_pipeClient->RequestCandidatesAsync(g_kanaBuffer, [hwnd](const IpcCandidateResponse& response) {
        {
            std::lock_guard<std::mutex> lock(g_candidateMutex);
            g_lastLookupOk = response.ok;
            g_lastLatencyMs = response.latencyMs;
            g_lastLookupError = response.errorMessage;
            g_pendingCandidates = response.candidates;
        }
        PostMessageW(hwnd, WM_KEYRO_CANDIDATES_READY, 0, 0);
    });
}

// ウィンドウプロシージャ
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
        {
            // 候補ウィンドウを作成します。
            g_candidateWindow = new CandidateWindow();
            g_candidateWindow->Initialize(GetModuleHandle(nullptr));

            g_pipeClient = new NamedPipeClient();
            g_pipeClient->Start();
            return 0;
        }
        
        case WM_CHAR:
        {
            char ch = (char)wParam;
            
            if (ch == VK_RETURN) {
                // Enter: コミットしてクリアします。
                g_romajiBuffer.clear();
                g_kanaBuffer.clear();
                g_currentPage = 0;
                HideCandidateWindow();
                InvalidateRect(hwnd, nullptr, TRUE);
                return 0;
            }
            
            if (ch == VK_BACK) {
                // Backspace
                if (!g_romajiBuffer.empty()) {
                    g_romajiBuffer.pop_back();
                    g_currentPage = 0;
                    RequestCandidateLookup(hwnd);
                    InvalidateRect(hwnd, nullptr, TRUE);
                } else {
                    HideCandidateWindow();
                }
                return 0;
            }
            
            if (std::isalpha(static_cast<unsigned char>(ch)) || std::isdigit(static_cast<unsigned char>(ch))) {
                g_romajiBuffer += ch;
                g_currentPage = 0;
                RequestCandidateLookup(hwnd);
                InvalidateRect(hwnd, nullptr, TRUE);
                return 0;
            }
            
            break;
        }
        
        case WM_KEYDOWN:
        {
            if (wParam == VK_NEXT || wParam == VK_OEM_PERIOD) {
                // 現在の IPC v1 要求はページ番号を持たず、1ページ目の低遅延検索を維持します。
                return 0;
            }
            
            if (wParam == VK_PRIOR || wParam == VK_OEM_COMMA) {
                // 現在の IPC v1 要求はページ番号を持たず、1ページ目の低遅延検索を維持します。
                return 0;
            }
            break;
        }

        case WM_KEYRO_CANDIDATES_READY:
        {
            std::vector<std::wstring> candidates;
            bool ok = false;
            {
                std::lock_guard<std::mutex> lock(g_candidateMutex);
                candidates = g_pendingCandidates;
                ok = g_lastLookupOk;
            }

            if (ok) {
                ShowCandidateWindow(hwnd, candidates);
            } else {
                HideCandidateWindow();
            }

            InvalidateRect(hwnd, nullptr, TRUE);
            return 0;
        }
        
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            // 入力状態を描画します。
            RECT rect;
            GetClientRect(hwnd, &rect);
            
            SetBkMode(hdc, TRANSPARENT);
            
            char latencyText[128];
            sprintf_s(latencyText, "IPC latency: %.3f ms", g_lastLatencyMs);

            std::string display = "Input: " + g_romajiBuffer +
                "\nService query: " + g_kanaBuffer +
                "\n" + latencyText;
            if (!g_lastLookupOk && !g_lastLookupError.empty()) {
                display += "\nPipe: " + g_lastLookupError;
            }
            DrawTextA(hdc, display.c_str(), -1, &rect, DT_LEFT | DT_TOP);
            
            EndPaint(hwnd, &ps);
            return 0;
        }
        
        case WM_DESTROY:
        {
            if (g_pipeClient) {
                g_pipeClient->Stop();
                delete g_pipeClient;
                g_pipeClient = nullptr;
            }

            if (g_candidateWindow) {
                delete g_candidateWindow;
                g_candidateWindow = nullptr;
            }

            PostQuitMessage(0);
            return 0;
        }
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// メイン関数
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // ウィンドウクラスを登録
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"KeyroTestBench";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    
    RegisterClassW(&wc);
    
    // メインウィンドウを作成します。
    HWND hwnd = CreateWindowW(
        L"KeyroTestBench",
        L"KeyroIME v1.0 Sandbox Test Bench",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 600, 400,
        nullptr, nullptr, hInstance, nullptr
    );
    
    if (!hwnd) {
        MessageBoxW(nullptr, L"ウィンドウの作成に失敗しました", L"エラー", MB_OK | MB_ICONERROR);
        return 1;
    }
    
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    
    // メッセージループ
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return (int)msg.wParam;
}
