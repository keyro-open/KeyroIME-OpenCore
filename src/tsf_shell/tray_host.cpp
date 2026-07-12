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

#include "ui/system_tray.h"

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int)
{
    HRESULT comResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    bool shouldUninitializeCom = SUCCEEDED(comResult);

    // v2 separates the corrected medium-integrity tray from stale elevated
    // v1 instances left behind by older installers.
    HANDLE singleInstance = CreateMutexW(nullptr, TRUE, L"Local\\KeyroIME.TrayHost.v2");
    if (!singleInstance) {
        if (shouldUninitializeCom) {
            CoUninitialize();
        }
        return 1;
    }
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(singleInstance);
        if (shouldUninitializeCom) {
            CoUninitialize();
        }
        return 0;
    }

    int exitCode = 0;
    {
        KeyroIME::SystemTray tray;
        if (!tray.Initialize(instance, nullptr)) {
            exitCode = 1;
        } else {
            tray.Show();

            MSG message = {};
            while (GetMessageW(&message, nullptr, 0, 0) > 0) {
                TranslateMessage(&message);
                DispatchMessageW(&message);
            }
            exitCode = static_cast<int>(message.wParam);
        }
    }

    ReleaseMutex(singleInstance);
    CloseHandle(singleInstance);
    if (shouldUninitializeCom) {
        CoUninitialize();
    }
    return exitCode;
}
