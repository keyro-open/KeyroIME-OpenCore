# KeyroIME OpenCore Architecture

KeyroIME OpenCore uses a process-isolated Windows IME architecture:

- `KeyroIME.dll`: C++17 TSF/COM DLL for host input, composition text, candidate UI, local fallback, and JIS/ANSI key mapping.
- `keyro_service.exe`: Rust Windows service running as `LocalService` for dictionary lookup, candidate recall, ranking, paging, and user frequency.
- `keyro_tray.exe`: C++17 user-session process for tray menu, status OSD, About, and License UI.
- IPC: `\\.\pipe\KeyroIME.Service.v1`, with five candidate slots per page.

The TSF DLL must remain lightweight inside host processes. It must not load Rust, full dictionaries, user configuration files, network code, or AI runtimes.

When the Rust service is unavailable, the TSF shell must continue local romaji-to-kana conversion and English fallback input.

## Data Flow

```text
Windows text host
        |
        v
KeyroIME.dll  -- compact named pipe -->  keyro_service.exe
        |
        +-- local romaji-to-kana fallback

keyro_tray.exe  -- shared settings -->  KeyroIME.dll
```

## Protocol Boundary

The IPC protocol is a compact little-endian binary format. Do not replace it with JSON without a planned protocol migration and cross-language compatibility tests.

The pipe rejects remote clients. Its ACL is limited to the service identities, administrators, interactive users, and the AppContainer groups required by Windows text hosts. Incomplete client requests are disconnected after a short read deadline. This local pipe is not an authorization boundary between applications in the same interactive Windows environment.

Each lookup response returns a fixed maximum of five candidate slots. UI labels such as translation, place, station, and person markers are display hints; committed text must strip display-only labels.

## 記号とかなの連続入力

かなの composition 中は、`@`、`:`、`;`、括弧などの記号キーを、現在のかなと連結して確定します。日本語句読点モードでは、キーボード配列と文字幅の設定に従って記号を変換します。

`,` と `.` は候補ページ操作と共有します。現在の読みで `total_pages > 1` の候補が有効な場合だけ、それぞれ前ページ・次ページとして消費します。候補ページがない場合は通常の記号入力として確定するため、かなの直後に `、` や `。` を入力できます。Shift 付きの記号はページ操作に予約しません。

## 短いかなの候補推薦

1～2 文字のかなでは、読みの先頭一致に専用の高いスコア階層を適用し、中間一致を低い階層へ下げます。3 文字以上では既存の prefix / middle のスコア差を維持します。これにより、短い読みの先頭候補を候補ページの前方へ安定して配置します。
