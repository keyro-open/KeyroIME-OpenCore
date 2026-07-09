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
// tip_guid.cpp

#include "tip_guid.h"

namespace KeyroIME {

const CLSID CLSID_KeyroTextService =
    { 0x8b4f9b54, 0x7b15, 0x4d8c, { 0x9e, 0x32, 0x6d, 0x5a, 0x17, 0xb0, 0xa5, 0x1e } };

const GUID GUID_KeyroProfile =
    { 0x6c33b9a3, 0x5ce6, 0x4d27, { 0x8a, 0x9f, 0x0e, 0x9b, 0x60, 0xf0, 0xe7, 0xb9 } };

} // namespace KeyroIME
