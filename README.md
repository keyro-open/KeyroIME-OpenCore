# KeyroIME OpenCore v1.0

Type Japanese without breaking your flow.

KeyroIME OpenCore is a local-first Japanese input method editor for Windows x64. It keeps kana/kanji conversion, katakana loanwords, half-width numbers and symbols, English fallback, and JIS/ANSI keyboard switching close to the keys your fingers already know.

Official public repository:

```text
https://github.com/keyro-open/KeyroIME-OpenCore.git
```

README:

- [日本語](README.ja.md)
- [English](README.en.md)

Core product highlights:

- Fast Japanese input for Windows users who mix Japanese, numbers, symbols, and English throughout the day.
- Default half-width input for Japanese/English mixed writing.
- One-key number and symbol entry without candidate selection or waiting.
- ANSI/JIS physical keyboard switching for Japanese and US keyboard environments.
- `CapsLock` English upper/lowercase lock, `Shift+CapsLock` width switching, Shift punctuation switching, `v` names/places/stations, and `q` translation shortcuts.
- Katakana loanword matching, fuzzy prediction from one kana/kanji, and dynamic candidate concatenation for uncommon names and places.
- Local-first Windows architecture: C++17 TSF shell, Rust dictionary service, tray UI, and no cloud or AI dependency in v1.0.

This repository intentionally excludes build outputs, release binaries, logs, local paths, credentials, generated caches, closed-source commercial logic, private models, customer assets, and commercial installer secrets.

Repository handoff: see [PROJECT_HANDOFF.md](PROJECT_HANDOFF.md) and [doc](doc).

OpenCore license: GPL v3. See [LICENSE](LICENSE). Release packages also include OpenCore notices in [LICENSE_ja.txt](LICENSE_ja.txt) and [LICENSE_en.txt](LICENSE_en.txt). Third-party dictionary notices: see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
