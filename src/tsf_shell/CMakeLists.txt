# Copyright (C) 2025-2026 株式会社LocalPro (LocalPro Co., Ltd.). All rights reserved.
# Brand Official Website: https://keyro.jp
#
# This file is part of KeyroIME (キーロ) OpenCore.
# It is source-available under the KeyroIME OpenCore Non-Commercial Source
# License 1.0. See LICENSE. Commercial use requires a separate written license
# from 株式会社LocalPro.
cmake_minimum_required(VERSION 3.20)
set(KEYRO_VERSION_FILE "${CMAKE_CURRENT_LIST_DIR}/../../VERSION")
file(READ "${KEYRO_VERSION_FILE}" KEYRO_VERSION)
string(STRIP "${KEYRO_VERSION}" KEYRO_VERSION)
project(KeyroIME_TSF_Shell VERSION ${KEYRO_VERSION})

set(KEYRO_GENERATED_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${KEYRO_GENERATED_INCLUDE_DIR}")
configure_file(
    "${CMAKE_CURRENT_LIST_DIR}/version.h.in"
    "${KEYRO_GENERATED_INCLUDE_DIR}/keyro_version.h"
    @ONLY
)

# C++ 標準を設定します。
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "KeyroIME.dll must be built as x64. Reconfigure CMake with -A x64.")
endif()

if(CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_CONFIGURATION_TYPES "Release" CACHE STRING "KeyroIME release configurations" FORCE)
elseif(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "KeyroIME build type" FORCE)
endif()

# TSF コアモジュールソースを追加します。
set(TSF_CORE_SOURCES
    tsf_core/module_path.cpp
    tsf_core/keyboard_layout.cpp
    tsf_core/caps_lock_hook.cpp
    tsf_core/key_event_sink.cpp
)

set(TSF_CORE_HEADERS
    tsf_core/module_path.h
    tsf_core/keyboard_layout.h
    tsf_core/caps_lock_hook.h
    tsf_core/key_event_sink.h
)

set(TSF_DLL_SOURCES
    tsf_core/tip_guid.cpp
    tsf_core/module_path.cpp
    tsf_core/keyboard_layout.cpp
    tsf_core/local_romaji.cpp
    tsf_core/shared_settings.cpp
    tsf_core/text_service.cpp
    tsf_core/class_factory.cpp
    tsf_core/dll_exports.cpp
    tsf_core/KeyroIME.def
)

set(TSF_DLL_HEADERS
    tsf_core/tip_guid.h
    tsf_core/module_path.h
    tsf_core/keyboard_layout.h
    tsf_core/local_romaji.h
    tsf_core/shared_settings.h
    tsf_core/text_service.h
    tsf_core/class_factory.h
)

# UI モジュールソースを追加します。
set(UI_SOURCES
    ui/candidate_tag.cpp
    ui/candidate_window.cpp
    ui/system_tray.cpp
    ui/status_osd.cpp
)

set(UI_HEADERS
    ui/candidate_tag.h
    ui/candidate_window.h
    ui/system_tray.h
    ui/status_osd.h
)

set(IPC_SOURCES
    ipc/named_pipe_client.cpp
)

set(IPC_HEADERS
    ipc/named_pipe_client.h
)

set(KEYRO_RESOURCE_SOURCES
    KeyroIME.rc
    resource.h
    resources/KeyroIME.ico
)

# サンドボックステストプログラムを追加します。
add_executable(keyro_test_bench 
    keyro_test_bench.cpp
    ${UI_SOURCES}
    ${UI_HEADERS}
    ${IPC_SOURCES}
    ${IPC_HEADERS}
    ${KEYRO_RESOURCE_SOURCES}
    tsf_core/tip_guid.cpp
    tsf_core/tip_guid.h
    tsf_core/shared_settings.cpp
    tsf_core/shared_settings.h
)

add_executable(keyro_tray WIN32
    tray_host.cpp
    ui/system_tray.cpp
    ui/system_tray.h
    ui/status_osd.cpp
    ui/status_osd.h
    ${IPC_SOURCES}
    ${IPC_HEADERS}
    ${KEYRO_RESOURCE_SOURCES}
    tsf_core/tip_guid.cpp
    tsf_core/tip_guid.h
    tsf_core/shared_settings.cpp
    tsf_core/shared_settings.h
)

add_executable(keyro_tray_menu_smoke
    tray_menu_smoke.cpp
    ui/system_tray.cpp
    ui/system_tray.h
    ui/status_osd.cpp
    ui/status_osd.h
    ${IPC_SOURCES}
    ${IPC_HEADERS}
    ${KEYRO_RESOURCE_SOURCES}
    tsf_core/keyboard_layout.cpp
    tsf_core/keyboard_layout.h
    tsf_core/tip_guid.cpp
    tsf_core/tip_guid.h
    tsf_core/shared_settings.cpp
    tsf_core/shared_settings.h
)
target_compile_definitions(keyro_tray_menu_smoke PRIVATE KEYRO_TRAY_TESTING)
foreach(KEYRO_VERSION_TARGET keyro_test_bench keyro_tray keyro_tray_menu_smoke)
    target_include_directories(${KEYRO_VERSION_TARGET} PRIVATE "${KEYRO_GENERATED_INCLUDE_DIR}")
endforeach()

# IPC レイテンシ基準コンソールプログラムを追加します。
add_executable(keyro_ipc_bench
    ipc_latency_bench.cpp
    ${IPC_SOURCES}
    ${IPC_HEADERS}
)

add_executable(keyro_ipc_failover_smoke
    ipc_failover_smoke.cpp
    ${IPC_SOURCES}
    ${IPC_HEADERS}
)

add_executable(keyro_tsf_activation_smoke
    tsf_activation_smoke.cpp
    tsf_core/module_path.cpp
    tsf_core/module_path.h
    tsf_core/tip_guid.cpp
    tsf_core/tip_guid.h
)

add_executable(keyro_runtime_input_smoke
    runtime_input_smoke.cpp
    tsf_core/tip_guid.cpp
    tsf_core/tip_guid.h
    tsf_core/shared_settings.cpp
    tsf_core/shared_settings.h
)

add_executable(keyro_local_fallback_smoke
    local_fallback_smoke.cpp
    tsf_core/local_romaji.cpp
    tsf_core/local_romaji.h
)

add_executable(keyro_candidate_tag_smoke
    candidate_tag_smoke.cpp
    ui/candidate_tag.cpp
    ui/candidate_tag.h
)

# 旧 PoC コンソールプログラムを追加します。
add_executable(poc_console
    main.cpp
    tsf_core/module_path.cpp
    tsf_core/module_path.h
)

# TSF 注入側 COM DLL: この段階では登録エクスポートと IClassFactory を提供します。
add_library(KeyroIME SHARED
    ${TSF_DLL_SOURCES}
    ${TSF_DLL_HEADERS}
    ${IPC_SOURCES}
    ${IPC_HEADERS}
    ${KEYRO_RESOURCE_SOURCES}
    ui/candidate_window.cpp
    ui/candidate_window.h
    ui/candidate_tag.cpp
    ui/candidate_tag.h
)

# 旧サンドボックス支援モジュールをビルドし、キーボード配列、Caps Lock、絶対パス FFI コードを継続検査します。
add_library(tsf_core STATIC ${TSF_CORE_SOURCES} ${TSF_CORE_HEADERS})
target_include_directories(tsf_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(MSVC)
    target_compile_options(tsf_core PRIVATE /utf-8)
endif()

# 独立 FFI PoC 専用。実行時は常にプログラムモジュールと同じディレクトリの絶対パスから読み込みます。
set(KEYRO_RUNTIME_DIR "${CMAKE_CURRENT_LIST_DIR}/../../target/keyro_service_runtime/x86_64-pc-windows-msvc/release")
set(RUST_DLL_SOURCE "${KEYRO_RUNTIME_DIR}/ime_core.dll")
set(RUST_PDB_SOURCE "${KEYRO_RUNTIME_DIR}/ime_core.pdb")

# Windows プラットフォーム固有設定
if(WIN32)
    # サンドボックステストプログラム: Windows GUI プログラムとして設定します。
    set_target_properties(keyro_test_bench PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
    
    # コンソールプログラムのエントリポイントを設定します。
    set_target_properties(poc_console PROPERTIES
        WIN32_EXECUTABLE FALSE  # コンソールプログラムとして維持します。
    )

    set_target_properties(keyro_ipc_bench PROPERTIES
        WIN32_EXECUTABLE FALSE
    )

    set_target_properties(keyro_ipc_failover_smoke PROPERTIES
        WIN32_EXECUTABLE FALSE
    )

    set_target_properties(keyro_tsf_activation_smoke PROPERTIES
        WIN32_EXECUTABLE FALSE
    )

    set_target_properties(keyro_runtime_input_smoke PROPERTIES
        WIN32_EXECUTABLE FALSE
    )

    set_target_properties(keyro_local_fallback_smoke PROPERTIES
        WIN32_EXECUTABLE FALSE
    )

    set_target_properties(keyro_candidate_tag_smoke PROPERTIES
        WIN32_EXECUTABLE FALSE
    )

    set_target_properties(keyro_tray_menu_smoke PROPERTIES
        WIN32_EXECUTABLE FALSE
    )
    
    # Windows SDK ライブラリを追加します。
    target_link_libraries(poc_console PRIVATE)
    target_link_libraries(keyro_test_bench PRIVATE Shell32 Advapi32 User32 Gdi32 Ole32 Uuid Comctl32 Wininet)
    target_link_libraries(keyro_tray PRIVATE Shell32 Advapi32 User32 Gdi32 Ole32 Uuid Comctl32 Wininet)
    target_link_libraries(keyro_tray_menu_smoke PRIVATE Shell32 Advapi32 User32 Gdi32 Ole32 Uuid Comctl32 Wininet)
    target_link_libraries(keyro_ipc_bench PRIVATE)
    target_link_libraries(keyro_ipc_failover_smoke PRIVATE)
    target_link_libraries(keyro_tsf_activation_smoke PRIVATE Ole32 Uuid)
    target_link_libraries(keyro_runtime_input_smoke PRIVATE Ole32 Uuid User32 Advapi32)
    target_link_libraries(keyro_local_fallback_smoke PRIVATE)
    target_link_libraries(keyro_candidate_tag_smoke PRIVATE)
    target_link_libraries(KeyroIME PRIVATE Ole32 Advapi32 Uuid)

    if(MSVC)
        target_compile_options(poc_console PRIVATE /utf-8)
        target_compile_options(keyro_test_bench PRIVATE /utf-8)
        target_compile_options(keyro_ipc_bench PRIVATE /utf-8)
        target_compile_options(keyro_ipc_failover_smoke PRIVATE /utf-8)
        target_compile_options(keyro_tsf_activation_smoke PRIVATE /utf-8)
        target_compile_options(keyro_runtime_input_smoke PRIVATE /utf-8)
        target_compile_options(keyro_local_fallback_smoke PRIVATE /utf-8)
        target_compile_options(keyro_candidate_tag_smoke PRIVATE /utf-8)
        target_compile_options(keyro_tray PRIVATE /utf-8)
        target_compile_options(keyro_tray_menu_smoke PRIVATE /utf-8)
        target_compile_options(KeyroIME PRIVATE /utf-8)
    endif()
    
    # 【重要】ビルド後に ime_core.dll を実行ファイルのディレクトリへ自動コピーします。
    add_custom_command(TARGET poc_console POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${RUST_DLL_SOURCE}"
            "$<TARGET_FILE_DIR:poc_console>/ime_core.dll"
        COMMENT "ime_core.dll を出力ディレクトリへコピーしています..."
    )
    
    # 任意: PDB デバッグシンボルもコピーします（デバッグ用）。
    add_custom_command(TARGET poc_console POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${RUST_PDB_SOURCE}"
            "$<TARGET_FILE_DIR:poc_console>/ime_core.pdb"
        COMMENT "ime_core.pdb デバッグシンボルをコピーしています..."
    )
    
    # サンドボックステストプログラムは名前付きパイプでバックエンドサービスへ接続し、ime_core.dll を直接コピー/読み込みしません。
endif()

# 表示用メッセージ
message(STATUS "Building KeyroIME PoC Console Application")
message(STATUS "Rust DLL will be auto-copied from: ${RUST_DLL_SOURCE}")
