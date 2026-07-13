// Copyright (C) 2025-2026 株式会社LocalPro (LocalPro Co., Ltd.). All rights reserved.
// Brand Official Website: https://keyro.jp
//
// This file is part of KeyroIME (キーロ) OpenCore.
// It is source-available under the KeyroIME OpenCore Non-Commercial Source
// License 1.0. See LICENSE. Commercial use requires a separate written license
// from 株式会社LocalPro.
#[cfg(windows)]
fn main() -> std::io::Result<()> {
    ime_core::service_host::run_service_or_console()
}

#[cfg(not(windows))]
fn main() {
    eprintln!("keyro_service is only supported on Windows.");
}
