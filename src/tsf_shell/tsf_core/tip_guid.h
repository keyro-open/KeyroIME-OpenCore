// Copyright (C) 2025-2026 株式会社LocalPro (LocalPro Co., Ltd.). All rights reserved.
// Brand Official Website: https://keyro.jp
//
// This file is part of KeyroIME (キーロ) OpenCore.
// GNU GPLv3に基づいて配布されます。LICENSE（英語正文）を参照してください。
// tip_guid.h
// Stable COM and TSF profile GUIDs for KeyroIME.

#pragma once

#include <windows.h>

namespace KeyroIME {

extern const CLSID CLSID_KeyroTextService;
extern const GUID GUID_KeyroProfile;

constexpr wchar_t kTextServiceClsidString[] = L"{8B4F9B54-7B15-4D8C-9E32-6D5A17B0A51E}";
constexpr wchar_t kProfileGuidString[] = L"{6C33B9A3-5CE6-4D27-8A9F-0E9B60F0E7B9}";
constexpr wchar_t kJapaneseLangIdString[] = L"0x00000411";

constexpr wchar_t kTipKeyboardCategoryString[] = L"{34745C63-B2F0-4784-8B67-5E12C8701A31}";
constexpr wchar_t kTipSecureModeCategoryString[] = L"{49D2F9CE-1F5E-11D7-A6D3-00065B84435C}";
constexpr wchar_t kTipUiElementCategoryString[] = L"{49D2F9CF-1F5E-11D7-A6D3-00065B84435C}";

} // namespace KeyroIME
