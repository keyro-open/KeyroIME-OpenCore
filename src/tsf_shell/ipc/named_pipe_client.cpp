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
// named_pipe_client.cpp
// KeyroIME TSF shell - compact binary named pipe client implementation.

#include "named_pipe_client.h"

#include <algorithm>
#include <cstdio>
#include <exception>

namespace KeyroIME {

NamedPipeClient::NamedPipeClient()
    : NamedPipeClient(PIPE_NAME)
{
}

NamedPipeClient::NamedPipeClient(const wchar_t* pipeName)
    : m_pipe(INVALID_HANDLE_VALUE)
    , m_pipeName(pipeName ? pipeName : PIPE_NAME)
    , m_running(false)
{
}

NamedPipeClient::~NamedPipeClient()
{
    Stop();
}

bool NamedPipeClient::Start()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_running) {
        return true;
    }

    try {
        m_running = true;
        m_worker = std::thread(&NamedPipeClient::WorkerLoop, this);
        return true;
    } catch (...) {
        m_running = false;
        return false;
    }
}

void NamedPipeClient::Stop()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_running) {
            ClosePipe();
            return;
        }
        m_running = false;
    }

    m_cv.notify_all();

    if (m_worker.joinable()) {
        m_worker.join();
    }

    ClosePipe();
}

bool NamedPipeClient::RequestCandidatesAsync(
    const std::string& input,
    uint32_t page,
    ResponseCallback callback)
{
    std::vector<ResponseCallback> supersededCallbacks;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_running) {
            return false;
        }

        std::queue<PendingRequest> preservedRequests;
        while (!m_queue.empty()) {
            PendingRequest pending = std::move(m_queue.front());
            m_queue.pop();
            if (pending.requestType == REQUEST_RECORD_SELECTION) {
                preservedRequests.push(std::move(pending));
            } else if (pending.callback) {
                supersededCallbacks.push_back(std::move(pending.callback));
            }
        }
        m_queue.swap(preservedRequests);
        m_queue.push(PendingRequest{
            REQUEST_LOOKUP_CANDIDATES,
            input,
            page,
            std::move(callback)
        });
    }

    IpcCandidateResponse superseded;
    superseded.errorCode = ERROR_CANCELLED;
    superseded.errorMessage = "request superseded by newer input";
    for (ResponseCallback& oldCallback : supersededCallbacks) {
        try {
            oldCallback(superseded);
        } catch (...) {
        }
    }

    m_cv.notify_one();
    return true;
}

IpcCandidateResponse NamedPipeClient::RequestCandidatesSync(const std::string& input, uint32_t page)
{
    try {
        return ExecuteRequest(REQUEST_LOOKUP_CANDIDATES, input, page);
    } catch (...) {
        IpcCandidateResponse result;
        result.errorCode = ERROR_UNHANDLED_EXCEPTION;
        result.errorMessage = "unexpected IPC client exception";
        return result;
    }
}

bool NamedPipeClient::RecordSelectionAsync(
    const std::string& reading,
    const std::string& text)
{
    if (reading.empty() || text.empty() || reading.find('\t') != std::string::npos ||
        text.find('\t') != std::string::npos) {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_running) {
        return false;
    }
    m_queue.push(PendingRequest{
        REQUEST_RECORD_SELECTION,
        reading + "\t" + text,
        0,
        ResponseCallback()
    });
    m_cv.notify_one();
    return true;
}

bool NamedPipeClient::UpdateSettingsAsync(
    int inputMode,
    int charWidth,
    int keyboardLayout,
    int punctuationStyle,
    int englishCase)
{
    char payload[32] = {};
    sprintf_s(
        payload,
        "%d\t%d\t%d\t%d\t%d",
        inputMode,
        charWidth,
        keyboardLayout,
        punctuationStyle,
        englishCase);

    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_running) {
        return false;
    }
    m_queue.push(PendingRequest{
        REQUEST_UPDATE_SETTINGS,
        payload,
        0,
        ResponseCallback()
    });
    m_cv.notify_one();
    return true;
}

void NamedPipeClient::WorkerLoop()
{
    for (;;) {
        PendingRequest request;

        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [&]() { return !m_running || !m_queue.empty(); });

            if (!m_running && m_queue.empty()) {
                break;
            }

            request = std::move(m_queue.front());
            m_queue.pop();
        }

        IpcCandidateResponse response;
        try {
            response = ExecuteRequest(request.requestType, request.input, request.page);
        } catch (...) {
            response.errorCode = ERROR_UNHANDLED_EXCEPTION;
            response.errorMessage = "unexpected IPC worker exception";
        }
        if (request.callback) {
            try {
                request.callback(response);
            } catch (...) {
            }
        }
    }
}

bool NamedPipeClient::EnsureConnected(DWORD& errorCode, const IpcDeadline& deadline)
{
    errorCode = ERROR_SUCCESS;
    if (m_pipe != INVALID_HANDLE_VALUE) {
        return true;
    }

    if (RemainingTimeoutMs(deadline) == 0) {
        errorCode = ERROR_TIMEOUT;
        return false;
    }

    // Never wait for a busy/missing service instance. CreateFile either
    // connects immediately or fails into the local TSF fallback path.
    m_pipe = CreateFileW(
        m_pipeName.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
        nullptr);

    if (m_pipe == INVALID_HANDLE_VALUE) {
        errorCode = GetLastError();
        return false;
    }

    DWORD mode = PIPE_READMODE_BYTE;
    if (!SetNamedPipeHandleState(m_pipe, &mode, nullptr, nullptr)) {
        errorCode = GetLastError();
        ClosePipe();
        return false;
    }

    return true;
}

void NamedPipeClient::ClosePipe()
{
    if (m_pipe != INVALID_HANDLE_VALUE) {
        CloseHandle(m_pipe);
        m_pipe = INVALID_HANDLE_VALUE;
    }
}

IpcCandidateResponse NamedPipeClient::ExecuteRequest(
    uint8_t requestType,
    const std::string& input,
    uint32_t page)
{
    auto started = std::chrono::steady_clock::now();
    auto deadline = started + std::chrono::milliseconds(IPC_TIMEOUT_MS);
    IpcCandidateResponse result = ExecuteRequestOnce(requestType, input, page, deadline);
    if (!result.ok) {
        ClosePipe();
    }

    auto finished = std::chrono::steady_clock::now();
    result.latencyMs = std::chrono::duration<double, std::milli>(finished - started).count();
    return result;
}

IpcCandidateResponse NamedPipeClient::ExecuteRequestOnce(
    uint8_t requestType,
    const std::string& input,
    uint32_t page,
    const IpcDeadline& deadline)
{
    IpcCandidateResponse result;
    DWORD errorCode = ERROR_SUCCESS;

    if (!EnsureConnected(errorCode, deadline)) {
        result.errorCode = errorCode;
        result.errorMessage = "failed to connect named pipe";
        return result;
    }

    std::vector<uint8_t> request = BuildRequest(requestType, input, page);
    if (!WriteAll(request.data(), static_cast<DWORD>(request.size()), errorCode, deadline)) {
        ClosePipe();
        result.errorCode = errorCode;
        result.errorMessage = "failed to write named pipe request";
        return result;
    }

    uint8_t header[RESPONSE_HEADER_LEN] = {};
    if (!ReadExact(header, RESPONSE_HEADER_LEN, errorCode, deadline)) {
        ClosePipe();
        result.errorCode = errorCode;
        result.errorMessage = "failed to read named pipe response header";
        return result;
    }

    uint32_t payloadLen =
        static_cast<uint32_t>(header[4]) |
        (static_cast<uint32_t>(header[5]) << 8) |
        (static_cast<uint32_t>(header[6]) << 16) |
        (static_cast<uint32_t>(header[7]) << 24);

    if (payloadLen > MAX_RESPONSE_PAYLOAD_BYTES) {
        ClosePipe();
        result.errorCode = ERROR_INVALID_DATA;
        result.errorMessage = "named pipe response payload exceeds limit";
        return result;
    }

    std::vector<uint8_t> response(RESPONSE_HEADER_LEN + payloadLen);
    std::copy(header, header + RESPONSE_HEADER_LEN, response.begin());

    if (payloadLen > 0 &&
        !ReadExact(response.data() + RESPONSE_HEADER_LEN, payloadLen, errorCode, deadline)) {
        ClosePipe();
        result.errorCode = errorCode;
        result.errorMessage = "failed to read named pipe response payload";
        return result;
    }

    return ParseResponse(response);
}

bool NamedPipeClient::WriteAll(
    const uint8_t* data,
    DWORD size,
    DWORD& errorCode,
    const IpcDeadline& deadline)
{
    DWORD offset = 0;
    while (offset < size) {
        DWORD written = 0;
        if (!TransferWithTimeout(
                true,
                const_cast<uint8_t*>(data + offset),
                size - offset,
                written,
                errorCode,
                deadline)) {
            return false;
        }
        if (written == 0) {
            errorCode = ERROR_WRITE_FAULT;
            return false;
        }
        offset += written;
    }
    return true;
}

bool NamedPipeClient::ReadExact(
    uint8_t* data,
    DWORD size,
    DWORD& errorCode,
    const IpcDeadline& deadline)
{
    DWORD offset = 0;
    while (offset < size) {
        DWORD bytesRead = 0;
        if (!TransferWithTimeout(
                false,
                data + offset,
                size - offset,
                bytesRead,
                errorCode,
                deadline)) {
            return false;
        }
        if (bytesRead == 0) {
            errorCode = ERROR_BROKEN_PIPE;
            return false;
        }
        offset += bytesRead;
    }
    return true;
}

bool NamedPipeClient::TransferWithTimeout(
    bool writeOperation,
    uint8_t* buffer,
    DWORD size,
    DWORD& transferred,
    DWORD& errorCode,
    const IpcDeadline& deadline)
{
    transferred = 0;
    errorCode = ERROR_SUCCESS;

    HANDLE eventHandle = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!eventHandle) {
        errorCode = GetLastError();
        return false;
    }

    OVERLAPPED overlapped = {};
    overlapped.hEvent = eventHandle;

    BOOL started = writeOperation
        ? WriteFile(m_pipe, buffer, size, nullptr, &overlapped)
        : ReadFile(m_pipe, buffer, size, nullptr, &overlapped);

    if (!started) {
        errorCode = GetLastError();
        if (errorCode != ERROR_IO_PENDING) {
            CloseHandle(eventHandle);
            return false;
        }

        auto now = std::chrono::steady_clock::now();
        HANDLE timerHandle = nullptr;
        DWORD waitResult = WAIT_TIMEOUT;
        if (now < deadline) {
            constexpr DWORD kCreateWaitableTimerHighResolution = 0x00000002;
            timerHandle = CreateWaitableTimerExW(
                nullptr,
                nullptr,
                kCreateWaitableTimerHighResolution,
                TIMER_ALL_ACCESS);
            if (!timerHandle) {
                timerHandle = CreateWaitableTimerW(nullptr, TRUE, nullptr);
            }

            auto remaining100ns = std::chrono::duration_cast<
                std::chrono::duration<int64_t, std::ratio<1, 10'000'000>>>(deadline - now).count();
            LARGE_INTEGER dueTime = {};
            dueTime.QuadPart = -std::max<int64_t>(remaining100ns, 1);
            if (timerHandle && SetWaitableTimer(timerHandle, &dueTime, 0, nullptr, nullptr, FALSE)) {
                HANDLE handles[] = { eventHandle, timerHandle };
                waitResult = WaitForMultipleObjects(2, handles, FALSE, INFINITE);
            }
        }

        if (waitResult != WAIT_OBJECT_0) {
            CancelIoEx(m_pipe, &overlapped);
            HANDLE failedPipe = m_pipe;
            m_pipe = INVALID_HANDLE_VALUE;
            CloseHandle(failedPipe);
            WaitForSingleObject(eventHandle, INFINITE);
            errorCode = waitResult == WAIT_OBJECT_0 + 1 || waitResult == WAIT_TIMEOUT
                ? ERROR_TIMEOUT
                : GetLastError();
            if (timerHandle) {
                CloseHandle(timerHandle);
            }
            CloseHandle(eventHandle);
            return false;
        }
        if (timerHandle) {
            CancelWaitableTimer(timerHandle);
            CloseHandle(timerHandle);
        }
    }

    BOOL completed = GetOverlappedResult(m_pipe, &overlapped, &transferred, FALSE);
    if (!completed) {
        errorCode = GetLastError();
    }

    CloseHandle(eventHandle);
    return completed == TRUE;
}

DWORD NamedPipeClient::RemainingTimeoutMs(const IpcDeadline& deadline)
{
    auto now = std::chrono::steady_clock::now();
    if (now >= deadline) {
        return 0;
    }

    auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now).count();
    return static_cast<DWORD>(std::max<int64_t>(remaining, 1));
}

std::vector<uint8_t> NamedPipeClient::BuildRequest(
    uint8_t requestType,
    const std::string& input,
    uint32_t page)
{
    uint32_t len = static_cast<uint32_t>(input.size());
    std::vector<uint8_t> request;
    request.reserve(REQUEST_HEADER_LEN + input.size());
    request.push_back(requestType);
    request.push_back(static_cast<uint8_t>(page & 0xff));
    request.push_back(static_cast<uint8_t>((page >> 8) & 0xff));
    request.push_back(static_cast<uint8_t>((page >> 16) & 0xff));
    request.push_back(static_cast<uint8_t>((page >> 24) & 0xff));
    request.push_back(static_cast<uint8_t>(len & 0xff));
    request.push_back(static_cast<uint8_t>((len >> 8) & 0xff));
    request.push_back(static_cast<uint8_t>((len >> 16) & 0xff));
    request.push_back(static_cast<uint8_t>((len >> 24) & 0xff));
    request.insert(request.end(), input.begin(), input.end());
    return request;
}

IpcCandidateResponse NamedPipeClient::ParseResponse(const std::vector<uint8_t>& response)
{
    IpcCandidateResponse result;
    if (response.size() < RESPONSE_HEADER_LEN) {
        result.errorCode = ERROR_INVALID_DATA;
        result.errorMessage = "response is shorter than fixed header";
        return result;
    }

    uint8_t status = response[0];
    uint8_t count = response[1];
    if (count > MAX_CANDIDATE_COUNT) {
        result.errorCode = ERROR_INVALID_DATA;
        result.errorMessage = "response candidate count exceeds limit";
        return result;
    }
    result.totalPages =
        static_cast<uint16_t>(response[2]) |
        static_cast<uint16_t>(response[3] << 8);
    uint32_t payloadLen =
        static_cast<uint32_t>(response[4]) |
        (static_cast<uint32_t>(response[5]) << 8) |
        (static_cast<uint32_t>(response[6]) << 16) |
        (static_cast<uint32_t>(response[7]) << 24);

    if (response.size() != RESPONSE_HEADER_LEN + payloadLen) {
        result.errorCode = ERROR_INVALID_DATA;
        result.errorMessage = "response payload length mismatch";
        return result;
    }

    if (status != STATUS_OK) {
        result.errorCode = ERROR_INVALID_DATA;
        result.errorMessage.assign(
            reinterpret_cast<const char*>(response.data() + RESPONSE_HEADER_LEN),
            reinterpret_cast<const char*>(response.data() + response.size()));
        return result;
    }

    size_t cursor = RESPONSE_HEADER_LEN;
    for (uint8_t i = 0; i < count && cursor + 2 <= response.size(); ++i) {
        uint16_t textLen =
            static_cast<uint16_t>(response[cursor]) |
            static_cast<uint16_t>(response[cursor + 1] << 8);
        cursor += 2;

        if (cursor + textLen > response.size()) {
            result.errorCode = ERROR_INVALID_DATA;
            result.errorMessage = "candidate entry exceeds response payload";
            return result;
        }

        std::string utf8(
            reinterpret_cast<const char*>(response.data() + cursor),
            reinterpret_cast<const char*>(response.data() + cursor + textLen));
        cursor += textLen;

        if (!utf8.empty()) {
            std::wstring wide = Utf8ToWide(utf8);
            if (wide.empty()) {
                result.errorCode = ERROR_INVALID_DATA;
                result.errorMessage = "candidate is not valid UTF-8";
                return result;
            }
            result.candidates.push_back(std::move(wide));
        }
    }

    if (cursor != response.size()) {
        result.errorCode = ERROR_INVALID_DATA;
        result.errorMessage = "response contains trailing bytes";
        return result;
    }

    result.ok = true;
    return result;
}

std::wstring NamedPipeClient::Utf8ToWide(const std::string& utf8)
{
    if (utf8.empty()) {
        return std::wstring();
    }

    int len = MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        utf8.c_str(),
        static_cast<int>(utf8.size()),
        nullptr,
        0);
    if (len <= 0) {
        return std::wstring();
    }

    std::wstring wide(static_cast<size_t>(len), L'\0');
    if (MultiByteToWideChar(
            CP_UTF8,
            MB_ERR_INVALID_CHARS,
            utf8.c_str(),
            static_cast<int>(utf8.size()),
            wide.data(),
            len) != len) {
        return std::wstring();
    }
    return wide;
}

} // namespace KeyroIME
