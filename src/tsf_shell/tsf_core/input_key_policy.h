// Copyright (C) 2025-2026 株式会社LocalPro (LocalPro Co., Ltd.). All rights reserved.
// Brand Official Website: https://keyro.jp
//
// This file is part of KeyroIME (キーロ) OpenCore.
// GNU GPLv3に基づいて配布されます。LICENSE（英語正文）を参照してください。

#pragma once

#include <windows.h>

namespace KeyroIME {

enum class PunctuationKeyAction {
    None,
    PagePrevious,
    PageNext,
    CommitSymbol,
};

inline PunctuationKeyAction ResolvePunctuationKeyAction(
    WPARAM wParam,
    bool shiftPressed,
    bool hasCandidatePages)
{
    if (wParam == VK_PRIOR) {
        return PunctuationKeyAction::PagePrevious;
    }
    if (wParam == VK_NEXT) {
        return PunctuationKeyAction::PageNext;
    }
    if (wParam != VK_OEM_COMMA && wParam != VK_OEM_PERIOD) {
        return PunctuationKeyAction::None;
    }
    if (!shiftPressed && hasCandidatePages) {
        return wParam == VK_OEM_COMMA
            ? PunctuationKeyAction::PagePrevious
            : PunctuationKeyAction::PageNext;
    }
    return PunctuationKeyAction::CommitSymbol;
}

} // namespace KeyroIME
