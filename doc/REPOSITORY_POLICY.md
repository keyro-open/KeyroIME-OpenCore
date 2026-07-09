# KeyroIME OpenCore Repository Policy

- This repository is the public OpenCore source baseline.
- Use Japanese or English for tracked text, comments, commit messages, and handoff notes.
- Do not add Chinese text to public repository files.
- Do not add private repository names, private GitHub organization references, or local machine paths.
- Do not add closed-source commercial ranking logic, enterprise encrypted dictionary payloads, private SLM models, customer assets, credentials, or commercial installer secrets.
- The TSF DLL must not load Rust, full dictionaries, network components, or AI runtimes inside host processes.
- Keep IPC on `\\.\pipe\KeyroIME.Service.v1` unless a formal protocol migration is approved.
- `q`/`v` source promotion, prediction, candidate concatenation, and katakana loanword support are valid OpenCore capabilities.
- Do not commit `release/`, `dist/`, `target/`, `build/`, certificates, keys, logs, credentials, generated caches, or local path files.
- For dictionary files, prefer manifest, size, row count, or exact-match sampling. Full TSV reads are allowed only for explicit dictionary import, dictionary audit, or release build work.
