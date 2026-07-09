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
use std::env;
use std::ffi::c_void;
use std::io::{Error, ErrorKind, Result};
use std::mem::size_of;
use std::panic::{catch_unwind, AssertUnwindSafe};
use std::ptr::null_mut;
use std::sync::{Arc, OnceLock, RwLock};
use std::thread;

use crate::dictionary;
use crate::protocol::{
    decode_request, encode_candidate_response, encode_error_response, encode_ok_response,
    LookupRequest, MAX_INPUT_BYTES, PIPE_NAME, REQUEST_HEADER_LEN, REQUEST_LOOKUP_CANDIDATES,
    REQUEST_RECORD_SELECTION, REQUEST_UPDATE_SETTINGS, STATUS_BAD_REQUEST, STATUS_INTERNAL_ERROR,
    STATUS_UNSUPPORTED_REQUEST,
};
use crate::ranking::CandidateSorter;

type Bool = i32;
type Dword = u32;
type Handle = isize;

const FALSE: Bool = 0;
const INVALID_HANDLE_VALUE: Handle = -1isize;

const PIPE_ACCESS_DUPLEX: Dword = 0x0000_0003;
const PIPE_TYPE_BYTE: Dword = 0x0000_0000;
const PIPE_READMODE_BYTE: Dword = 0x0000_0000;
const PIPE_WAIT: Dword = 0x0000_0000;
const PIPE_UNLIMITED_INSTANCES: Dword = 255;

const ERROR_PIPE_CONNECTED: Dword = 535;
const SDDL_REVISION_1: Dword = 1;

const PIPE_BUFFER_BYTES: Dword = 8192;
const PIPE_DEFAULT_TIMEOUT_MS: Dword = 10;
const PIPE_WORKER_COUNT: usize = 16;
const PIPE_WORKER_STACK_BYTES: usize = 256 * 1024;

#[allow(dead_code)]
#[derive(Debug, Clone, Copy)]
struct ServiceSettings {
    input_mode: u8,
    char_width: u8,
    keyboard_layout: u8,
    punctuation_style: u8,
    english_case: u8,
}

static SERVICE_SETTINGS: OnceLock<RwLock<ServiceSettings>> = OnceLock::new();

#[repr(C)]
struct SecurityAttributes {
    n_length: Dword,
    lp_security_descriptor: *mut c_void,
    b_inherit_handle: Bool,
}

#[link(name = "advapi32")]
unsafe extern "system" {
    fn ConvertStringSecurityDescriptorToSecurityDescriptorW(
        string_security_descriptor: *const u16,
        string_s_d_revision: Dword,
        security_descriptor: *mut *mut c_void,
        security_descriptor_size: *mut Dword,
    ) -> Bool;
}

#[link(name = "kernel32")]
unsafe extern "system" {
    fn CloseHandle(handle: Handle) -> Bool;
    fn ConnectNamedPipe(named_pipe: Handle, overlapped: *mut c_void) -> Bool;
    fn CreateNamedPipeW(
        name: *const u16,
        open_mode: Dword,
        pipe_mode: Dword,
        max_instances: Dword,
        out_buffer_size: Dword,
        in_buffer_size: Dword,
        default_timeout: Dword,
        security_attributes: *mut SecurityAttributes,
    ) -> Handle;
    fn DisconnectNamedPipe(named_pipe: Handle) -> Bool;
    fn FlushFileBuffers(file: Handle) -> Bool;
    fn GetLastError() -> Dword;
    fn LocalFree(memory: *mut c_void) -> *mut c_void;
    fn ReadFile(
        file: Handle,
        buffer: *mut c_void,
        bytes_to_read: Dword,
        bytes_read: *mut Dword,
        overlapped: *mut c_void,
    ) -> Bool;
    fn WriteFile(
        file: Handle,
        buffer: *const c_void,
        bytes_to_write: Dword,
        bytes_written: *mut Dword,
        overlapped: *mut c_void,
    ) -> Bool;
}

pub fn run() -> Result<()> {
    dictionary::warm_up();
    let pipe_name_text =
        env::var("KEYROIME_TEST_PIPE_NAME").unwrap_or_else(|_| PIPE_NAME.to_string());
    let pipe_name = Arc::new(to_wide_null(&pipe_name_text));

    println!("KeyroIME backend service listening on {pipe_name_text}");

    for worker_index in 1..PIPE_WORKER_COUNT {
        let worker_pipe_name = Arc::clone(&pipe_name);
        thread::Builder::new()
            .name(format!("keyro-pipe-worker-{worker_index}"))
            .stack_size(PIPE_WORKER_STACK_BYTES)
            .spawn(move || {
                if let Err(error) = run_pipe_worker(&worker_pipe_name) {
                    eprintln!("named pipe worker stopped: {error}");
                }
            })?;
    }

    run_pipe_worker(&pipe_name)
}

fn run_pipe_worker(pipe_name: &[u16]) -> Result<()> {
    let security_descriptor = SecurityDescriptor::new_appcontainer_pipe_acl()?;
    loop {
        let pipe = create_pipe(pipe_name, &security_descriptor)?;
        if let Err(error) = connect_pipe(pipe.raw()) {
            eprintln!("named pipe connect failed: {error}");
            continue;
        }

        if let Err(error) = handle_client(pipe) {
            eprintln!("named pipe client closed: {error}");
        }
    }
}

fn create_pipe(pipe_name: &[u16], security_descriptor: &SecurityDescriptor) -> Result<PipeHandle> {
    let mut security_attributes = SecurityAttributes {
        n_length: size_of::<SecurityAttributes>() as Dword,
        lp_security_descriptor: security_descriptor.as_ptr(),
        b_inherit_handle: FALSE,
    };

    let handle = unsafe {
        CreateNamedPipeW(
            pipe_name.as_ptr(),
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
            PIPE_UNLIMITED_INSTANCES,
            PIPE_BUFFER_BYTES,
            PIPE_BUFFER_BYTES,
            PIPE_DEFAULT_TIMEOUT_MS,
            &mut security_attributes,
        )
    };

    if handle == INVALID_HANDLE_VALUE {
        Err(Error::last_os_error())
    } else {
        Ok(PipeHandle(handle))
    }
}

fn connect_pipe(handle: Handle) -> Result<()> {
    let connected = unsafe { ConnectNamedPipe(handle, null_mut()) };
    if connected != FALSE {
        return Ok(());
    }

    let error_code = unsafe { GetLastError() };
    if error_code == ERROR_PIPE_CONNECTED {
        Ok(())
    } else {
        Err(Error::from_raw_os_error(error_code as i32))
    }
}

fn handle_client(pipe: PipeHandle) -> Result<()> {
    loop {
        let request = match read_request(pipe.raw()) {
            Ok(request) => request,
            Err(error) if error.kind() == ErrorKind::UnexpectedEof => break,
            Err(error) => {
                let response = encode_error_response(STATUS_BAD_REQUEST, &error.to_string());
                write_all(pipe.raw(), &response)?;
                continue;
            }
        };

        let response = dispatch_request(request).unwrap_or_else(|error| {
            encode_error_response(STATUS_INTERNAL_ERROR, &error.to_string())
        });
        write_all(pipe.raw(), &response)?;
    }

    unsafe {
        FlushFileBuffers(pipe.raw());
        DisconnectNamedPipe(pipe.raw());
    }
    Ok(())
}

fn read_request(handle: Handle) -> Result<LookupRequest> {
    let mut header = [0_u8; REQUEST_HEADER_LEN];
    read_exact(handle, &mut header)?;

    let input_len = u32::from_le_bytes([header[5], header[6], header[7], header[8]]) as usize;
    if input_len > MAX_INPUT_BYTES {
        return Err(Error::new(
            ErrorKind::InvalidData,
            "input length exceeds limit",
        ));
    }

    let mut payload = vec![0_u8; input_len];
    read_exact(handle, &mut payload)?;
    decode_request(header, payload)
}

fn dispatch_request(request: LookupRequest) -> Result<Vec<u8>> {
    if request.request_type == REQUEST_RECORD_SELECTION {
        let Some((reading, text)) = request.input.split_once('\t') else {
            return Ok(encode_error_response(
                STATUS_BAD_REQUEST,
                "selection payload is missing separator",
            ));
        };
        dictionary::record_selection(reading, text);
        return Ok(encode_ok_response());
    }

    if request.request_type == REQUEST_UPDATE_SETTINGS {
        let fields = request.input.split('\t').collect::<Vec<_>>();
        if fields.len() != 3 && fields.len() != 5 {
            return Ok(encode_error_response(
                STATUS_BAD_REQUEST,
                "settings payload is invalid",
            ));
        }
        let parsed = fields
            .iter()
            .map(|value| value.parse::<u8>().ok())
            .collect::<Vec<_>>();
        if parsed.iter().any(Option::is_none) {
            return Ok(encode_error_response(
                STATUS_BAD_REQUEST,
                "settings payload is invalid",
            ));
        }
        let input_mode = parsed[0].unwrap_or_default();
        let char_width = parsed[1].unwrap_or_default();
        let keyboard_layout = parsed[2].unwrap_or_default();
        let punctuation_style = parsed.get(3).and_then(|value| *value).unwrap_or(0);
        let english_case = parsed.get(4).and_then(|value| *value).unwrap_or(0);
        if input_mode > 2
            || char_width > 1
            || keyboard_layout > 1
            || punctuation_style > 1
            || english_case > 1
        {
            return Ok(encode_error_response(
                STATUS_BAD_REQUEST,
                "settings value is out of range",
            ));
        }
        let settings = SERVICE_SETTINGS.get_or_init(|| {
            RwLock::new(ServiceSettings {
                input_mode: 0,
                char_width: 0,
                keyboard_layout: 0,
                punctuation_style: 0,
                english_case: 0,
            })
        });
        if let Ok(mut guard) = settings.write() {
            *guard = ServiceSettings {
                input_mode,
                char_width,
                keyboard_layout,
                punctuation_style,
                english_case,
            };
        }
        return Ok(encode_ok_response());
    }

    if request.request_type != REQUEST_LOOKUP_CANDIDATES {
        return Ok(encode_error_response(
            STATUS_UNSUPPORTED_REQUEST,
            "unsupported request type",
        ));
    }

    let page = catch_unwind(AssertUnwindSafe(|| {
        CandidateSorter::rank_page(&request.input, request.page as usize)
    }))
    .map_err(|_| Error::new(ErrorKind::Other, "candidate ranking panicked"))?;

    encode_candidate_response(&page.candidates, page.total_pages)
}

fn read_exact(handle: Handle, buffer: &mut [u8]) -> Result<()> {
    let mut offset = 0;
    while offset < buffer.len() {
        let mut bytes_read = 0;
        let ok = unsafe {
            ReadFile(
                handle,
                buffer[offset..].as_mut_ptr().cast(),
                (buffer.len() - offset) as Dword,
                &mut bytes_read,
                null_mut(),
            )
        };

        if ok == FALSE {
            return Err(Error::last_os_error());
        }
        if bytes_read == 0 {
            return Err(Error::new(ErrorKind::UnexpectedEof, "pipe closed"));
        }

        offset += bytes_read as usize;
    }
    Ok(())
}

fn write_all(handle: Handle, buffer: &[u8]) -> Result<()> {
    let mut offset = 0;
    while offset < buffer.len() {
        let mut bytes_written = 0;
        let ok = unsafe {
            WriteFile(
                handle,
                buffer[offset..].as_ptr().cast(),
                (buffer.len() - offset) as Dword,
                &mut bytes_written,
                null_mut(),
            )
        };

        if ok == FALSE {
            return Err(Error::last_os_error());
        }
        if bytes_written == 0 {
            return Err(Error::new(
                ErrorKind::WriteZero,
                "pipe write returned zero bytes",
            ));
        }

        offset += bytes_written as usize;
    }
    Ok(())
}

struct PipeHandle(Handle);

impl PipeHandle {
    fn raw(&self) -> Handle {
        self.0
    }
}

impl Drop for PipeHandle {
    fn drop(&mut self) {
        if self.0 != INVALID_HANDLE_VALUE {
            unsafe {
                CloseHandle(self.0);
            }
        }
    }
}

struct SecurityDescriptor {
    ptr: *mut c_void,
}

impl SecurityDescriptor {
    fn new_appcontainer_pipe_acl() -> Result<Self> {
        // S-1-15-2-1 is ALL_APPLICATION_PACKAGES. GA covers duplex pipe read/write
        // plus synchronization rights required by CreateFile(GENERIC_READ|GENERIC_WRITE).
        // The low-integrity mandatory label allows AppContainer/low IL frontends
        // to write to the pipe without tripping Windows MIC write-up checks.
        let sddl = concat!(
            "D:P",
            "(A;;GA;;;SY)",
            "(A;;GA;;;BA)",
            "(A;;GA;;;AU)",
            "(A;;GA;;;IU)",
            "(A;;GA;;;S-1-15-2-1)",
            "(A;;GA;;;S-1-15-2-2)",
            "S:(ML;;NW;;;LW)"
        );
        let wide_sddl = to_wide_null(sddl);
        let mut descriptor = null_mut();

        let ok = unsafe {
            ConvertStringSecurityDescriptorToSecurityDescriptorW(
                wide_sddl.as_ptr(),
                SDDL_REVISION_1,
                &mut descriptor,
                null_mut(),
            )
        };

        if ok == FALSE || descriptor.is_null() {
            Err(Error::last_os_error())
        } else {
            Ok(Self { ptr: descriptor })
        }
    }

    fn as_ptr(&self) -> *mut c_void {
        self.ptr
    }
}

impl Drop for SecurityDescriptor {
    fn drop(&mut self) {
        if !self.ptr.is_null() {
            unsafe {
                LocalFree(self.ptr);
            }
        }
    }
}

fn to_wide_null(value: &str) -> Vec<u16> {
    value.encode_utf16().chain(Some(0)).collect()
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::protocol::{LookupRequest, STATUS_OK};

    #[test]
    fn settings_update_request_persists_service_state() {
        let response = dispatch_request(LookupRequest {
            request_type: REQUEST_UPDATE_SETTINGS,
            page: 0,
            input: "2\t1\t1\t1\t1".to_string(),
        })
        .unwrap();
        assert_eq!(response[0], STATUS_OK);

        let settings = SERVICE_SETTINGS
            .get()
            .expect("settings should be initialized")
            .read()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        assert_eq!(settings.input_mode, 2);
        assert_eq!(settings.char_width, 1);
        assert_eq!(settings.keyboard_layout, 1);
        assert_eq!(settings.punctuation_style, 1);
        assert_eq!(settings.english_case, 1);
    }

    #[test]
    fn settings_update_request_rejects_out_of_range_values() {
        let response = dispatch_request(LookupRequest {
            request_type: REQUEST_UPDATE_SETTINGS,
            page: 0,
            input: "3\t0\t0".to_string(),
        })
        .unwrap();
        assert_eq!(response[0], STATUS_BAD_REQUEST);
    }
}
