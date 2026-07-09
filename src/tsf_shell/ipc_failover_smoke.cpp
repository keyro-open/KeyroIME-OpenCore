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
#include "ipc/named_pipe_client.h"

#include <chrono>
#include <iostream>
#include <thread>
#include <windows.h>

int main()
{
    const wchar_t* pipeName = L"\\\\.\\pipe\\KeyroIME.FailoverSmoke";
    HANDLE server = CreateNamedPipeW(
        pipeName,
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
        1,
        4096,
        4096,
        0,
        nullptr);
    if (server == INVALID_HANDLE_VALUE) {
        std::cerr << "failed to create smoke pipe\n";
        return 1;
    }

    std::thread stalledServer([server]() {
        BOOL connected = ConnectNamedPipe(server, nullptr);
        if (!connected && GetLastError() != ERROR_PIPE_CONNECTED) {
            CloseHandle(server);
            return;
        }
        Sleep(40);
        DisconnectNamedPipe(server);
        CloseHandle(server);
    });

    KeyroIME::NamedPipeClient client(pipeName);
    KeyroIME::IpcCandidateResponse response = client.RequestCandidatesSync("koukan", 0);
    stalledServer.join();

    if (response.ok || response.errorCode != ERROR_TIMEOUT || response.latencyMs >= 10.0) {
        std::cerr << "failover deadline failed: ok=" << response.ok
                  << " error=" << response.errorCode
                  << " latency_ms=" << response.latencyMs << "\n";
        return 2;
    }

    std::cout << "IPC failover smoke passed: latency_ms=" << response.latencyMs << "\n";
    return 0;
}
