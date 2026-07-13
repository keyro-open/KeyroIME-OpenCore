// Copyright (C) 2025-2026 株式会社LocalPro (LocalPro Co., Ltd.). All rights reserved.
// Brand Official Website: https://keyro.jp
//
// This file is part of KeyroIME (キーロ) OpenCore.
// It is source-available under the KeyroIME OpenCore Non-Commercial Source
// License 1.0. See LICENSE. Commercial use requires a separate written license
// from 株式会社LocalPro.

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
