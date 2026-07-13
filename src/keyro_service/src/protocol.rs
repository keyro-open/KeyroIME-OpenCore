// Copyright (C) 2025-2026 株式会社LocalPro (LocalPro Co., Ltd.). All rights reserved.
// Brand Official Website: https://keyro.jp
//
// This file is part of KeyroIME (キーロ) OpenCore.
// It is source-available under the KeyroIME OpenCore Non-Commercial Source
// License 1.0. See LICENSE. Commercial use requires a separate written license
// from 株式会社LocalPro.
use std::io::{Error, ErrorKind, Result};

pub const PIPE_NAME: &str = r"\\.\pipe\KeyroIME.Service.v1";
pub const REQUEST_HEADER_LEN: usize = 9;
pub const RESPONSE_HEADER_LEN: usize = 8;
pub const PAGE_SIZE: usize = 5;
pub const MAX_INPUT_BYTES: usize = 4096;

pub const REQUEST_LOOKUP_CANDIDATES: u8 = 1;
pub const REQUEST_RECORD_SELECTION: u8 = 2;
pub const REQUEST_UPDATE_SETTINGS: u8 = 3;

pub const STATUS_OK: u8 = 0;
pub const STATUS_BAD_REQUEST: u8 = 1;
pub const STATUS_UNSUPPORTED_REQUEST: u8 = 2;
pub const STATUS_INTERNAL_ERROR: u8 = 3;

#[derive(Debug, Clone, Eq, PartialEq)]
pub struct LookupRequest {
    pub request_type: u8,
    pub page: u32,
    pub input: String,
}

pub fn decode_request(header: [u8; REQUEST_HEADER_LEN], payload: Vec<u8>) -> Result<LookupRequest> {
    let page = u32::from_le_bytes([header[1], header[2], header[3], header[4]]);
    let input_len = u32::from_le_bytes([header[5], header[6], header[7], header[8]]) as usize;
    if input_len > MAX_INPUT_BYTES {
        return Err(Error::new(
            ErrorKind::InvalidData,
            "input length exceeds limit",
        ));
    }
    if payload.len() != input_len {
        return Err(Error::new(
            ErrorKind::UnexpectedEof,
            "request payload length mismatch",
        ));
    }

    let input = String::from_utf8(payload)
        .map_err(|_| Error::new(ErrorKind::InvalidData, "request payload is not valid UTF-8"))?;

    Ok(LookupRequest {
        request_type: header[0],
        page,
        input,
    })
}

pub fn encode_candidate_response(candidates: &[String], total_pages: usize) -> Result<Vec<u8>> {
    let mut payload = Vec::new();

    for candidate in normalized_page(candidates) {
        let bytes = candidate.as_bytes();
        let len = u16::try_from(bytes.len())
            .map_err(|_| Error::new(ErrorKind::InvalidData, "candidate text is too long"))?;
        payload.extend_from_slice(&len.to_le_bytes());
        payload.extend_from_slice(bytes);
    }

    let payload_len = u32::try_from(payload.len())
        .map_err(|_| Error::new(ErrorKind::InvalidData, "response payload is too long"))?;

    let mut response = Vec::with_capacity(RESPONSE_HEADER_LEN + payload.len());
    response.push(STATUS_OK);
    response.push(PAGE_SIZE as u8);
    let total_pages = u16::try_from(total_pages.max(1))
        .map_err(|_| Error::new(ErrorKind::InvalidData, "too many candidate pages"))?;
    response.extend_from_slice(&total_pages.to_le_bytes());
    response.extend_from_slice(&payload_len.to_le_bytes());
    response.extend_from_slice(&payload);
    Ok(response)
}

pub fn encode_error_response(status: u8, message: &str) -> Vec<u8> {
    let payload = message.as_bytes();
    let payload_len = u32::try_from(payload.len()).unwrap_or(0);

    let mut response = Vec::with_capacity(RESPONSE_HEADER_LEN + payload.len());
    response.push(status);
    response.push(0);
    response.extend_from_slice(&0_u16.to_le_bytes());
    response.extend_from_slice(&payload_len.to_le_bytes());
    response.extend_from_slice(payload);
    response
}

pub fn encode_ok_response() -> Vec<u8> {
    vec![STATUS_OK, 0, 1, 0, 0, 0, 0, 0]
}

fn normalized_page(candidates: &[String]) -> Vec<String> {
    let mut page = candidates
        .iter()
        .take(PAGE_SIZE)
        .cloned()
        .collect::<Vec<_>>();
    while page.len() < PAGE_SIZE {
        page.push(String::new());
    }
    page
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn encodes_exactly_five_candidate_slots() {
        let response = encode_candidate_response(&["交換".to_string()], 1).unwrap();
        assert_eq!(response[0], STATUS_OK);
        assert_eq!(response[1], PAGE_SIZE as u8);
        assert_eq!(response.len() > RESPONSE_HEADER_LEN, true);
    }

    #[test]
    fn encodes_compact_ok_response() {
        assert_eq!(encode_ok_response(), [STATUS_OK, 0, 1, 0, 0, 0, 0, 0]);
    }
}
