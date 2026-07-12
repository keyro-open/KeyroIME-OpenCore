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
// named_pipe_client.h
// KeyroIME TSF shell - compact binary named pipe client.

#pragma once

#include <windows.h>

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace KeyroIME {

struct IpcCandidateResponse {
    bool ok = false;
    std::vector<std::wstring> candidates;
    uint16_t totalPages = 1;
    double latencyMs = 0.0;
    DWORD errorCode = ERROR_SUCCESS;
    std::string errorMessage;
};

class NamedPipeClient {
public:
    using ResponseCallback = std::function<void(const IpcCandidateResponse&)>;

    NamedPipeClient();
    explicit NamedPipeClient(const wchar_t* pipeName);
    ~NamedPipeClient();

    NamedPipeClient(const NamedPipeClient&) = delete;
    NamedPipeClient& operator=(const NamedPipeClient&) = delete;

    bool Start();
    void Stop();

    bool RequestCandidatesAsync(
        const std::string& input,
        uint32_t page,
        ResponseCallback callback);
    bool RequestCandidatesAsync(const std::string& input, ResponseCallback callback)
    {
        return RequestCandidatesAsync(input, 0, std::move(callback));
    }
    IpcCandidateResponse RequestCandidatesSync(const std::string& input, uint32_t page = 0);
    bool RecordSelectionAsync(const std::string& reading, const std::string& text);
    bool UpdateSettingsAsync(
        int inputMode,
        int charWidth,
        int keyboardLayout,
        int punctuationStyle,
        int englishCase);

private:
    struct PendingRequest {
        uint8_t requestType;
        std::string input;
        uint32_t page;
        ResponseCallback callback;
    };

    static constexpr const wchar_t* PIPE_NAME = L"\\\\.\\pipe\\KeyroIME.Service.v1";
    static constexpr uint8_t REQUEST_LOOKUP_CANDIDATES = 1;
    static constexpr uint8_t REQUEST_RECORD_SELECTION = 2;
    static constexpr uint8_t REQUEST_UPDATE_SETTINGS = 3;
    static constexpr size_t REQUEST_HEADER_LEN = 9;
    static constexpr size_t RESPONSE_HEADER_LEN = 8;
    static constexpr uint8_t MAX_CANDIDATE_COUNT = 5;
    static constexpr uint32_t MAX_RESPONSE_PAYLOAD_BYTES = 64 * 1024;
    static constexpr uint8_t STATUS_OK = 0;
    static constexpr DWORD IPC_TIMEOUT_MS = 8;

    HANDLE m_pipe;
    std::wstring m_pipeName;
    std::thread m_worker;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::queue<PendingRequest> m_queue;
    bool m_running;

    void WorkerLoop();
    using IpcDeadline = std::chrono::steady_clock::time_point;

    bool EnsureConnected(DWORD& errorCode, const IpcDeadline& deadline);
    void ClosePipe();

    IpcCandidateResponse ExecuteRequest(uint8_t requestType, const std::string& input, uint32_t page);
    IpcCandidateResponse ExecuteRequestOnce(
        uint8_t requestType,
        const std::string& input,
        uint32_t page,
        const IpcDeadline& deadline);

    bool WriteAll(
        const uint8_t* data,
        DWORD size,
        DWORD& errorCode,
        const IpcDeadline& deadline);
    bool ReadExact(
        uint8_t* data,
        DWORD size,
        DWORD& errorCode,
        const IpcDeadline& deadline);

    bool TransferWithTimeout(
        bool writeOperation,
        uint8_t* buffer,
        DWORD size,
        DWORD& transferred,
        DWORD& errorCode,
        const IpcDeadline& deadline);

    static DWORD RemainingTimeoutMs(const IpcDeadline& deadline);

    static std::vector<uint8_t> BuildRequest(
        uint8_t requestType,
        const std::string& input,
        uint32_t page);
    static IpcCandidateResponse ParseResponse(const std::vector<uint8_t>& response);
    static std::wstring Utf8ToWide(const std::string& utf8);
};

} // namespace KeyroIME
