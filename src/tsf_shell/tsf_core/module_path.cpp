// Copyright (C) 2025-2026 株式会社LocalPro (LocalPro Co., Ltd.). All rights reserved.
// Brand Official Website: https://keyro.jp
//
// This file is part of KeyroIME (キーロ) OpenCore.
// GNU GPLv3に基づいて配布されます。LICENSE（英語正文）を参照してください。
#include "module_path.h"

#include <vector>

namespace KeyroIME {

bool BuildModuleSiblingPath(
    HMODULE moduleHandle,
    const wchar_t* fileName,
    std::wstring& absolutePath)
{
    absolutePath.clear();
    if (!moduleHandle || !fileName || fileName[0] == L'\0') {
        return false;
    }

    std::vector<wchar_t> buffer(32768, L'\0');
    DWORD length = GetModuleFileNameW(
        moduleHandle,
        buffer.data(),
        static_cast<DWORD>(buffer.size()));
    if (length == 0 || length >= buffer.size()) {
        return false;
    }

    std::wstring modulePath(buffer.data(), length);
    size_t separator = modulePath.find_last_of(L"\\/");
    if (separator == std::wstring::npos) {
        return false;
    }

    absolutePath.assign(modulePath, 0, separator + 1);
    absolutePath.append(fileName);
    return true;
}

} // namespace KeyroIME
