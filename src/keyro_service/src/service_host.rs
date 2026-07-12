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
use std::ffi::c_void;
use std::io::{Error, Result};
use std::ptr::null_mut;

use crate::pipe_server;

type Bool = i32;
type Dword = u32;
type ServiceStatusHandle = isize;

const FALSE: Bool = 0;
const NO_ERROR: Dword = 0;
const ERROR_FAILED_SERVICE_CONTROLLER_CONNECT: i32 = 1063;

const SERVICE_WIN32_OWN_PROCESS: Dword = 0x0000_0010;
const SERVICE_STOPPED: Dword = 0x0000_0001;
const SERVICE_START_PENDING: Dword = 0x0000_0002;
const SERVICE_STOP_PENDING: Dword = 0x0000_0003;
const SERVICE_RUNNING: Dword = 0x0000_0004;
const SERVICE_ACCEPT_STOP: Dword = 0x0000_0001;
const SERVICE_ACCEPT_SHUTDOWN: Dword = 0x0000_0004;
const SERVICE_CONTROL_STOP: Dword = 0x0000_0001;
const SERVICE_CONTROL_SHUTDOWN: Dword = 0x0000_0005;

static SERVICE_NAME: [u16; 17] = [
    b'K' as u16,
    b'e' as u16,
    b'y' as u16,
    b'r' as u16,
    b'o' as u16,
    b'I' as u16,
    b'M' as u16,
    b'E' as u16,
    b'_' as u16,
    b'S' as u16,
    b'e' as u16,
    b'r' as u16,
    b'v' as u16,
    b'i' as u16,
    b'c' as u16,
    b'e' as u16,
    0,
];

static mut SERVICE_STATUS_HANDLE_VALUE: ServiceStatusHandle = 0;

#[repr(C)]
struct ServiceStatus {
    service_type: Dword,
    current_state: Dword,
    controls_accepted: Dword,
    win32_exit_code: Dword,
    service_specific_exit_code: Dword,
    check_point: Dword,
    wait_hint: Dword,
}

#[repr(C)]
struct ServiceTableEntry {
    service_name: *mut u16,
    service_proc: Option<unsafe extern "system" fn(Dword, *mut *mut u16)>,
}

#[link(name = "advapi32")]
unsafe extern "system" {
    fn RegisterServiceCtrlHandlerExW(
        service_name: *const u16,
        handler_proc: Option<
            unsafe extern "system" fn(Dword, Dword, *mut c_void, *mut c_void) -> Dword,
        >,
        context: *mut c_void,
    ) -> ServiceStatusHandle;
    fn SetServiceStatus(
        service_status_handle: ServiceStatusHandle,
        service_status: *mut ServiceStatus,
    ) -> Bool;
    fn StartServiceCtrlDispatcherW(service_start_table: *const ServiceTableEntry) -> Bool;
}

pub fn run_service_or_console() -> Result<()> {
    let mut service_name = SERVICE_NAME;
    let table = [
        ServiceTableEntry {
            service_name: service_name.as_mut_ptr(),
            service_proc: Some(service_main),
        },
        ServiceTableEntry {
            service_name: null_mut(),
            service_proc: None,
        },
    ];

    let ok = unsafe { StartServiceCtrlDispatcherW(table.as_ptr()) };
    if ok != FALSE {
        return Ok(());
    }

    let error = Error::last_os_error();
    if error.raw_os_error() == Some(ERROR_FAILED_SERVICE_CONTROLLER_CONNECT) {
        return pipe_server::run();
    }

    Err(error)
}

unsafe extern "system" fn service_main(_argc: Dword, _argv: *mut *mut u16) {
    SERVICE_STATUS_HANDLE_VALUE = RegisterServiceCtrlHandlerExW(
        SERVICE_NAME.as_ptr(),
        Some(service_control_handler),
        null_mut(),
    );

    if SERVICE_STATUS_HANDLE_VALUE == 0 {
        return;
    }

    set_service_status(SERVICE_START_PENDING, 0, 3_000);
    set_service_status(
        SERVICE_RUNNING,
        SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN,
        0,
    );

    let _ = pipe_server::run();

    set_service_status(SERVICE_STOPPED, 0, 0);
}

unsafe extern "system" fn service_control_handler(
    control: Dword,
    _event_type: Dword,
    _event_data: *mut c_void,
    _context: *mut c_void,
) -> Dword {
    if control == SERVICE_CONTROL_STOP || control == SERVICE_CONTROL_SHUTDOWN {
        set_service_status(SERVICE_STOP_PENDING, 0, 3_000);
        set_service_status(SERVICE_STOPPED, 0, 0);
        std::process::exit(0);
    }

    NO_ERROR
}

fn set_service_status(current_state: Dword, controls_accepted: Dword, wait_hint: Dword) {
    let mut status = ServiceStatus {
        service_type: SERVICE_WIN32_OWN_PROCESS,
        current_state,
        controls_accepted,
        win32_exit_code: NO_ERROR,
        service_specific_exit_code: 0,
        check_point: 0,
        wait_hint,
    };

    unsafe {
        if SERVICE_STATUS_HANDLE_VALUE != 0 {
            SetServiceStatus(SERVICE_STATUS_HANDLE_VALUE, &mut status);
        }
    }
}
