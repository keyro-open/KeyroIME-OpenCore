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
// ipc_latency_bench.cpp
// Console benchmark client for KeyroIME named pipe lookup latency.

#include "ipc/named_pipe_client.h"

#include <algorithm>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

namespace {

std::wstring Utf8ToWide(const std::string& value)
{
    if (value.empty()) return std::wstring();
    int length = MultiByteToWideChar(
        CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
    if (length <= 0) return std::wstring();
    std::wstring result(static_cast<size_t>(length), L'\0');
    MultiByteToWideChar(
        CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), length);
    return result;
}

std::string WideToUtf8(const std::wstring& value)
{
    if (value.empty()) return std::string();
    int length = WideCharToMultiByte(
        CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    if (length <= 0) return std::string();
    std::string result(static_cast<size_t>(length), '\0');
    WideCharToMultiByte(
        CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), length, nullptr, nullptr);
    return result;
}

} // namespace

int main(int argc, char** argv)
{
    SetConsoleOutputCP(CP_UTF8);

    std::string input = argc > 1 ? argv[1] : "qcheck";
    int iterations = argc > 2 ? std::atoi(argv[2]) : 100;
    int requestedPage = argc > 3 ? std::atoi(argv[3]) : 0;
    uint32_t page = requestedPage > 0 ? static_cast<uint32_t>(requestedPage) : 0;
    std::wstring pipeName = argc > 4 ? Utf8ToWide(argv[4]) : std::wstring();
    if (iterations < 1) {
        iterations = 1;
    }

    KeyroIME::NamedPipeClient client(pipeName.empty() ? nullptr : pipeName.c_str());
    std::vector<double> latencies;
    KeyroIME::IpcCandidateResponse firstResponse;
    latencies.reserve(static_cast<size_t>(iterations));

    for (int i = 0; i < iterations; ++i) {
        auto response = client.RequestCandidatesSync(input, page);
        if (!response.ok) {
            std::cerr << "lookup failed: " << response.errorMessage
                      << " (win32=" << response.errorCode << ")\n";
            return 1;
        }
        if (i == 0) {
            firstResponse = response;
        }
        latencies.push_back(response.latencyMs);
    }

    std::sort(latencies.begin(), latencies.end());
    double sum = std::accumulate(latencies.begin(), latencies.end(), 0.0);
    double avg = sum / latencies.size();
    double min = latencies.front();
    double max = latencies.back();
    double p95 = latencies[static_cast<size_t>((latencies.size() - 1) * 0.95)];

    std::cout << "KeyroIME IPC latency benchmark\n";
    std::cout << "input=" << input << " page=" << page
              << " total_pages=" << firstResponse.totalPages
              << " iterations=" << iterations << "\n";
    for (size_t index = 0; index < firstResponse.candidates.size(); ++index) {
        std::cout << (index + 1) << ". " << WideToUtf8(firstResponse.candidates[index]) << "\n";
    }
    std::cout << "min_ms=" << min << " avg_ms=" << avg
              << " p95_ms=" << p95 << " max_ms=" << max << "\n";
    std::cout << "target_lt_10ms=" << (avg < 10.0 ? "PASS" : "FAIL") << "\n";

    return avg < 10.0 ? 0 : 2;
}
