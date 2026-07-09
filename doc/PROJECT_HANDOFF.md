# KeyroIME OpenCore Project Handoff

Snapshot date: 2026-07-09 (Asia/Tokyo)

## Repository

- Public URL: `https://github.com/keyro-open/KeyroIME-OpenCore.git`
- Visibility target: Public OpenCore repository.
- Role: standalone OpenCore source baseline for KeyroIME.
- Repository language: Japanese or English only.

## Modules

- `src/tsf_shell`: C++17 TSF/COM DLL, candidate window, tray UI, OSD, and smoke tests.
- `src/keyro_service`: Rust dictionary, prediction, ranking, user-frequency, and IPC service.
- `src/keyro_service/assets`: public sample dictionary assets and manifest.
- `tools`: dictionary asset tooling.
- `doc`: public handoff, architecture, and repository policy.

## Public Boundary

OpenCore keeps the public TSF shell, tray process, Rust local service, public dictionary assets, and open documentation.

Do not add:

- closed-source commercial ranking logic
- enterprise encrypted dictionary payloads
- private SLM models or runtime initialization
- customer assets or credentials
- commercial installer secrets
- local machine paths
- private GitHub organization or repository references

## Validation Baseline

- Rust release tests: 52 passed.
- `git diff --check`: required before commit.
- Full release validation should run `build_release.bat` when preparing a release or changing TSF/IPC behavior.

## Next Work

1. Keep public README, license, third-party notices, and dictionary manifest aligned.
2. Add CI for Rust tests and Windows TSF smoke-test builds.
3. Continue host validation in Notepad, Chromium/Electron, and AppContainer/UWP.
4. Keep generated release artifacts outside Git.
