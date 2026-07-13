#!/usr/bin/env python3
"""Build KeyroIME TSV assets from licensed JMdict/JMnedict snapshots.

Copyright (C) 2025-2026 LocalPro Co., Ltd. All rights reserved.
Source-available under the KeyroIME OpenCore Non-Commercial Source License 1.0.
Commercial use requires a separate written license from LocalPro Co., Ltd.
"""

from __future__ import annotations

import argparse
import datetime as dt
import gzip
import hashlib
import json
import re
import xml.etree.ElementTree as ET
from collections import defaultdict
from pathlib import Path


PRIORITY_SCORE = {
    "news1": 1000,
    "ichi1": 990,
    "spec1": 980,
    "gai1": 970,
    "news2": 900,
    "ichi2": 890,
    "spec2": 880,
    "gai2": 870,
}

DEFAULT_LIMITS = {
    "system": 60_000,
    "frequent": 15_000,
    "names": 40_000,
    "places": 25_000,
    "translations": 50_100,
}

CURATED_FREQUENT_ROWS = {
    ("にほん", "日本", 2_500),
    ("ありがとう", "ありがとう", 2_500),
    ("おねがい", "お願い", 2_500),
    ("だいじょうぶ", "大丈夫", 2_500),
}

CURATED_TRANSLATION_ROWS = {
    ("computer", "コンピューター", 1_000),
    ("computer", "コンピュータ", 980),
    ("computer", "電子計算機", 940),
    ("computer", "計算機", 920),
    ("software", "ソフトウェア", 1_000),
    ("software", "ソフト", 900),
    ("keyboard", "キーボード", 1_000),
    ("input method", "入力方式", 980),
    ("input method", "入力メソッド", 940),
}


def score_priority(tags: list[str]) -> int:
    score = max((PRIORITY_SCORE.get(tag, 0) for tag in tags), default=0)
    for tag in tags:
        match = re.fullmatch(r"nf(\d\d)", tag)
        if match:
            score = max(score, 950 - int(match.group(1)) * 10)
    return max(score, 500)


def clean(value: str | None) -> str:
    return (value or "").replace("\t", " ").replace("\r", " ").replace("\n", " ").strip()


def read_rows(path: Path, columns: int) -> set[tuple]:
    rows: set[tuple] = set()
    if not path.exists():
        return rows
    for line in path.read_text(encoding="utf-8").splitlines():
        if not line or line.startswith("#"):
            continue
        values = line.split("\t")
        if len(values) < columns:
            continue
        values[2] = int(values[2])
        rows.add(tuple(values[:columns]))
    return rows


def merge_best_scores(rows: set[tuple]) -> set[tuple]:
    best: dict[tuple, tuple] = {}
    for row in rows:
        identity = (row[0], row[1], *row[3:])
        previous = best.get(identity)
        if previous is None or int(row[2]) > int(previous[2]):
            best[identity] = row
    return set(best.values())


def select_rows(
    generated: set[tuple],
    baseline: set[tuple],
    limit: int,
    per_key: int,
    minimum_score: int = 0,
) -> set[tuple]:
    selected = merge_best_scores(baseline)
    selected_identities = {(row[0], row[1], *row[3:]) for row in selected}
    key_counts: defaultdict[str, int] = defaultdict(int)
    for row in selected:
        key_counts[str(row[0])] += 1

    ranked = sorted(
        merge_best_scores(generated),
        key=lambda row: (-int(row[2]), len(str(row[1])), str(row[0]), str(row[1]), row[3:]),
    )
    for row in ranked:
        if len(selected) >= limit:
            break
        identity = (row[0], row[1], *row[3:])
        key = str(row[0])
        if int(row[2]) < minimum_score or identity in selected_identities:
            continue
        if key_counts[key] >= per_key:
            continue
        selected.add(row)
        selected_identities.add(identity)
        key_counts[key] += 1
    return merge_best_scores(selected)


def write_rows(path: Path, header: str, rows: set[tuple]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="\n") as output:
        output.write(f"# {header}\n")
        for row in sorted(rows, key=lambda item: (item[0], -int(item[2]), item[1])):
            output.write("\t".join(str(value) for value in row) + "\n")


def build_jmdict(source: Path) -> tuple[set[tuple], set[tuple], set[tuple]]:
    system_rows: set[tuple[str, str, int]] = set()
    frequent_rows: set[tuple[str, str, int]] = set()
    translation_rows: set[tuple[str, str, int]] = set()

    with gzip.open(source, "rb") as stream:
        for _, entry in ET.iterparse(stream, events=("end",)):
            if entry.tag != "entry":
                continue
            writings = [clean(node.text) for node in entry.findall("./k_ele/keb")]
            readings = [clean(node.text) for node in entry.findall("./r_ele/reb")]
            priorities = [clean(node.text) for node in entry.findall(".//ke_pri")]
            priorities.extend(clean(node.text) for node in entry.findall(".//re_pri"))
            score = score_priority(priorities)
            surfaces = writings or readings
            glosses = [
                clean(node.text)
                for node in entry.findall("./sense/gloss")
                if node.attrib.get("{http://www.w3.org/XML/1998/namespace}lang", "eng") == "eng"
            ]

            for reading in readings:
                for surface in surfaces[:6]:
                    if reading and surface:
                        system_rows.add((reading, surface, score))
                        if score >= 900:
                            frequent_rows.add((reading, surface, score + 1_500))
                for gloss in glosses[:3]:
                    if score > 500 and reading and gloss and len(gloss) <= 48:
                        translation_rows.add((reading, gloss, score))

            for gloss in glosses[:3] if score > 500 else []:
                key = gloss.casefold()
                if re.fullmatch(r"[a-z][a-z '\-]{0,31}", key) and len(key.split()) <= 4:
                    for surface in surfaces[:3]:
                        translation_rows.add((key, surface, score))
            entry.clear()

    return system_rows, frequent_rows, translation_rows


def classify_name(types: list[str]) -> str:
    joined = " ".join(types).casefold()
    if "railway station" in joined or "station" in joined:
        return "station"
    if any(token in joined for token in ("place name", "geographic")):
        return "place"
    if any(token in joined for token in (
        "surname", "given name", "full name", "female", "male", "person", "unclassified name"
    )):
        return "name"
    return "exclude"


def name_score(types: list[str], kind: str) -> int:
    joined = " ".join(types).casefold()
    if kind == "station":
        return 980
    if kind == "place":
        return 940
    if "family or surname" in joined:
        return 920
    if "given name" in joined:
        return 900
    if "full name" in joined:
        return 880
    return 800


def build_jmnedict(source: Path) -> tuple[set[tuple], set[tuple]]:
    name_rows: set[tuple[str, str, int]] = set()
    place_rows: set[tuple[str, str, int, str]] = set()

    with gzip.open(source, "rb") as stream:
        for _, entry in ET.iterparse(stream, events=("end",)):
            if entry.tag != "entry":
                continue
            writings = [clean(node.text) for node in entry.findall("./k_ele/keb")]
            readings = [clean(node.text) for node in entry.findall("./r_ele/reb")]
            types = [clean(node.text) for node in entry.findall(".//name_type")]
            kind = classify_name(types)
            score = name_score(types, kind)
            for reading in readings:
                for surface in (writings or readings)[:6]:
                    if kind == "name":
                        name_rows.add((reading, surface, score))
                    elif kind in ("place", "station"):
                        place_rows.add((reading, surface, score, kind))
            entry.clear()

    return name_rows, place_rows


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--jmdict", type=Path, required=True)
    parser.add_argument("--jmnedict", type=Path, required=True)
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("src/keyro_service/assets"),
    )
    parser.add_argument("--max-system", type=int, default=DEFAULT_LIMITS["system"])
    parser.add_argument("--max-frequent", type=int, default=DEFAULT_LIMITS["frequent"])
    parser.add_argument("--max-names", type=int, default=DEFAULT_LIMITS["names"])
    parser.add_argument("--max-places", type=int, default=DEFAULT_LIMITS["places"])
    parser.add_argument("--max-translations", type=int, default=DEFAULT_LIMITS["translations"])
    args = parser.parse_args()

    baselines = {
        "system": read_rows(args.output / "system.tsv", 3),
        "frequent": read_rows(args.output / "frequent.tsv", 3),
        "names": read_rows(args.output / "names.tsv", 3),
        "places": read_rows(args.output / "places.tsv", 4),
        "translations": read_rows(args.output / "translations.tsv", 3),
    }
    baselines["frequent"].update(CURATED_FREQUENT_ROWS)
    baselines["translations"].update(CURATED_TRANSLATION_ROWS)
    system_generated, frequent_generated, translation_generated = build_jmdict(args.jmdict)
    name_generated, place_generated = build_jmnedict(args.jmnedict)
    existing_manifest_path = args.output / "dictionary_manifest.json"
    existing_manifest = {}
    if existing_manifest_path.exists():
        try:
            existing_manifest = json.loads(existing_manifest_path.read_text(encoding="utf-8"))
        except json.JSONDecodeError:
            existing_manifest = {}

    assets = {
        "system": select_rows(system_generated, baselines["system"], args.max_system, 8, 501),
        "frequent": select_rows(frequent_generated, baselines["frequent"], args.max_frequent, 4, 2_300),
        "names": select_rows(name_generated, baselines["names"], args.max_names, 4),
        "places": select_rows(place_generated, baselines["places"], args.max_places, 5),
        "translations": select_rows(
            translation_generated, baselines["translations"], args.max_translations, 6, 501
        ),
    }
    katakana_rows = read_rows(args.output / "katakana.tsv", 3)
    supplement_counts = {
        "system": len(read_rows(args.output / "system_supplement.tsv", 3)),
        "katakana": len(katakana_rows),
        "names": len(read_rows(args.output / "names_supplement.tsv", 3)),
        "places": len(read_rows(args.output / "places_supplement.tsv", 4)),
        "translations": len(read_rows(args.output / "translations_supplement.tsv", 3)),
    }

    write_rows(args.output / "system.tsv", "reading\tword\tscore", assets["system"])
    write_rows(args.output / "frequent.tsv", "reading\tword\tscore", assets["frequent"])
    write_rows(args.output / "names.tsv", "reading\tname\tscore", assets["names"])
    write_rows(
        args.output / "places.tsv", "reading\tplace_or_station\tscore\tkind", assets["places"]
    )
    write_rows(
        args.output / "translations.tsv",
        "query\ttranslation\tscore",
        assets["translations"],
    )

    manifest = {
        "generated_on": dt.date.today().isoformat(),
        "license": "CC BY-SA 4.0",
        "sources": {
            args.jmdict.name: sha256(args.jmdict),
            args.jmnedict.name: sha256(args.jmnedict),
        },
        "selection_targets": {
            "system": args.max_system,
            "frequent": args.max_frequent,
            "names": args.max_names,
            "places": args.max_places,
            "translations": args.max_translations,
            "katakana": len(katakana_rows),
        },
        "katakana_references": {
            "note": (
                "Project-authored selection verified against multiple public terminology "
                "references; no definitions or bulk third-party word lists are redistributed."
            ),
            "references": [
                "EDRDG JMdict project snapshot",
                "https://www2.ninjal.ac.jp/gairaigo/Teian1_4/index.html",
                "https://www.digital.go.jp/resources/standard_guidelines",
            ],
        },
        "curated_supplements": {
            "added_on": dt.date.today().isoformat(),
            "license": existing_manifest
                .get("curated_supplements", {})
                .get("license", "Project-authored KeyroIME product baseline"),
            "counts": supplement_counts,
        },
        "local_dictionary_imports": existing_manifest.get("local_dictionary_imports", []),
        "counts": {
            **{name: len(rows) for name, rows in assets.items()},
            "katakana": len(katakana_rows),
        },
        "effective_counts": {
            **{
                name: len(rows) + supplement_counts.get(name, 0)
                for name, rows in assets.items()
            },
            "katakana": len(katakana_rows),
        },
    }
    args.output.mkdir(parents=True, exist_ok=True)
    (args.output / "dictionary_manifest.json").write_text(
        json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8"
    )


if __name__ == "__main__":
    main()
