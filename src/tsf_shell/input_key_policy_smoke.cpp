// Copyright (C) 2025-2026 株式会社LocalPro (LocalPro Co., Ltd.). All rights reserved.
// Brand Official Website: https://keyro.jp
//
// This file is part of KeyroIME (キーロ) OpenCore.
// GNU GPLv3に基づいて配布されます。LICENSE（英語正文）を参照してください。

#include <iostream>

#include "tsf_core/input_key_policy.h"

using KeyroIME::PunctuationKeyAction;
using KeyroIME::ResolvePunctuationKeyAction;

int main()
{
    bool passed =
        ResolvePunctuationKeyAction(VK_OEM_COMMA, false, true) ==
            PunctuationKeyAction::PagePrevious &&
        ResolvePunctuationKeyAction(VK_OEM_PERIOD, false, true) ==
            PunctuationKeyAction::PageNext &&
        ResolvePunctuationKeyAction(VK_OEM_COMMA, false, false) ==
            PunctuationKeyAction::CommitSymbol &&
        ResolvePunctuationKeyAction(VK_OEM_PERIOD, false, false) ==
            PunctuationKeyAction::CommitSymbol &&
        ResolvePunctuationKeyAction(VK_OEM_COMMA, true, true) ==
            PunctuationKeyAction::CommitSymbol &&
        ResolvePunctuationKeyAction(VK_OEM_PERIOD, true, true) ==
            PunctuationKeyAction::CommitSymbol &&
        ResolvePunctuationKeyAction(VK_PRIOR, false, false) ==
            PunctuationKeyAction::PagePrevious &&
        ResolvePunctuationKeyAction(VK_NEXT, false, false) ==
            PunctuationKeyAction::PageNext;

    if (!passed) {
        std::cerr << "punctuation key policy smoke failed" << std::endl;
        return 1;
    }
    std::cout << "punctuation key policy smoke passed" << std::endl;
    return 0;
}
