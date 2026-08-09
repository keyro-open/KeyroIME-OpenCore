// Copyright (C) 2025-2026 株式会社LocalPro (LocalPro Co., Ltd.). All rights reserved.
// Brand Official Website: https://keyro.jp
//
// This file is part of KeyroIME (キーロ) OpenCore.
// GNU GPLv3に基づいて配布されます。LICENSE（英語正文）を参照してください。
#include "ipc/named_pipe_client.h"

#include <chrono>
#include <iostream>
#include <thread>
#include <windows.h>

int main()
{
    const wchar_t* pipeName = L"\\\\.\\pipe\\KeyroIME.FailoverSmoke";
    KeyroIME::NamedPipeClient client(pipeName);
    KeyroIME::IpcCandidateResponse oversized =
        client.RequestCandidatesSync(std::string(4097, 'a'), 0);
    if (oversized.ok || oversized.errorCode != ERROR_INVALID_DATA) {
        std::cerr << "oversized IPC request was not rejected\n";
        return 1;
    }

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
        return 2;
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

    KeyroIME::IpcCandidateResponse response = client.RequestCandidatesSync("koukan", 0);
    stalledServer.join();

    if (response.ok || response.errorCode != ERROR_TIMEOUT || response.latencyMs >= 10.0) {
        std::cerr << "failover deadline failed: ok=" << response.ok
                  << " error=" << response.errorCode
                  << " latency_ms=" << response.latencyMs << "\n";
        return 3;
    }

    std::cout << "IPC failover smoke passed: latency_ms=" << response.latencyMs << "\n";
    return 0;
}
