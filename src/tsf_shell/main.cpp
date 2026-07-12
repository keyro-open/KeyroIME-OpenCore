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
#include <windows.h>
#include <iostream>
#include <string>

#include "tsf_core/module_path.h"

// FFI 関数ポインター型定義
typedef char* (*MatchRomajiFunc)(const char*);
typedef void (*FreeStringFunc)(char*);

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    std::wstring rustDllPath;
    if (!KeyroIME::BuildModuleSiblingPath(
            GetModuleHandleW(nullptr),
            L"ime_core.dll",
            rustDllPath)) {
        std::cerr << "Failed to resolve ime_core.dll absolute path." << std::endl;
        return 1;
    }

    // 1. 現在のプログラムと同じディレクトリにある ime_core.dll を絶対パスで動的ロードします。
    HMODULE hDll = LoadLibraryW(rustDllPath.c_str());
    if (!hDll) {
        std::cerr << "Failed to load ime_core.dll. Error code: " 
                  << GetLastError() << std::endl;
        return 1;
    }

    // 2. 取得関数ポインター
    auto match_romaji = (MatchRomajiFunc)GetProcAddress(hDll, "match_romaji");
    auto free_string = (FreeStringFunc)GetProcAddress(hDll, "free_string");

    if (!match_romaji || !free_string) {
        std::cerr << "Failed to get function pointers." << std::endl;
        FreeLibrary(hDll);
        return 1;
    }

    // 3. 呼び出し match_romaji
    const char* input = "koukan";
    char* result = match_romaji(input);
    
    if (result) {
        std::cout << "Input: " << input << std::endl;
        std::cout << "Output: " << result << std::endl;
        
        // 4. 【重要】free_string を呼び出して Rust が確保したメモリを解放します。
        free_string(result);
    } else {
        std::cerr << "match_romaji returned nullptr" << std::endl;
    }

    // 5. アンロード DLL
    FreeLibrary(hDll);
    
    std::cout << "FFI memory safety test passed!" << std::endl;
    return 0;
}
