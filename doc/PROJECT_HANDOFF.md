# KeyroIME OpenCore 技術引き継ぎ

スナップショット: 2026-07-13 (Asia/Tokyo)

## 目的と公開範囲

KeyroIME OpenCore は、Windows x64 向け日本語 IME「KeyroIME」の個人・非商用向けソース公開基準です。TSF シェル、ローカル辞書サービス、通知領域プロセス、公開辞書サンプル、ビルド・検証ツール、公開文書を含みます。

このリポジトリには、非公開の商用順位付けロジック、企業向け暗号化辞書、非公開 SLM、顧客資産、認証情報、証明書、署名鍵、商用インストーラー秘密情報、ローカルマシン固有パス、非公開組織・リポジトリ情報を入れません。

## ライセンスモデル

適用ライセンスは `KeyroIME OpenCore Non-Commercial Source License 1.0` です。これは OSI 定義のオープンソースライセンスではありません。

- 自然人が自己のために行う非商用利用、調査、改変、共有を許可する。
- 共有時は、完全な対応ソース、同一ライセンス、LocalPro と元プロジェクトの出典、改変日と改変概要を提供する。
- 会社、団体、雇用主、顧客、有償サービス、収益活動その他の商用利用には、株式会社LocalProの書面による別途ライセンスが必要。
- 外部コントリビューションは同ライセンスで公開し、株式会社LocalProに再ライセンスと商業利用を含む非独占的権利を付与する。
- 第三者コード・辞書はそれぞれのライセンスを適用する。
- 英語正文は `LICENSE` と `LICENSE_en.txt`、日本語訳は `LICENSE_ja.txt`。

公開文書と UI は `source-available` または「ソース公開版」と表現し、`open source` と表現しません。

## GitHub とブランチ

- URL: `https://github.com/keyro-open/KeyroIME-OpenCore.git`
- 可視性: Private。
- default branch: `main`
- base commit: `b623d2474a103080d51287931f7714c9b5c33fad`
- current work branch: `agent/resolve-public-release-risks`
- release tag: 未作成。
- PR #2: マージ済み。
- PR #1: Draft。README 改善は current work branch に取り込み済み。本ブランチのマージ後に重複 PR として整理する。

## 実行時アーキテクチャ

```text
keyro_tray.exe
        |
        | per-session shared settings
        v
Windows text host -- TSF/COM --> KeyroIME.dll
        |                          |
        | compact local pipe       | local fallback
        v                          |
keyro_service.exe (LocalService) <-+
        |
        v
embedded dictionaries, prediction, ranking, bounded user WAL
```

### プロセス責務

- `src/tsf_shell`: C++17 TSF/COM DLL、キー処理、composition、候補 UI、非同期 IPC、ローカルフォールバック。
- `src/keyro_service`: Rust 辞書検索、予測、順位付け、paging、ユーザー頻度、名前付きパイプサーバー。
- `keyro_tray.exe`: 通知領域メニュー、OSD、About、License、共有設定。
- `src/keyro_service/assets`: 公開辞書サンプルと manifest。
- `tools`: 辞書生成、公開 manifest 生成、IPC 互換検査。

TSF DLL は Windows の複数ホストへ読み込まれるため、Rust、完全辞書、設定ファイル、ネットワーク処理、AI ランタイムを読み込みません。サービスが停止しても `Activate` は成功し、ローカル入力を継続します。

### TSF/COM 固定条件

- CLSID: `{8B4F9B54-7B15-4D8C-9E32-6D5A17B0A51E}`
- Profile GUID: `{6C33B9A3-5CE6-4D27-8A9F-0E9B60F0E7B9}`
- LangID: `0x0411`
- Category: `GUID_TFCAT_TIP_KEYBOARD`
- COM `ThreadingModel`: `Apartment`
- COM lock 中に遅い IPC を待たない。
- worker thread から `ITfContext` を直接操作しない。
- async response は query と page が現在値に一致する場合だけ適用する。

## IPC v1 とセキュリティ

Canonical specification: `doc/IPC_PROTOCOL_V1.md`

- pipe: `\\.\pipe\KeyroIME.Service.v1`
- little-endian binary protocol
- request header: 9 bytes
- response header: 8 bytes
- request payload: 最大 4096 UTF-8 bytes
- page size: 5
- request types: lookup `1`、selection `2`、settings `3`

セキュリティ条件:

- Windows service account は `NT AUTHORITY\LocalService`。
- `PIPE_REJECT_REMOTE_CLIENTS` を使用する。
- pipe ACL は `SYSTEM`、`Administrators`、`LocalService`、`Interactive Users`、必要な AppContainer groups に限定する。
- 接続後 250 ms 以内に完全な header/payload が届かなければ切断する。
- client は broken/stale pipe の場合、8 ms の全体 deadline 内で 1 回だけ再接続する。
- C++ client と Rust server の定数は `tools/check_protocol_constants.py` で照合する。
- 同じ対話 Windows 環境内の app 間では、この pipe を認証境界として扱わない。

### ユーザー辞書

- WAL path: `%ProgramData%\KeyroIME\user_dictionary.wal`
- data directory は `LocalService` modify、`SYSTEM`/`Administrators` full control。
- reading/surface は tab、CR、LF を除去し、各 256 文字に制限する。
- WAL は 16 MiB で追加を停止する。
- 候補は reading と surface の predictive index に登録する。
- user base score は `30,000`、選択ごとに `200` を加算する。
- user exact boost は `24,000`、source boost は `30,000`。

## 辞書と候補

- `q` は translation を優先する。
- `v` は name/place/station を優先する。
- prefix prediction は middle prediction より上位。
- sort tie-break は source、match、base score、surface text の順で安定化する。
- 人名・地名・駅名は内部種別を維持し、UI 表示時のみ `名` に正規化する。
- translation UI tag は `訳`。表示 tag は commit text に含めない。

辞書由来:

- JMdict/JMnedict derivative は `CC BY-SA 4.0`。
- curated supplement と local import は株式会社LocalProのオリジナルデータ。
- `tools/prepare_release_manifest.py` は LocalPro provenance を設定し、local path を拒否する。
- `THIRD_PARTY_NOTICES.md` と manifest を release に含める。

## FFI

`src/keyro_service/src/lib.rs` の C ABI は互換実験用であり、本番 TSF 経路ではありません。

- `match_romaji`
- `get_candidates`
- `evaluate_frequency`
- `free_string`

Rust が返す pointer は Rust 所有です。caller は内容を copy 後、同じ pointer を `free_string` で 1 回だけ解放します。panic は ABI 境界外へ伝播させません。

## ビルド、CI、Release

標準コマンド:

```bat
build_release.bat
```

Version source:

- product: root `VERSION` = `1.0.6.15`
- Rust SemVer: `1.0.6+15`
- pinned Rust: `1.96.0`、rustfmt、x86_64-pc-windows-msvc
- `Cargo.lock`: tracked

`build_release.bat` は次を実行します。

1. license copy consistency
2. IPC protocol compatibility
3. Rust release build/test
4. Visual Studio 2022 x64 CMake build
5. TSF activation、local fallback、candidate tag、runtime input、tray menu、IPC failover smoke
6. public dictionary manifest preparation
7. release SHA-256 generation

GitHub Actions は Windows build、55 Rust tests、非対話 smoke、secured service IPC integration を実行します。

Release package:

- `KeyroIME.dll`
- `keyro_service.exe`
- `keyro_tray.exe`
- `install.bat` / `uninstall.bat`
- `VERSION`
- license / third-party notices / public dictionary manifest / release README
- `SHA256SUMS.txt`

Installer は checksum と x64 PE を確認し、service を `LocalService` で登録します。`uninstall.bat` は既定で WAL を保持し、`/purge` 指定時のみ削除します。`/validate` はシステム変更なしで package/script 前提を確認します。

## 2026-07-13 検証

- Rust release tests: 55 passed、0 failed。
- Rust format: passed。
- C++ Release x64: passed。
- 全 6 smoke: passed。
- IPC failover: 約 8.19 ms、10 ms 未満。
- secured service 100 requests: average 約 1.62 ms、p95 約 1.97 ms。
- 400 ms idle reconnect: average 約 1.02 ms。
- public full manifest: local path/旧 provenance なし。
- release checksum: 11 files passed。
- installer/uninstaller `/validate`: passed。
- GitHub Actions Windows CI: passed（run `29257686514`）。

未実施:

- 管理者権限による実 install/update/uninstall/purge。既存の Windows IME と service を変更するため、専用テスト環境で実施する。
- Authenticode signing。証明書と鍵は repository 外で管理する。

## 残存リスクと次の作業

1. Draft PR の CI と review を完了し、`main` へ merge する。
2. 専用 VM で install/update/uninstall/purge と `LocalService` ACL を確認する。
3. Notepad、Chromium/Electron、AppContainer/UWP の実 host test を継続する。
4. 非商用 source license を日本法の専門家に最終確認する。
5. repository 外の code-signing pipeline を整備する。
6. 旧 commit には廃止済み GPL 文面が含まれる。Public 化前に最終 tree から clean public baseline を作成するか、明示的承認の下で history rewrite を実施する。既存 history をそのまま公開しない。
7. clean baseline の current tree と全 Git refs を再監査する。
8. `v1.0.6.15` release tag と GitHub Release を作成する。

TSF DLL 更新時は、Explorer/CTF 再起動または sign-out/sign-in により旧 DLL mapping を解放します。
