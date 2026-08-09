// Copyright (C) 2025-2026 株式会社LocalPro (LocalPro Co., Ltd.). All rights reserved.
// Brand Official Website: https://keyro.jp
//
// This file is part of KeyroIME (キーロ) OpenCore.
// GNU GPLv3に基づいて配布されます。LICENSE（英語正文）を参照してください。

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
