// Copyright (C) 2025-2026 株式会社LocalPro (LocalPro Co., Ltd.). All rights reserved.
// Brand Official Website: https://keyro.jp
//
// This file is part of KeyroIME (キーロ) OpenCore.
// GNU GPLv3に基づいて配布されます。LICENSE（英語正文）を参照してください。

#include "ui/candidate_tag.h"

#include <array>
#include <iostream>
#include <string>

int wmain()
{
    const std::array<std::pair<const wchar_t*, const wchar_t*>, 9> cases = {{
        {L"訳", L"訳"},
        {L"翻訳", L"訳"},
        {L"名", L"名"},
        {L"人", L"名"},
        {L"人名", L"名"},
        {L"地", L"名"},
        {L"地名", L"名"},
        {L"駅", L"名"},
        {L"駅名", L"名"},
    }};

    for (const auto& [source, expected] : cases) {
        const std::wstring actual = KeyroIME::NormalizeCandidateTag(source);
        if (actual != expected) {
            std::wcerr << L"候補タグの正規化に失敗しました: " << source << L" -> " << actual
                       << std::endl;
            return 1;
        }
    }
    if (!KeyroIME::NormalizeCandidateTag(L"未知").empty()) {
        std::wcerr << L"未知の候補タグが表示対象になりました。" << std::endl;
        return 1;
    }

    std::wcout << L"候補タグの統一テストに合格しました。" << std::endl;
    return 0;
}
