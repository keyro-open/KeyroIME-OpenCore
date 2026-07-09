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
#pragma once
#include <windows.h>

extern "C" {
    // ローマ字をリアルタイムでかなへ変換します。
    char* match_romaji(const char* input);
    
    // かなと入力モードに基づいて候補一覧を取得します（JSON 文字列または区切り文字列を返します）。
    char* get_candidates(const char* kana, unsigned char mode);
    
    // 語句の使用頻度を評価し、候補順位の更新に利用します。
    void evaluate_frequency(const char* word);
    
    // Rust 側で確保された文字列を C++ 側が誤って解放しないよう、専用の解放関数を提供します。
    void free_string(char* ptr);
}
