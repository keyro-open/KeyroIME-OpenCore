# KeyroIME OpenCore Handoff

Snapshot date: 2026-07-12 (Asia/Tokyo)

## Public Repository

```text
https://github.com/keyro-open/KeyroIME-OpenCore.git
```

## Scope

KeyroIME OpenCore is the public source baseline for the KeyroIME Windows Japanese IME. It includes:

- `KeyroIME.dll`: C++17 TSF/COM text service.
- `keyro_service.exe`: Rust 2021 dictionary, prediction, ranking, user-frequency, and IPC service.
- `keyro_tray.exe`: C++17 user-session tray process, menu, OSD, About, and License UI.
- `\\.\pipe\KeyroIME.Service.v1`: compact local named-pipe protocol.
- Public sample dictionary assets, tooling, documentation, and release build scripts.

Do not add closed-source commercial ranking logic, enterprise encrypted dictionary payloads, private SLM models, customer assets, credentials, or commercial installer secrets.

## Current Baseline

- Product version: `KeyroIME OpenCore v1.0.6.15`.
- Default branch: `main`.
- Primary capabilities: TSF composition/commit, candidate window, ANSI/JIS mapping, Shift/CapsLock behavior, tray menu, OSD, `q`/`v` source promotion, katakana loanwords, long-vowel handling, static and user-learned prediction, candidate concatenation, KeyroIME help candidate page, and local 14-day pseudo update reminder.
- User-learned candidates use a dedicated predictive index and the highest ranking source priority. Name, place, and station source tags retain their internal meaning while the candidate window displays the unified `名` tag.
- The legal company name is `株式会社LocalPro` in Japanese and `LocalPro Co., Ltd.` in English.

## Documentation Entry Points

- `README.md`: public entry point.
- `README.en.md`: English product documentation.
- `README.ja.md`: Japanese product documentation.
- `doc/PROJECT_HANDOFF.md`: repository handoff.
- `doc/OPENCORE_ARCHITECTURE.md`: process and protocol architecture.
- `doc/REPOSITORY_POLICY.md`: public repository policy.

## Validation Baseline

- Rust release tests: 54 passed.
- Full release build: expected to build Rust service, C++ TSF DLL, tray process, and smoke-test utilities.
- Required smoke tests: TSF activation, local fallback, candidate tag normalization, runtime input, tray menu, and IPC failover.
- Last full release validation: passed on 2026-07-12 with full dictionary assets; IPC failover remained below the 10 ms target.

## Release Notes

- `release/`, `dist/`, `target/`, and `build/` must not enter Git.
- Dictionary TSV files can be large. Prefer manifest, size, row count, or exact-match checks unless the task explicitly requires full dictionary processing.
- After replacing the TSF DLL, restart Explorer/CTF or sign out and sign in again so host processes release old DLL mappings.
