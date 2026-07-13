"""Verify that the Rust and C++ IPC v1 constants remain compatible.

Copyright (C) 2025-2026 LocalPro Co., Ltd. All rights reserved.
Source-available under the KeyroIME OpenCore Non-Commercial Source License 1.0.
Commercial use requires a separate written license from LocalPro Co., Ltd.
"""

from __future__ import annotations

import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
RUST_PROTOCOL = ROOT / "src/keyro_service/src/protocol.rs"
CPP_CLIENT = ROOT / "src/tsf_shell/ipc/named_pipe_client.h"


def extract(pattern: str, text: str, label: str) -> str:
    match = re.search(pattern, text)
    if not match:
        raise SystemExit(f"missing protocol constant: {label}")
    return match.group(1)


def main() -> None:
    rust = RUST_PROTOCOL.read_text(encoding="utf-8")
    cpp = CPP_CLIENT.read_text(encoding="utf-8")

    comparisons = {
        "request header length": (
            r"REQUEST_HEADER_LEN:\s*usize\s*=\s*(\d+)",
            r"REQUEST_HEADER_LEN\s*=\s*(\d+)",
        ),
        "response header length": (
            r"RESPONSE_HEADER_LEN:\s*usize\s*=\s*(\d+)",
            r"RESPONSE_HEADER_LEN\s*=\s*(\d+)",
        ),
        "page size": (
            r"PAGE_SIZE:\s*usize\s*=\s*(\d+)",
            r"MAX_CANDIDATE_COUNT\s*=\s*(\d+)",
        ),
        "maximum input bytes": (
            r"MAX_INPUT_BYTES:\s*usize\s*=\s*(\d+)",
            r"MAX_INPUT_BYTES\s*=\s*(\d+)",
        ),
        "lookup request type": (
            r"REQUEST_LOOKUP_CANDIDATES:\s*u8\s*=\s*(\d+)",
            r"REQUEST_LOOKUP_CANDIDATES\s*=\s*(\d+)",
        ),
        "selection request type": (
            r"REQUEST_RECORD_SELECTION:\s*u8\s*=\s*(\d+)",
            r"REQUEST_RECORD_SELECTION\s*=\s*(\d+)",
        ),
        "settings request type": (
            r"REQUEST_UPDATE_SETTINGS:\s*u8\s*=\s*(\d+)",
            r"REQUEST_UPDATE_SETTINGS\s*=\s*(\d+)",
        ),
    }

    failures: list[str] = []
    for label, (rust_pattern, cpp_pattern) in comparisons.items():
        rust_value = extract(rust_pattern, rust, f"Rust {label}")
        cpp_value = extract(cpp_pattern, cpp, f"C++ {label}")
        if rust_value != cpp_value:
            failures.append(f"{label}: Rust={rust_value}, C++={cpp_value}")

    rust_pipe = extract(r'PIPE_NAME:\s*&str\s*=\s*r"([^"]+)"', rust, "Rust pipe name")
    cpp_pipe = extract(r'PIPE_NAME\s*=\s*L"([^"]+)"', cpp, "C++ pipe name")
    cpp_pipe_value = cpp_pipe.replace("\\\\", "\\")
    if rust_pipe != cpp_pipe_value:
        failures.append(f"pipe name: Rust={rust_pipe}, C++={cpp_pipe}")

    if failures:
        raise SystemExit("IPC protocol mismatch:\n" + "\n".join(failures))
    print("IPC protocol constants are compatible.")


if __name__ == "__main__":
    main()
