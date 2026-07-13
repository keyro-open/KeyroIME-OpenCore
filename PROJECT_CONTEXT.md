# KeyroIME OpenCore Engineering Context

This file keeps a compact engineering snapshot for maintainers. For current handoff and policy, start with `PROJECT_HANDOFF.md` and the `doc/` directory.

## Reading Order

1. `.clinerules` and `.gitignore`
2. `PROJECT_HANDOFF.md`
3. `doc/PROJECT_HANDOFF.md`
4. `doc/OPENCORE_ARCHITECTURE.md`
5. Task-relevant source files

Source code is authoritative when it differs from this document.

## Runtime Architecture

```text
keyro_tray.exe
        |
        | shared settings
        v
Windows text host -- TSF COM --> KeyroIME.dll
        |                         |
        | compact named pipe      | local romaji-to-kana fallback
        v                         |
keyro_service.exe <--------------+
        |
        v
static dictionaries, ranking, paging, user frequency
```

Core isolation rules:

- The TSF DLL does not load Rust, full dictionaries, configuration files, network code, or AI runtimes inside host processes.
- The Rust backend service runs as `LocalService`, rejects remote pipe clients, and stores its bounded WAL under `%ProgramData%\KeyroIME` with service-only write access.
- If the background service fails, `Activate` must still return `S_OK`, and the IME must keep local fallback input.
- TSF callbacks must not block on slow IPC while holding COM locks.
- Production binaries are Release x64.
- v1.0 has no cloud login, cloud request path, or AI inference runtime.
- The tray UI runs as a user-session process, not inside every injected text host.

## Directory Roles

- `src/tsf_shell`: C++ TSF COM DLL, async named-pipe client, GDI candidate window, tray UI, and smoke tests.
- `src/keyro_service`: Rust Windows service, named-pipe server, protocol, romaji conversion, dictionary lookup, and ranking.
- `src/keyro_service/assets`: public sample dictionary assets and manifest.
- `tools`: dictionary asset generation and profile preparation.
- `include`: compatibility headers for FFI experiments.
- `release`, `target`, and `src/tsf_shell/build`: generated output, not source.

## TSF/COM State

Production entry points live under `src/tsf_shell/tsf_core`:

- `DllGetClassObject`
- `DllCanUnloadNow`
- `DllRegisterServer`
- `DllUnregisterServer`
- `ITfTextInputProcessor`
- `ITfKeyEventSink`
- `ITfCompositionSink`

Candidate UI uses a GDI vertical window positioned from TSF context view text extents. Display labels are UI hints; committed text must remove display-only labels.

Fixed identifiers:

- Text service CLSID: `{8B4F9B54-7B15-4D8C-9E32-6D5A17B0A51E}`
- Profile GUID: `{6C33B9A3-5CE6-4D27-8A9F-0E9B60F0E7B9}`
- LangID: `0x0411`
- Main category: `GUID_TFCAT_TIP_KEYBOARD`
- COM `ThreadingModel`: `Apartment`

## IPC Protocol v1

Pipe name:

```text
\\.\pipe\KeyroIME.Service.v1
```

The protocol is compact little-endian binary data, not JSON.

Lookup request:

```text
u8  request_type = 1
u32 page
u32 input_utf8_byte_length
u8[] input_utf8
```

Selection record request:

```text
u8  request_type = 2
u32 page
u32 payload_utf8_byte_length
u8[] reading<TAB>surface
```

Response:

```text
u8  status
u8  candidate_count
u16 total_pages
u32 payload_byte_length
repeat candidate_count times:
    u16 candidate_utf8_byte_length
    u8[] candidate_utf8
```

Constraints:

- Each page returns up to five candidate slots.
- Maximum input is 4096 UTF-8 bytes.
- The client uses a short total deadline for connect, write, response header, and payload reads.
- Overlapped I/O must fail fast when the pipe is busy or unavailable.
- Async responses must be checked against the current input and page before use.
- The server disconnects clients that do not provide a complete bounded request within 250 ms. The client retries once when a stale persistent connection has been closed.
- Pipe access is limited to required local service, administrator, interactive-user, and AppContainer identities. It is not an authorization boundary between applications in the same interactive Windows environment.

## Input Behavior

- C++ converts romaji to unconfirmed kana immediately.
- Rust ranks normal candidates, `q` translation candidates, and `v` name/place/station candidates.
- Static assets live in `src/keyro_service/assets`.
- User selection frequency is kept in memory and appended by a background WAL thread.
- `q` mode promotes translation candidates.
- `v` mode promotes name, place, and station candidates.
- Enter commits local kana when the user has not navigated candidates; after navigation it commits the highlighted candidate.
- Space or right arrow commits the highlighted candidate.
- `CapsLock` controls English upper/lowercase behavior.
- Shared tray settings are exposed through a local shared settings object.

## FFI Boundary

`src/keyro_service/src/lib.rs` exports C ABI compatibility functions for experiments and smoke tests:

- `match_romaji`
- `get_candidates`
- `evaluate_frequency`
- `free_string`

These functions are not the production TSF communication path. Returned strings are allocated by Rust and must be released with `free_string`. FFI entry points must prevent panic from crossing the ABI boundary.

## Build and Validation

Full release build:

```bat
build_release.bat
```

The script builds and tests the Rust service, configures CMake for Visual Studio x64, builds the TSF DLL, tray executable, smoke tests, and release package.

The product version is read from the repository-root `VERSION` file. Rust uses the SemVer-compatible package version `1.0.6+15`, and CMake/UI/release scripts use `1.0.6.15`. The Rust toolchain and `Cargo.lock` are committed for reproducibility. Windows CI builds the products and runs non-interactive smoke tests.

Useful checks:

```powershell
cargo test --release --target x86_64-pc-windows-msvc `
  --manifest-path src/keyro_service/Cargo.toml `
  --target-dir target/keyro_service_runtime

src\tsf_shell\build\Release\keyro_tsf_activation_smoke.exe
src\tsf_shell\build\Release\keyro_local_fallback_smoke.exe
src\tsf_shell\build\Release\keyro_ipc_failover_smoke.exe
```

## Known Follow-Up

- Continue end-to-end testing in Notepad, Chromium/Electron, and AppContainer/UWP applications.
- `src/keyro_service/src/state_machine.rs` is a legacy prototype path and currently produces `dead_code` warnings.
- C++ and Rust currently maintain protocol constants separately. A shared protocol source should include cross-language encoding compatibility tests.
- Runtime behavior must stay local-first: no network, login, or AI dependency in v1.0.
- Code signing remains a release-operations requirement because signing certificates and keys must not be stored in this repository.
