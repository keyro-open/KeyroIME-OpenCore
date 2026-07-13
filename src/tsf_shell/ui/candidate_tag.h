// Copyright (C) 2025-2026 株式会社LocalPro (LocalPro Co., Ltd.). All rights reserved.
// Brand Official Website: https://keyro.jp
//
// This file is part of KeyroIME (キーロ) OpenCore.
// It is source-available under the KeyroIME OpenCore Non-Commercial Source
// License 1.0. See LICENSE. Commercial use requires a separate written license
// from 株式会社LocalPro.

#pragma once

#include <string>

namespace KeyroIME {

/// コア候補の種別タグを候補ウィンドウ用の1文字タグへ正規化します。
std::wstring NormalizeCandidateTag(const std::wstring& sourceTag);

} // namespace KeyroIME
