# KeyroIME OpenCore v1.0.6.15

KeyroIME OpenCore is the source-available, non-commercial community baseline for the KeyroIME Windows Japanese IME.

Repository:

```text
https://github.com/keyro-open/KeyroIME-OpenCore.git
```

Do not add closed-source ranking logic, encrypted enterprise dictionary payloads, private SLM models, customer assets, credentials, or commercial installer secrets to this repository.

KeyroIME is a local-first Japanese input method editor for Windows x64. It is designed for people who write Japanese while constantly touching numbers, symbols, English words, half-width text, and both Japanese and US keyboards.

The product idea is simple: stay in the sentence. KeyroIME reduces the small interruptions that usually happen between "I know what I want to type" and "it appears on screen."

## Keep the Sentence Moving

Japanese writing naturally includes dates, prices, model numbers, email addresses, URLs, English terms, and katakana loanwords. KeyroIME treats them as parts of one writing task rather than separate input modes to manage. Move from kana/kanji conversion to half-width numbers, punctuation, or English, select the candidate you need, and continue with the next phrase.

Getting started does not require a complex setup. Familiar keys control the input mode, character width, punctuation style, and JIS/ANSI layout. KeyroIME works with existing Windows applications, and if the dictionary service is temporarily unavailable, local romaji-to-kana conversion keeps basic input available.

KeyroIME is designed for practical workflows such as:

- Writing email and business documents that mix Japanese with dates, prices, product names, and addresses.
- Combining English identifiers, commands, and symbols with Japanese explanations in development and technical writing.
- Moving between Japanese and US keyboards across office, home, and international work environments.
- Finding katakana loanwords, personal names, place names, station names, and longer phrases through mixed dictionaries and predictive candidates.
- Keeping dictionary lookup and candidate generation on the Windows PC without requiring an account or cloud connection.

The `q` prefix prioritizes translation candidates, while `v` prioritizes names, places, and stations. Fuzzy prediction and dynamic candidate concatenation help surface useful forms from partial readings, including names that are not stored as a single dictionary entry.

## Architecture

- `KeyroIME.dll`: C++17 TSF/COM text service loaded by Windows text hosts.
- `keyro_service.exe`: Rust Windows service for dictionary lookup, prediction, ranking, user frequency, and IPC handling.
- `keyro_tray.exe`: C++17 user-session tray process for the notification icon, right-click menu, and status OSD.
- IPC: compact local named-pipe protocol at `\\.\pipe\KeyroIME.Service.v1`.

The TSF DLL must not load Rust, dictionaries, user configuration, network code, or AI runtimes inside host processes. When the service is unavailable, the TSF layer falls back to local romaji-to-kana conversion.

## Main Features

- Hiragana, katakana, and English input modes.
- ANSI/JIS physical keyboard layout switching. `Alt+;` is available, and the tray menu remains the fallback when a host app owns the shortcut.
- Default half-width mode for Japanese/English mixed input.
- One-key number and symbol entry without candidate selection or waiting.
- `Shift+CapsLock` switches half-width/full-width; full-width mode also applies to English letters.
- `CapsLock` switches the internal English upper/lowercase lock.
- Japanese/Western punctuation switching with a single Shift tap.
- Five-slot vertical candidate window with labels for translation, place/name sources, and paging.
- Mixed system, frequent, katakana, translation, name, and place dictionaries.
- Prefix boosting with `q` for translation and `v` for names/places/stations.
- Fuzzy prediction from one kana or kanji, with prefix matches ranked before middle matches.
- Dynamic candidate concatenation for uncommon words, names, and places.
- Built-in KeyroIME help candidate for `キーロ` and `ki-ro`.

## Repository Layout

```text
src/keyro_service/         Rust dictionary and ranking service
src/tsf_shell/             C++ TSF shell, tray UI, candidate window, smoke tests
src/tsf_shell/resources/   Windows icon resources
tools/                     Dictionary asset tooling
include/                   FFI compatibility headers
doc/                       Public handoff, architecture, and repository policy
```

## Build and Test

Requirements:

- Windows x64
- Visual Studio 2022 with C++ desktop workload
- Windows SDK
- Rust stable with `x86_64-pc-windows-msvc`
- CMake 3.20+

Typical command:

```bat
build_release.bat
```

Useful validation tools are generated under `src\tsf_shell\build\Release\`, including TSF activation, local fallback, tray menu, runtime input, and IPC failover smoke tests.

## Install

Installation requires Administrator privileges:

```bat
release\install.bat /silent
```

After upgrading the TSF DLL, restart Explorer/CTF or sign out and sign in again so text hosts stop using old loaded DLLs.

To uninstall while preserving learned dictionary data:

```bat
release\uninstall.bat /silent
```

Add `/purge` to remove the preserved user dictionary data.

## Data Safety and Source Control

The repository excludes build outputs, release binaries, logs, local deployment paths, credentials, `.env` files, certificates, and generated caches. Before pushing, review:

```powershell
git status --short
git status --ignored --short
```

## License and Third-Party Notices

KeyroIME OpenCore permits personal non-commercial use, modification, and sharing under the KeyroIME OpenCore Non-Commercial Source License 1.0. Shared versions must provide complete source code under the same license, retain attribution and notices, and identify modifications. Company use and all other commercial use require a separate written license from LocalPro Co., Ltd. See `LICENSE`.

Dictionary assets include project-authored supplements and third-party-derived resources. See `THIRD_PARTY_NOTICES.md` and `src/keyro_service/assets/dictionary_manifest.json`.
