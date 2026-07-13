# KeyroIME OpenCore 引き継ぎ

スナップショット: 2026-07-13 (Asia/Tokyo)

この文書は、次の担当者が最短で作業を再開するための運用スナップショットです。安定した設計情報は `PROJECT_CONTEXT.md`、詳細は `doc/PROJECT_HANDOFF.md` を参照してください。ソースコードと GitHub の最新状態が文書と異なる場合は、ソースコードと GitHub を正とします。

## 最初に確認するファイル

1. `.clinerules` と `.gitignore`
2. `PROJECT_HANDOFF.md`
3. `doc/PROJECT_HANDOFF.md`
4. `PROJECT_CONTEXT.md`
5. `doc/OPENCORE_ARCHITECTURE.md`、`doc/IPC_PROTOCOL_V1.md`、`doc/REPOSITORY_POLICY.md`
6. 対象タスクに関係するソースコード

大容量の辞書 TSV は、辞書の取り込み、監査、またはリリース検証が必要な場合に限り全件処理します。通常はマニフェスト、ファイルサイズ、行数、完全一致検索、少量サンプルを使用します。

## Git と GitHub

- リポジトリ: `https://github.com/keyro-open/KeyroIME-OpenCore.git`
- 可視性: Private。公開前確認後に Public へ移行する予定。
- デフォルトブランチ: `main`
- 作業ブランチ: `agent/resolve-public-release-risks`
- 作業ブランチの基点: `b623d2474a103080d51287931f7714c9b5c33fad`
- Git release tag: 未作成。
- PR #2: マージ済み。
- PR #1: Draft、未マージ。この作業ブランチは PR #1 の日英 README 改善を取り込んでいるため、本ブランチのマージ後に PR #1 を重複 PR として整理する。

## ライセンス基準

KeyroIME OpenCore は OSI 定義のオープンソースではなく、個人・非商用向けのソース公開版です。

- 適用ライセンス: `KeyroIME OpenCore Non-Commercial Source License 1.0`
- 個人は非商用目的で利用、調査、改変、共有できる。
- 共有時は同じライセンスで完全な対応ソースを提供し、LocalPro と元プロジェクトを出典表示し、改変内容を明示する。
- 会社での利用およびその他の商用利用には、株式会社LocalProの書面による別途ライセンスが必要。
- 外部コントリビューションは同ライセンスで公開し、株式会社LocalProに再ライセンスと商業利用を含む非独占的権利を付与する。
- 第三者辞書は各ライセンスを継続適用する。
- `LICENSE` と `LICENSE_en.txt` は同一の英語正文であり、`LICENSE_ja.txt` は日本語訳。

公開説明では `open source` ではなく `source-available` を使用します。

## 製品とセキュリティ基準

- 製品バージョン: `1.0.6.15`。リポジトリルートの `VERSION` を CMake、UI、ビルド、インストーラーの正とする。
- Rust package version: `1.0.6+15`。
- `KeyroIME.dll`: C++17 TSF/COM テキストサービス。
- `keyro_service.exe`: Rust 2021 辞書、予測、順位付け、ユーザー頻度、IPC サービス。
- `keyro_tray.exe`: C++17 通知領域、OSD、About、License UI。
- IPC: `\\.\pipe\KeyroIME.Service.v1` の小型リトルエンディアン・バイナリプロトコル。

セキュリティ変更:

- サービスアカウントを `LocalSystem` から `LocalService` へ変更。
- 名前付きパイプはリモートクライアントを拒否する。
- ACL から広範な Authenticated Users を外し、サービス、管理者、対話ユーザー、必要な AppContainer グループに限定。
- 不完全なクライアント要求は 250 ms で切断し、クライアントは stale pipe を 1 回だけ高速再接続する。
- IPC 入力は C++ と Rust の両方で 4096 bytes に制限。
- ユーザー辞書フィールドは 256 文字、WAL は 16 MiB に制限。
- `%ProgramData%\KeyroIME` は `LocalService` のみ変更可能にする。
- 同じ対話 Windows 環境内のアプリ間では、パイプを認証境界として扱わない。詳細は `SECURITY.md`。

## 辞書由来

- JMdict/JMnedict 由来データは `CC BY-SA 4.0` と既存第三者表示を維持する。
- curated supplement とローカル取り込みデータは、株式会社LocalProが作成・所有するオリジナルデータ。
- 公開 manifest は `tools/prepare_release_manifest.py` を通し、開発機パスと内部準備名を除去する。
- Release build は sample/full のどちらでも公開用 manifest を再生成する。

## 2026-07-13 検証結果

- Rust Release tests: 55 passed、0 failed。
- Rust formatting: 成功。
- IPC C++/Rust 定数互換検査: 成功。
- Release x64 build: 成功。
- TSF activation smoke: 成功。
- Local fallback smoke: 成功。
- Candidate tag smoke: 成功。
- Runtime input smoke: 成功。
- Tray menu smoke: 成功。
- IPC failover smoke: 約 8.19 ms、目標 10 ms 未満。
- 加固済み独立サービス IPC 100 回: 平均約 1.62 ms、p95 約 1.97 ms。
- 400 ms idle 後の pipe 再接続: 平均約 1.02 ms。
- Release manifest のローカルパス検査: 成功。
- Release SHA-256: 11 ファイル検証成功。
- `install.bat /validate`: 成功。
- `uninstall.bat /validate`: 成功。
- `git diff --check`: 成功。

管理者権限を使う実インストール・アンインストールは、既存 IME と Windows サービスを変更するため、この作業中には実行していません。公開配布前に専用テスト環境で確認します。

## 解決済み P1

- `Cargo.lock` を追跡し、Rust `1.96.0` と rustfmt を固定。
- Windows GitHub Actions を追加。
- `VERSION` に製品バージョンを一元化。
- 旧 `src/ime_core` を参照していた `build_sandbox.bat` を修正。
- `uninstall.bat` と `/purge` を追加。
- Release SHA-256 生成と installer 検証を追加。
- IPC v1 仕様書と C++/Rust 定数互換検査を追加。
- 旧 `state_machine.rs` を test-only にし、production dead-code 警告を除去。
- `SECURITY.md` と `CONTRIBUTING.md` を追加。

## 次に行う作業

1. このブランチの Draft PR をレビューし、CI 成功後にマージする。
2. 専用 Windows テスト環境で管理者インストール、サービスアカウント、更新、アンインストール、`/purge` を確認する。
3. Notepad、Chromium/Electron、AppContainer/UWP の実ホスト入力を継続検証する。
4. ライセンス正文を日本法の専門家に最終確認する。
5. コード署名証明書をリポジトリ外で管理し、署名済み Release を作成する。
6. 旧 commit には廃止済み GPL 文面が含まれるため、Public 変更前に最終 tree から clean public baseline を作成するか、明示的な承認を得て history を rewrite する。現在の history をそのまま Public にしない。
7. clean baseline に対して現行 tree と Git history を再監査し、`v1.0.6.15` tag を作成する。
8. C++ と Rust の IPC 定数は互換検査で保護済み。将来は必要に応じて単一コード生成へ移行する。

## 変更時の必須条件

- リポジトリ内の文章、コメント、commit message は日本語または英語を使用する。
- TSF DLL に Rust、完全辞書、ネットワーク、AI ランタイムを読み込ませない。
- COM ロック保持中に遅い IPC を待たない。別スレッドから `ITfContext` を直接操作しない。
- `release/`、`dist/`、`target/`、`build/`、バイナリ、ログ、証明書、認証情報を Git に追加しない。
- push 前にリモートを読み取り確認し、`git status --short --branch` と `git diff --check` を実行する。
- TSF DLL 更新後は Explorer/CTF を再起動するか、サインアウト・サインインして旧 DLL のマッピングを解放する。
