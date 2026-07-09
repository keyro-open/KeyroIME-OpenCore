# KeyroIME v1.0 FFI Boundary Specification

## Overview

This document defines the compatibility FFI contract between the experimental C++ caller side and the Rust core library exports in KeyroIME v1.0.

Version: v1.0
Last updated: 2026-07-09
Maintainer: Localpro株式会社

The production TSF path uses the local named-pipe service protocol. These FFI exports are kept for compatibility experiments and smoke tests.

## Architecture

```text
C++ caller
  - loads the Rust cdylib when explicitly used for compatibility testing
  - copies returned strings immediately
  - releases returned buffers through free_string

Rust cdylib
  - converts romaji to kana
  - returns candidate strings
  - owns allocation and release of returned C strings
```

## Exported Functions

### `match_romaji`

```c
char* match_romaji(const char* input);
```

Rust export:

```rust
#[no_mangle]
pub extern "C" fn match_romaji(input: *const c_char) -> *mut c_char
```

Behavior:

- Converts a UTF-8 null-terminated romaji string to hiragana.
- Returns a Rust-allocated UTF-8 C string.
- Returns `nullptr` on invalid input or allocation failure.
- The caller must release the returned pointer with `free_string`.

Example:

```cpp
char* kana = match_romaji("koukan");
// kana == "こうかん"
free_string(kana);
```

### `get_candidates`

```c
char* get_candidates(const char* kana, unsigned int page);
```

Rust export:

```rust
#[no_mangle]
pub extern "C" fn get_candidates(kana: *const c_char, page: u32) -> *mut c_char
```

Behavior:

- Returns candidate text for a UTF-8 kana/query string and zero-based page index.
- Returns a `|` separated UTF-8 string with up to five candidate slots.
- Returns `nullptr` on invalid input or allocation failure.
- The caller must release the returned pointer with `free_string`.

Example:

```cpp
char* candidates = get_candidates("こうかん", 0);
// candidates may contain "こうかん|交換|..."
free_string(candidates);
```

Prefix behavior:

- `q` promotes translation candidates.
- `v` promotes name, place, and station candidates.

### `free_string`

```c
void free_string(char* ptr);
```

Rust export:

```rust
#[no_mangle]
pub extern "C" fn free_string(ptr: *mut c_char)
```

Rules:

- Call exactly once for every non-null pointer returned by Rust.
- Passing `nullptr` is safe.
- Do not release Rust-owned pointers with `free`, `delete`, or any other allocator.
- Do not access the pointer after release.

## Memory Safety

Correct usage:

```cpp
char* result = match_romaji("koukan");
if (result) {
    std::string kana(result);
    free_string(result);
    // Use kana after copying.
}
```

Incorrect usage:

```cpp
char* result = match_romaji("koukan");
free_string(result);
const char* dangling = result; // invalid
```

## Threading

Current v1.0 compatibility exports are intended for simple same-thread calls. Production TSF communication uses the service process and named-pipe protocol instead.

## Error Handling

| Function | Success | Failure |
| --- | --- | --- |
| `match_romaji` | valid pointer | `nullptr` |
| `get_candidates` | valid pointer | `nullptr` |
| `free_string` | none | none |

Common failure cases:

1. Null input pointer.
2. Invalid UTF-8 input.
3. Allocation failure.

## Build Requirements

- Rust `cdylib` and `rlib` outputs.
- C++ caller built with MSVC-compatible x64 toolchain.
- Target: `x86_64-pc-windows-msvc`.
