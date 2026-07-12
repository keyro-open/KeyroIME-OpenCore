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
#pragma once

#include <windows.h>

#include <string>

namespace KeyroIME {

HMODULE DllModuleHandle();

bool BuildModuleSiblingPath(
    HMODULE moduleHandle,
    const wchar_t* fileName,
    std::wstring& absolutePath);

} // namespace KeyroIME
