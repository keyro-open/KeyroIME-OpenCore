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

#include "candidate_tag.h"

namespace KeyroIME {

std::wstring NormalizeCandidateTag(const std::wstring& sourceTag)
{
    if (sourceTag == L"訳" || sourceTag == L"翻訳") {
        return L"訳";
    }
    if (sourceTag == L"名" || sourceTag == L"人" || sourceTag == L"人名" ||
        sourceTag == L"地" || sourceTag == L"地名" || sourceTag == L"駅" ||
        sourceTag == L"駅名") {
        return L"名";
    }
    return {};
}

} // namespace KeyroIME
