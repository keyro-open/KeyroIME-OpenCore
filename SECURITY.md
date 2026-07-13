# Security Policy

## Supported Version

Security fixes are applied to the current `main` branch and the latest published release.

## Reporting a Vulnerability

Do not publish a suspected vulnerability in a public issue. Use GitHub private vulnerability reporting when it is available, or contact LocalPro through the support information at:

```text
https://keyro.jp/keyroime_opencore/support
```

Include the affected version, Windows version, reproduction steps, expected impact, and any proof-of-concept material that is safe to share. LocalPro will acknowledge a complete report, investigate it, and coordinate disclosure after a fix is available.

## Security Boundary

- `KeyroIME.dll` is loaded into Windows text hosts and must remain lightweight. It does not load Rust, full dictionaries, network code, or AI runtimes.
- `keyro_service.exe` runs as `LocalService`, rejects remote named-pipe clients, and accepts only bounded local protocol messages.
- Interactive desktop and AppContainer text hosts require local pipe access. The pipe is not an authentication or authorization boundary between applications running in the same interactive Windows environment.
- User dictionary records are treated as untrusted input, sanitized, size-bounded, and stored under `%ProgramData%\KeyroIME` with service-only write permission.
- No v1.0 component performs cloud login, cloud synchronization, or automatic network update checks.

Changes to COM registration, named-pipe ACLs, installer privileges, shared memory, FFI ownership, or input-host isolation require security review and targeted smoke tests.
