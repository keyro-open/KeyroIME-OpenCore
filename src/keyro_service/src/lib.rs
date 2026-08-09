// Copyright (C) 2025-2026 株式会社LocalPro (LocalPro Co., Ltd.). All rights reserved.
// Brand Official Website: https://keyro.jp
//
// This file is part of KeyroIME (キーロ) OpenCore.
// GNU GPLv3に基づいて配布されます。LICENSE（英語正文）を参照してください。
use std::ffi::{CStr, CString};
use std::os::raw::c_char;
use std::panic::{catch_unwind, AssertUnwindSafe};

pub mod dictionary;
pub mod protocol;
pub mod ranking;
mod romaji;
#[cfg(test)]
#[allow(dead_code)]
mod state_machine;

#[cfg(windows)]
pub mod pipe_server;

#[cfg(windows)]
pub mod service_host;

use romaji::RomajiConverter;

/// ローマ字をかなへ変換します（現在は実変換エンジンを使用）。
/// 呼び出し元は使用後に `free_string` でこのポインターを解放してください。
#[no_mangle]
pub extern "C" fn match_romaji(input: *const c_char) -> *mut c_char {
    if input.is_null() {
        return std::ptr::null_mut();
    }

    // 入力ローマ字文字列を解析します。
    let input_str = unsafe {
        match CStr::from_ptr(input).to_str() {
            Ok(s) => s,
            Err(_) => return std::ptr::null_mut(),
        }
    };

    catch_unwind(AssertUnwindSafe(|| {
        let converter = RomajiConverter::new();
        let kana = converter.convert(input_str);
        CString::new(kana)
            .map(CString::into_raw)
            .unwrap_or(std::ptr::null_mut())
    }))
    .unwrap_or(std::ptr::null_mut())
}

/// 候補一覧を取得します（各ページ固定5件）。
/// 戻り値形式: '|' 区切りの候補文字列。例: "こうかん|交換|光環|交歓|後間"
/// 5件に満たない場合は空文字列で埋めます。
/// @param kana 入力かな文字列
/// @param page ページ番号（0始まり）
/// 呼び出し元は使用後に `free_string` でこのポインターを解放してください。
#[no_mangle]
pub extern "C" fn get_candidates(kana: *const c_char, page: u32) -> *mut c_char {
    if kana.is_null() {
        return std::ptr::null_mut();
    }

    // 入力かな文字列を解析します。
    let kana_str = unsafe {
        match CStr::from_ptr(kana).to_str() {
            Ok(s) => s,
            Err(_) => return std::ptr::null_mut(),
        }
    };

    catch_unwind(AssertUnwindSafe(|| {
        let candidates = ranking::CandidateSorter::re_rank(kana_str, page as usize);
        let result_str = candidates.join("|");
        CString::new(result_str)
            .map(CString::into_raw)
            .unwrap_or(std::ptr::null_mut())
    }))
    .unwrap_or(std::ptr::null_mut())
}

#[no_mangle]
pub extern "C" fn evaluate_frequency(word: *const c_char) {
    if word.is_null() {
        return;
    }

    // A tab-separated "reading<TAB>surface" value updates the dynamic user
    // dictionary. Legacy single-field callers use the same value for both.
    let _ = catch_unwind(AssertUnwindSafe(|| unsafe {
        if let Ok(value) = CStr::from_ptr(word).to_str() {
            dictionary::record_legacy_frequency(value);
        }
    }));
}

/// Rust が確保した文字列ポインターを解放し、メモリ安全性を保ちリークを防ぎます。
#[no_mangle]
pub extern "C" fn free_string(ptr: *mut c_char) {
    if !ptr.is_null() {
        unsafe {
            let _ = CString::from_raw(ptr);
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_match_romaji_and_free() {
        let input = CString::new("koukan").unwrap();
        let ptr = match_romaji(input.as_ptr());
        assert!(!ptr.is_null());

        let result_cstr = unsafe { CStr::from_ptr(ptr) };
        assert_eq!(result_cstr.to_str().unwrap(), "こうかん");

        free_string(ptr);
    }
}
