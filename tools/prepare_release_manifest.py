"""Prepare a public dictionary manifest without local preparation metadata.

Copyright (C) 2025-2026 LocalPro Co., Ltd. All rights reserved.
GNU GPLv3に基づいて配布されます。LICENSE（英語正文）を参照してください。

"""

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path


LOCAL_PATH_PATTERN = re.compile(r"(?:^[A-Za-z]:[\\/]|/Users/|/home/)")


def prepare_manifest(source: Path) -> dict:
    manifest = json.loads(source.read_text(encoding="utf-8"))
    manifest["license"] = (
        "GNU General Public License version 3 for KeyroIME software and authored "
        "supplements; CC BY-SA 4.0 for JMdict/JMnedict-derived data"
    )

    supplements = manifest.setdefault("curated_supplements", {})
    supplements["origin"] = "Original data authored and owned by LocalPro Co., Ltd."
    supplements["license"] = "GNU General Public License version 3"

    for item in manifest.get("local_dictionary_imports", []):
        item["source"] = "LocalPro-authored original source file"
        item["provenance"] = (
            "Authored and owned by LocalPro Co., Ltd.; no external dictionary content."
        )

    return manifest


def validate_public_values(value: object, location: str = "manifest") -> None:
    if isinstance(value, dict):
        for key, child in value.items():
            validate_public_values(child, f"{location}.{key}")
    elif isinstance(value, list):
        for index, child in enumerate(value):
            validate_public_values(child, f"{location}[{index}]")
    elif isinstance(value, str) and LOCAL_PATH_PATTERN.search(value):
        raise SystemExit(f"local path is not allowed in public manifest: {location}")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    manifest = prepare_manifest(args.input)
    validate_public_values(manifest)
    args.output.write_text(
        json.dumps(manifest, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )


if __name__ == "__main__":
    main()
