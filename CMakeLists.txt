# Copyright (C) 2025-2026 株式会社LocalPro (LocalPro Co., Ltd.). All rights reserved.
# Brand Official Website: https://keyro.jp
#
# This file is part of KeyroIME (キーロ) v1.0 OpenCore.
# KeyroIME is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software Foundation.
#
# For commercial use licensing, custom deployment, or proprietary integrations,
# please contact 株式会社LocalPro via https://localpro.jp. Unauthorized closed-source
# commercial exploitation is strictly prohibited.
#
# KeyroIME v1.0 トップレベル CMake ビルド設定
# ハイブリッド構成: C++ TSF シェル + Rust バックグラウンドサービス/コアエンジン

cmake_minimum_required(VERSION 3.20)
project(KeyroIME VERSION 1.0.0 LANGUAGES CXX)

# グローバル C++ 標準を設定します。
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "KeyroIME v1.0 requires a 64-bit x64 C++ toolchain.")
endif()

if(CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_CONFIGURATION_TYPES "Release" CACHE STRING "KeyroIME release configurations" FORCE)
elseif(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "KeyroIME build type" FORCE)
endif()

# Rust バックグラウンドサービスの Release x64 成果物パスを定義します（プロジェクトルートからの相対位置）。
set(KEYRO_SERVICE_EXE_PATH "${CMAKE_SOURCE_DIR}/target/keyro_service_runtime/x86_64-pc-windows-msvc/release/keyro_service.exe")

# 構成情報を出力します。
message(STATUS "=================================================")
message(STATUS "KeyroIME v1.0 ハイブリッドビルドシステム")
message(STATUS "=================================================")
message(STATUS "C++ 標準: ${CMAKE_CXX_STANDARD}")
message(STATUS "C++ アーキテクチャ: x64")
message(STATUS "ビルド種別: Release")
message(STATUS "Rust サービスパス: ${KEYRO_SERVICE_EXE_PATH}")
message(STATUS "=================================================")

# TSF シェルのサブディレクトリを追加します。
add_subdirectory(src/tsf_shell)

# グローバル通知: Rust バックグラウンドサービスを先にビルドしてください。
if(NOT EXISTS ${KEYRO_SERVICE_EXE_PATH})
    message(WARNING "keyro_service.exe が見つかりません。先に実行してください: cargo build --release --target x86_64-pc-windows-msvc --manifest-path src/keyro_service/Cargo.toml --target-dir target/keyro_service_runtime")
endif()
