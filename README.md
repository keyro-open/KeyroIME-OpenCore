# KeyroIME OpenCore v1.0.6.15

Type Japanese without breaking your flow.

KeyroIME OpenCore is a local-first, source-available Japanese input method editor for Windows x64. It keeps kana/kanji conversion, katakana loanwords, half-width numbers and symbols, English input, and JIS/ANSI keyboard switching close to the keys your fingers already know.

Japanese writing rarely stays in one character set. A single email or technical note may contain names, dates, prices, model numbers, URLs, commands, and English terminology. KeyroIME brings those transitions into one continuous workflow so you can spend less time correcting modes and more time finishing the sentence.

Repository:

```text
https://github.com/keyro-open/KeyroIME-OpenCore.git
```

README:

- [日本語](README.ja.md)
- [English](README.en.md)

Designed for everyday mixed input:

- Continuous Japanese input for Windows users who mix Japanese, numbers, symbols, and English throughout the day.
- Default half-width input for Japanese/English mixed writing.
- One-key number and symbol entry without candidate selection or waiting.
- ANSI/JIS physical keyboard switching for Japanese and US keyboard environments.
- `CapsLock` English upper/lowercase lock, `Shift+CapsLock` width switching, Shift punctuation switching, `v` names/places/stations, and `q` translation shortcuts.
- Katakana loanword matching, fuzzy prediction from one kana/kanji, and dynamic candidate concatenation for uncommon names and places.
- Local-first Windows architecture: C++17 TSF shell, Rust dictionary service, tray UI, and no account, cloud, or AI dependency in v1.0.

KeyroIME is intended for business documents, software development, technical writing, and international workplaces where Japanese and US keyboard environments coexist. Dictionary lookup and candidate generation stay on the Windows PC, while the TSF layer retains local romaji-to-kana fallback when the service is unavailable.

This repository intentionally excludes build outputs, release binaries, logs, local paths, credentials, generated caches, closed-source commercial logic, private models, customer assets, and commercial installer secrets.

KeyroIME OpenCore permits personal non-commercial use, modification, and sharing. Shared versions must provide complete source code under the same license, retain notices, identify LocalPro and the original project, and describe modifications. Company use and all other commercial use require a separate written license from LocalPro Co., Ltd.

Repository handoff: see [PROJECT_HANDOFF.md](PROJECT_HANDOFF.md) and [doc](doc).

OpenCore license: KeyroIME OpenCore Non-Commercial Source License 1.0. See [LICENSE](LICENSE). Release packages also include [LICENSE_ja.txt](LICENSE_ja.txt) and [LICENSE_en.txt](LICENSE_en.txt). Third-party dictionary notices: see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
