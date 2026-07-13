"""Validate the installer source whitelist and generated executable.

Copyright (C) 2025-2026 LocalPro Co., Ltd. All rights reserved.
Source-available under the KeyroIME OpenCore Non-Commercial Source License 1.0.
Commercial use requires a separate written license from LocalPro Co., Ltd.
"""

from __future__ import annotations

import argparse
import re
from pathlib import Path


ALLOWED_PAYLOAD = {
    "KeyroIME.dll",
    "keyro_service.exe",
    "keyro_tray.exe",
    "LICENSE_ja.txt",
    "LICENSE_en.txt",
    "THIRD_PARTY_NOTICES.md",
    "dictionary_manifest.json",
}
ALLOWED_UI_ASSETS = {
    "cover.bmp",
    "progress-1.bmp",
    "progress-2.bmp",
    "progress-3.bmp",
    "progress-4.bmp",
    "finish.bmp",
}
FORBIDDEN_TEXT = (
    "PROJECT_HANDOFF",
    "doc\\",
    ".bat",
    ".ps1",
    ".py",
    "smoke",
    "bench",
    "test.exe",
)


def files_section(script: str) -> list[str]:
    match = re.search(r"(?ms)^\[Files\]\s*(.*?)(?=^\[[^]]+\])", script)
    if not match:
        raise SystemExit("installer script does not contain a [Files] section")
    return [
        line.strip()
        for line in match.group(1).splitlines()
        if line.strip() and not line.lstrip().startswith(";")
    ]


def source_name(line: str) -> str:
    match = re.search(r'Source:\s*"([^"]+)"', line, re.IGNORECASE)
    if not match:
        raise SystemExit(f"[Files] entry has no literal Source: {line}")
    source = match.group(1)
    if "*" in source or "?" in source:
        raise SystemExit(f"wildcards are not allowed in installer sources: {source}")
    return source.replace("/", "\\").rsplit("\\", 1)[-1]


def validate_script(path: Path) -> None:
    script = path.read_text(encoding="utf-8")
    entries = files_section(script)
    payload: set[str] = set()
    ui_assets: set[str] = set()

    for line in entries:
        name = source_name(line)
        lowered = line.lower()
        if "{#releasedir}" in lowered:
            if "dontcopy" in lowered:
                raise SystemExit(f"product payload cannot use dontcopy: {line}")
            payload.add(name)
        elif "assets\\" in lowered:
            if "dontcopy" not in lowered:
                raise SystemExit(f"wizard image must use dontcopy: {line}")
            ui_assets.add(name)
        else:
            raise SystemExit(f"unrecognized installer source: {line}")

    if payload != ALLOWED_PAYLOAD:
        raise SystemExit(
            f"installer payload whitelist mismatch: expected={sorted(ALLOWED_PAYLOAD)}, "
            f"actual={sorted(payload)}"
        )
    if ui_assets != ALLOWED_UI_ASSETS:
        raise SystemExit(
            f"installer UI asset mismatch: expected={sorted(ALLOWED_UI_ASSETS)}, "
            f"actual={sorted(ui_assets)}"
        )

    required_contract = (
        "CarouselStageDurationMs = 3000;",
        "CarouselMinimumDurationMs = 12000;",
        "SetTimer(0, 0, 100, CreateCallback(@UpdateCarousel))",
        "PipeReadyAttempts = 100;",
        "PipeReadyDelayMs = 100;",
        "ProgressImagePaths: array[0..3] of String;",
        "obj= \"NT AUTHORITY\\LocalService\"",
        "binPath= \"\\\"' + ProductServicePath + '\\\"\"",
        "WaitNamedPipe(PipeName, PipeReadyDelayMs)",
    )
    for text in required_contract:
        if text not in script:
            raise SystemExit(f"installer contract is missing: {text}")

    for forbidden in FORBIDDEN_TEXT:
        for line in entries:
            if forbidden.lower() in line.lower():
                raise SystemExit(f"forbidden packaged content found: {line}")


def validate_installer(path: Path, version: str) -> None:
    expected_name = f"KeyroIME_Setup_v{version}.exe"
    if path.name != expected_name:
        raise SystemExit(
            f"unexpected installer name: expected={expected_name}, actual={path.name}"
        )
    if path.stat().st_size < 1_000_000:
        raise SystemExit(f"installer is unexpectedly small: {path.stat().st_size} bytes")
    with path.open("rb") as stream:
        if stream.read(2) != b"MZ":
            raise SystemExit("installer does not have a Windows PE header")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--script", type=Path, default=Path("installer/KeyroIME.iss")
    )
    parser.add_argument("--installer", type=Path)
    parser.add_argument("--version-file", type=Path, default=Path("VERSION"))
    args = parser.parse_args()

    validate_script(args.script)
    if args.installer:
        version = args.version_file.read_text(encoding="utf-8").strip()
        validate_installer(args.installer, version)
    print("Installer contract validation passed.")


if __name__ == "__main__":
    main()
