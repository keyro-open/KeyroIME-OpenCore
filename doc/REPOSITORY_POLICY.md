# KeyroIME OpenCore Repository Policy

- This repository is the public source-available OpenCore baseline.
- Use Japanese or English for tracked text, comments, commit messages, and handoff notes.
- Do not add Chinese text to public repository files.
- Do not add private repository names, private GitHub organization references, or local machine paths.
- Describe the license model as source-available, not open source. Personal non-commercial modification and sharing must follow `LICENSE`; company use and all other commercial use require a separate written license from LocalPro Co., Ltd.
- Keep third-party materials under their own licenses and preserve provenance in `THIRD_PARTY_NOTICES.md` and the dictionary manifest.
- Obsolete commits contain a superseded license notice. Do not make the existing history public. Publish a clean baseline from the approved final tree, or rewrite history only after explicit authorization and a verified backup.
- Do not add closed-source commercial ranking logic, enterprise encrypted dictionary payloads, private SLM models, customer assets, credentials, or commercial installer secrets.
- The TSF DLL must not load Rust, full dictionaries, network components, or AI runtimes inside host processes.
- Keep IPC on `\\.\pipe\KeyroIME.Service.v1` unless a formal protocol migration is approved.
- `q`/`v` source promotion, prediction, candidate concatenation, and katakana loanword support are valid OpenCore capabilities.
- Do not commit `release/`, `dist/`, `target/`, `build/`, certificates, keys, logs, credentials, generated caches, or local path files.
- For dictionary files, prefer manifest, size, row count, or exact-match sampling. Full TSV reads are allowed only for explicit dictionary import, dictionary audit, or release build work.
