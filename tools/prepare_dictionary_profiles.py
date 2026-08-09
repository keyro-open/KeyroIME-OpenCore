#!/usr/bin/env python3
"""Import local curated rows and prepare full/sample dictionary profiles.

Copyright (C) 2025-2026 LocalPro Co., Ltd. All rights reserved.
GNU GPLv3に基づいて配布されます。LICENSE（英語正文）を参照してください。

"""

from __future__ import annotations

import argparse
import datetime as dt
import hashlib
import json
import shutil
from collections import defaultdict
from pathlib import Path


ASSET_COLUMNS = {
    "system.tsv": 3,
    "frequent.tsv": 3,
    "katakana.tsv": 3,
    "names.tsv": 3,
    "names_supplement.tsv": 3,
    "places.tsv": 4,
    "places_supplement.tsv": 4,
    "translations.tsv": 3,
    "translations_supplement.tsv": 3,
    "system_supplement.tsv": 3,
}

HEADERS = {
    "system.tsv": "reading\tword\tscore",
    "frequent.tsv": "reading\tword\tscore",
    "katakana.tsv": "reading\tword\tscore",
    "names.tsv": "reading\tname\tscore",
    "names_supplement.tsv": "reading\tname\tscore",
    "places.tsv": "reading\tplace_or_station\tscore\tkind",
    "places_supplement.tsv": "reading\tplace_or_station\tscore\tkind",
    "translations.tsv": "query\ttranslation\tscore",
    "translations_supplement.tsv": "query\ttranslation\tscore",
    "system_supplement.tsv": "reading\tword\tscore",
}

SAMPLE_LIMITS = {
    "system.tsv": 360,
    "frequent.tsv": 220,
    "katakana.tsv": 220,
    "names.tsv": 220,
    "places.tsv": 240,
    "translations.tsv": 360,
}

REQUIRED_KEYS = {
    "system.tsv": {
        "こうかん",
        "たべる",
        "つ",
        "つか",
        "しょうち",
        "しよう",
        "しょ",
        "しようしょ",
        "りんぎしょ",
        "きーろ",
    },
    "frequent.tsv": {
        "こうかん",
        "たべる",
        "つ",
        "つか",
        "しょうち",
        "しよう",
        "しょ",
        "しようしょ",
    },
    "katakana.tsv": {
        "いんすとーる",
        "ぜろとらすと",
        "あぷりけーしょん",
        "あくせしびりてぃー",
        "わんおぺ",
        "ばっくろぐ",
        "つーる",
        "すけじゅーる",
    },
    "names.tsv": {"おおつき", "はると", "れん"},
    "names_supplement.tsv": {"おおつき", "はると", "れん"},
    "places.tsv": {"おおつき", "しぶや", "しんじゅく"},
    "places_supplement.tsv": {"おおつき", "しぶや", "しんじゅく"},
    "translations.tsv": {
        "check",
        "nihon",
        "にほん",
        "日本",
        "translation",
        "すけじゅーる",
        "schedule",
        "digital transformation",
        "digitaltransformation",
    },
    "translations_supplement.tsv": {
        "check",
        "nihon",
        "にほん",
        "日本",
        "translation",
        "すけじゅーる",
        "schedule",
        "digital transformation",
        "digitaltransformation",
    },
    "system_supplement.tsv": {
        "きーろ",
        "りんぎしょ",
        "わんおぺ",
        "ばっくろぐ",
        "まる",
        "ばつ",
        "こめじるし",
        "みぎやじるし",
    },
}

CURATED_ROWS = {
    "frequent.tsv": {
        ("こうかん", "交換", 2500),
        ("たべる", "食べる", 2500),
        ("つ", "使う", 2450),
        ("つ", "遣う", 2400),
        ("つ", "使い", 2350),
        ("つ", "疲れ", 2300),
        ("つか", "使う", 2450),
        ("つか", "遣う", 2400),
        ("つか", "使い", 2350),
        ("しょうち", "承知", 2500),
        ("しょうちしました", "承知しました", 2300),
        ("しょうちいたしました", "承知いたしました", 2250),
        ("しようしょ", "仕様書", 2500),
        ("しよう", "使用", 2400),
        ("しょ", "書", 2300),
    },
    "system_supplement.tsv": {
        ("きーろ", "KeyroIME", 3600),
        ("きーろ", "キーロ", 3500),
        ("きーろ", "キーロ入力", 3400),
        ("きーろ", "keyro.jp", 3300),
        ("りんぎしょ", "稟議書", 3300),
    },
    "katakana.tsv": {
        ("いんすとーる", "インストール", 3600),
        ("ぜろとらすと", "ゼロトラスト", 3500),
        ("あぷりけーしょん", "アプリケーション", 3400),
        ("あくせしびりてぃー", "アクセシビリティー", 3300),
        ("わんおぺ", "ワンオペ", 3300),
        ("ばっくろぐ", "バックログ", 3300),
        ("つーる", "ツール", 3200),
        ("すけじゅーる", "スケジュール", 3200),
    },
    "names_supplement.tsv": {
        ("おおつき", "大突", 3300),
        ("おおつき", "大月", 3200),
        ("はると", "陽翔", 3300),
        ("れん", "蓮", 3300),
    },
    "places_supplement.tsv": {
        ("おおつき", "大月駅", 3600, "station"),
        ("おおつき", "大月", 3500, "place"),
        ("おおつき", "大月市", 3400, "place"),
        ("おおつき", "大月町", 3300, "place"),
        ("しんじゅく", "新宿", 3600, "place"),
        ("しんじゅく", "新宿駅", 3500, "station"),
        ("しぶや", "渋谷", 3600, "place"),
        ("しぶや", "渋谷駅", 3500, "station"),
    },
    "translations_supplement.tsv": {
        ("check", "チェック", 3600),
        ("check", "確認する", 3500),
        ("check", "点検する", 3400),
        ("check", "検査する", 3300),
        ("check", "照合する", 3200),
        ("check", "確認", 3100),
        ("nihon", "Japan", 3600),
        ("にほん", "Japan", 3600),
        ("日本", "Japan", 3600),
        ("translation", "訳", 3600),
        ("すけじゅーる", "schedule", 3600),
        ("schedule", "スケジュール", 3600),
        ("digital transformation", "デジタルトランスフォーメーション", 3600),
        ("digitaltransformation", "デジタルトランスフォーメーション", 3500),
        ("computer", "コンピューター", 3400),
        ("software", "ソフトウェア", 3400),
    },
}


def clean(value: str) -> str:
    return value.replace("\t", " ").replace("\r", " ").replace("\n", " ").strip()


def score_from_priority(priority: str) -> int:
    try:
        value = int(priority)
    except ValueError:
        value = 5
    value = min(max(value, 1), 10)
    return 3400 - ((value - 1) * 140)


def row_key(row: tuple) -> tuple:
    return row[0], row[1]


def read_asset(path: Path, columns: int) -> list[tuple]:
    if not path.exists():
        return []
    rows = []
    for line in path.read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        values = tuple(clean(value) for value in line.split("\t")[:columns])
        if len(values) < columns:
            continue
        converted = list(values)
        converted[2] = int(converted[2]) if str(converted[2]).isdigit() else 100
        rows.append(tuple(converted))
    return rows


def merge_rows(rows: list[tuple] | set[tuple]) -> list[tuple]:
    best = {}
    for row in rows:
        key = (row[0], row[1], *row[3:])
        old = best.get(key)
        if old is None or int(row[2]) > int(old[2]):
            best[key] = row
    return sorted(best.values(), key=lambda item: (str(item[0]), -int(item[2]), str(item[1]), item[3:]))


def write_asset(path: Path, rows: list[tuple]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    header = HEADERS[path.name]
    with path.open("w", encoding="utf-8", newline="\n") as output:
        output.write(f"# {header}\n")
        for row in merge_rows(rows):
            output.write("\t".join(str(value) for value in row) + "\n")


def load_assets(asset_dir: Path) -> dict[str, list[tuple]]:
    return {
        name: read_asset(asset_dir / name, columns)
        for name, columns in ASSET_COLUMNS.items()
    }


def add_curated_rows(rows_by_file: dict[str, list[tuple]]) -> int:
    added = 0
    existing = {
        row_key(row)
        for rows in rows_by_file.values()
        for row in rows
    }
    for file_name, rows in CURATED_ROWS.items():
        for row in rows:
            if row_key(row) in existing:
                continue
            rows_by_file[file_name].append(row)
            existing.add(row_key(row))
            added += 1
    return added


def contains_katakana(value: str) -> bool:
    return any("\u30a0" <= character <= "\u30ff" for character in value)


def source_sha256(source: Path) -> str:
    digest = hashlib.sha256()
    with source.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def import_local_source(rows_by_file: dict[str, list[tuple]], source: Path) -> dict[str, int | str]:
    existing = {
        row_key(row)
        for rows in rows_by_file.values()
        for row in rows
    }
    imported = defaultdict(int)
    skipped = 0
    source_rows = 0
    for line in source.read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        columns = [clean(value) for value in line.split("\t")]
        if columns[:4] in (
            ["category", "surface", "reading", "priority"],
            ["source", "surface", "reading", "priority"],
            ["分類", "表記", "読み", "優先度"],
        ):
            continue
        if len(columns) == 3:
            reading, surface, score_text = columns
            if not reading or not surface or not score_text.isdigit():
                skipped += 1
                continue
            target = "katakana.tsv" if contains_katakana(surface) else "system_supplement.tsv"
            score = int(score_text)
        elif len(columns) >= 4 and columns[0] == columns[2]:
            surface, reading, _, score_text = columns[:4]
            if not surface or not reading or not score_text.isdigit():
                skipped += 1
                continue
            target = "katakana.tsv" if contains_katakana(surface) else "system_supplement.tsv"
            score = int(score_text)
        elif len(columns) >= 4:
            category, surface, reading, priority = columns[:4]
            if not surface or not reading:
                skipped += 1
                continue
            normalized_category = category.lower()
            target = (
                "katakana.tsv"
                if "katakana" in normalized_category or "カタカナ" in category
                else "system_supplement.tsv"
            )
            score = score_from_priority(priority)
        else:
            skipped += 1
            continue
        source_rows += 1
        row = (reading, surface, score)
        if row_key(row) in existing:
            skipped += 1
            continue
        rows_by_file[target].append(row)
        existing.add(row_key(row))
        imported[target] += 1
    imported["source_sha256"] = source_sha256(source)
    imported["source_rows"] = source_rows
    imported["skipped"] = skipped
    return dict(imported)


def select_sample_rows(file_name: str, rows: list[tuple], imported_pairs: set[tuple]) -> list[tuple]:
    required_keys = REQUIRED_KEYS.get(file_name, set())
    selected = []
    seen = set()
    for row in rows:
        if row_key(row) in imported_pairs or row[0] in required_keys or row[1] in required_keys:
            selected.append(row)
            seen.add((row[0], row[1], *row[3:]))
    if file_name in SAMPLE_LIMITS:
        ranked = sorted(rows, key=lambda row: (-int(row[2]), str(row[0]), str(row[1]), row[3:]))
        for row in ranked:
            identity = (row[0], row[1], *row[3:])
            if identity in seen:
                continue
            selected.append(row)
            seen.add(identity)
            if len(selected) >= SAMPLE_LIMITS[file_name]:
                break
    return merge_rows(selected)


def write_manifest(
    path: Path,
    rows_by_file: dict[str, list[tuple]],
    profile: str,
    imported: dict[str, int],
    source_origin: str,
    previous_imports: list[dict] | None = None,
) -> None:
    counts = {
        "system": len(rows_by_file["system.tsv"]),
        "frequent": len(rows_by_file["frequent.tsv"]),
        "names": len(rows_by_file["names.tsv"]),
        "places": len(rows_by_file["places.tsv"]),
        "translations": len(rows_by_file["translations.tsv"]),
        "katakana": len(rows_by_file["katakana.tsv"]),
    }
    supplement_counts = {
        "system": len(rows_by_file["system_supplement.tsv"]),
        "names": len(rows_by_file["names_supplement.tsv"]),
        "places": len(rows_by_file["places_supplement.tsv"]),
        "translations": len(rows_by_file["translations_supplement.tsv"]),
    }
    effective_counts = {
        "system": counts["system"] + supplement_counts["system"],
        "frequent": counts["frequent"],
        "names": counts["names"] + supplement_counts["names"],
        "places": counts["places"] + supplement_counts["places"],
        "translations": counts["translations"] + supplement_counts["translations"],
        "katakana": counts["katakana"],
    }
    local_imports = list(previous_imports or [])
    if imported.get("source_rows", 0):
        current_import = {
            "imported_on": dt.date.today().isoformat(),
            "source": source_origin,
            "source_sha256": imported.get("source_sha256", ""),
            "provenance": "Authored and owned by LocalPro Co., Ltd.; no external dictionary content.",
            "note": "Three-column rows are routed by surface script; legacy four-column rows retain their source category when available. Imported pairs are deduplicated across all asset tables.",
            "source_rows": imported.get("source_rows", 0),
            "duplicates_or_invalid_skipped": imported.get("skipped", 0),
            "imported_rows": {
                key: value for key, value in imported.items()
                if key.endswith(".tsv")
            },
        }
        local_imports = [
            item for item in local_imports
            if not (
                item.get("source") == source_origin
                and item.get("source_sha256") == current_import["source_sha256"]
            )
        ]
        local_imports.append(current_import)

    manifest = {
        "generated_on": dt.date.today().isoformat(),
        "asset_profile": profile,
        "license": "CC BY-SA 4.0 plus KeyroIME project-authored supplements",
        "sources": {
            "JMdict_e.gz": "65a52f037d95c9bd0a1954d74825c06d5392626b71630ead431937d6e2c26adf",
            "JMnedict.xml.gz": "56a8138c37a2ee521b1894ca6a64fcbfca62c0752b5b437c5bb0dfdf96d96fea",
        },
        "counts": counts,
        "curated_supplements": {
            "added_on": dt.date.today().isoformat(),
            "origin": "Original data authored and owned by LocalPro Co., Ltd.",
            "license": "GNU General Public License version 3",
            "counts": supplement_counts,
        },
        "effective_counts": effective_counts,
        "local_dictionary_imports": local_imports,
        "profile_note": (
            "development_sample keeps only representative rows for tests and development"
            if profile == "development_sample"
            else "full local dictionary profile; ignored by Git"
        ),
    }
    path.write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", type=Path)
    parser.add_argument(
        "--source-origin",
        default="LocalPro-authored original source file",
        help="Public provenance label for the LocalPro-authored import source",
    )
    parser.add_argument("--assets", type=Path, default=Path("src/keyro_service/assets"))
    parser.add_argument("--dump", type=Path, default=Path("dictionary_dumps/full_assets_current"))
    args = parser.parse_args()

    baseline_dir = args.dump if args.dump.exists() else args.assets
    rows_by_file = load_assets(baseline_dir)
    previous_manifest_path = baseline_dir / "dictionary_manifest.json"
    previous_manifest = {}
    if previous_manifest_path.exists():
        try:
            previous_manifest = json.loads(previous_manifest_path.read_text(encoding="utf-8"))
        except json.JSONDecodeError:
            previous_manifest = {}
    add_curated_rows(rows_by_file)
    imported: dict[str, int | str] = {}
    if args.source is not None:
        if not args.source.exists():
            raise SystemExit(f"source file not found: {args.source}")
        imported = import_local_source(rows_by_file, args.source)

    args.dump.mkdir(parents=True, exist_ok=True)
    for file_name, rows in rows_by_file.items():
        write_asset(args.dump / file_name, rows)
    write_manifest(
        args.dump / "dictionary_manifest.json",
        rows_by_file,
        "full_local",
        imported,
        args.source_origin,
        previous_manifest.get("local_dictionary_imports", []),
    )

    imported_pairs = {
        row_key(row)
        for file_name in ("system_supplement.tsv", "katakana.tsv")
        for row in rows_by_file[file_name]
    }
    sample_rows = {
        file_name: select_sample_rows(file_name, rows, imported_pairs)
        for file_name, rows in rows_by_file.items()
    }
    for file_name, rows in sample_rows.items():
        write_asset(args.assets / file_name, rows)
    write_manifest(
        args.assets / "dictionary_manifest.json",
        sample_rows,
        "development_sample",
        imported,
        args.source_origin,
        json.loads((args.assets / "dictionary_manifest.json").read_text(encoding="utf-8")).get(
            "local_dictionary_imports", []
        ) if (args.assets / "dictionary_manifest.json").exists() else [],
    )

    readme = args.dump / "README.txt"
    readme.write_text(
        "This directory contains the full local KeyroIME dictionary profile.\n"
        "It is intentionally ignored by Git. Set KEYROIME_DICTIONARY_ASSET_DIR\n"
        "to this directory when building a full-dictionary release.\n",
        encoding="utf-8",
    )
    current = args.dump.parent / "README.txt"
    if not current.exists():
        shutil.copyfile(readme, current)


if __name__ == "__main__":
    main()
