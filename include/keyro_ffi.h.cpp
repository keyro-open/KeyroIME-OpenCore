// Copyright (C) 2025-2026 株式会社LocalPro (LocalPro Co., Ltd.). All rights reserved.
// Brand Official Website: https://keyro.jp
//
// This file is part of KeyroIME (キーロ) OpenCore.
// GNU GPLv3に基づいて配布されます。LICENSE（英語正文）を参照してください。
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
