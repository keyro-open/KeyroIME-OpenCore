# KeyroIME IPC Protocol v1

This document is the canonical compatibility specification for the C++ TSF client and Rust service protocol.

## Transport

- Local Windows named pipe: `\\.\pipe\KeyroIME.Service.v1`
- Byte stream, little-endian integers, UTF-8 text
- Remote clients are rejected
- Maximum request payload: 4096 bytes
- Maximum candidates per page: 5

## Request

The fixed request header is 9 bytes:

```text
u8  request_type
u32 page
u32 payload_byte_length
u8[] payload
```

Request types:

- `1`: candidate lookup; payload is the input query
- `2`: selection record; payload is `reading<TAB>surface`
- `3`: settings update; payload is tab-separated numeric settings

## Response

The fixed response header is 8 bytes:

```text
u8  status
u8  candidate_count
u16 total_pages
u32 payload_byte_length
u8[] payload
```

For a candidate response, the payload repeats the following entry `candidate_count` times:

```text
u16 candidate_utf8_byte_length
u8[] candidate_utf8
```

Status values:

- `0`: success
- `1`: bad request
- `2`: unsupported request
- `3`: internal error

The current client accepts response payloads up to 64 KiB. Every protocol constant change must update this document and pass `tools/check_protocol_constants.py`.
