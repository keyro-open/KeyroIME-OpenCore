// Copyright (C) 2025-2026 株式会社LocalPro (LocalPro Co., Ltd.). All rights reserved.
// Brand Official Website: https://keyro.jp
//
// This file is part of KeyroIME (キーロ) OpenCore.
// GNU GPLv3に基づいて配布されます。LICENSE（英語正文）を参照してください。

#pragma once

#include <string>

namespace KeyroIME {

/// コア候補の種別タグを候補ウィンドウ用の1文字タグへ正規化します。
std::wstring NormalizeCandidateTag(const std::wstring& sourceTag);

} // namespace KeyroIME
