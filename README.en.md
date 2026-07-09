# KeyroIME OpenCore v1.0

KeyroIME OpenCore is the public source baseline for the KeyroIME Windows Japanese IME.

Public repository:

```text
https://github.com/keyro-open/KeyroIME-OpenCore.git
```

Do not add closed-source ranking logic, encrypted enterprise dictionary payloads, private SLM models, customer assets, credentials, or commercial installer secrets to this repository.

KeyroIME is a local-first Japanese input method editor for Windows x64. It is designed for people who write Japanese while constantly touching numbers, symbols, English words, half-width text, and both Japanese and US keyboards.

The product idea is simple: stay in the sentence. KeyroIME reduces the tiny interruptions that usually happen between "I know what I want to type" and "it appears on screen."

## The Golden Three Seconds

Open a text field, start typing, and keep going.

KeyroIME is built so the first impression is not a settings screen or a learning curve. It is the feeling that numbers, punctuation, English, kana/kanji conversion, katakana loanwords, and keyboard layout changes are already where your fingers expect them to be.

## Product Promise

- Type Japanese faster by reducing mode switching, extra conversion steps, and repeated full-width/half-width corrections.
- Enter half-width numbers, symbols, and English smoothly during Japanese writing.
- Switch ANSI/JIS physical keyboard behavior for Japanese, US, and mixed workplace environments.
- Find katakana loanwords, names, places, and long phrases through mixed dictionary ranking, fuzzy prediction, and dynamic candidate concatenation.
- Keep the v1.0 experience local-first: no login, no cloud dependency, and no AI runtime loaded into Windows text host processes.

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

## Data Safety and Source Control

The repository excludes build outputs, release binaries, logs, local deployment paths, credentials, `.env` files, certificates, and generated caches. Before pushing, review:

```powershell
git status --short
git status --ignored --short
```

## License and Third-Party Notices

KeyroIME OpenCore source code is licensed under GPL v3. See `LICENSE`.

Dictionary assets include project-authored supplements and third-party-derived resources. See `THIRD_PARTY_NOTICES.md` and `src/keyro_service/assets/dictionary_manifest.json`.
