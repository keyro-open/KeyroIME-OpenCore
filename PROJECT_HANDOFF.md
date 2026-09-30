# KeyroIME OpenCore 引き継ぎ

更新日: 2026-09-30（Asia/Tokyo）

## 目的と読む順序

本リポジトリは Windows x64 向け日本語 IME の公開版です。本文書、`.clinerules`、`.gitignore`、[設計](doc/OPENCORE_ARCHITECTURE.md)、[IPC 仕様](doc/IPC_PROTOCOL_V1.md)、[公開方針](doc/REPOSITORY_POLICY.md) を先に確認してください。実装と GitHub の最新状態を優先します。

## リポジトリとライセンス

- URL: `https://github.com/keyro-open/KeyroIME-OpenCore.git`
- 公開状態: Public（2026-09-30 に変更）。匿名アクセスで README と LICENSE の HTTP 200 を確認済み。
- 既定ブランチ: `main`
- 製品版数: `VERSION` の `1.0.6.16`
- 本体: [GNU GPLv3](LICENSE)。`LICENSE_en.txt` は英語正文の同一コピー、`LICENSE_ja.txt` は日本語訳。
- JMdict / JMnedict 由来の辞書: CC BY-SA 4.0。[第三者表示](THIRD_PARTY_NOTICES.md) と [辞書 manifest](src/keyro_service/assets/dictionary_manifest.json) を参照。
- README: [日本語](README.ja.md) / [English](README.en.md)。ルート README は日本語の入口。

過去の commit には廃止済みの独自ライセンス説明が残っています。公開版の現行ライセンス表示は `main` の `LICENSE` と第三者表示を確認してください。履歴は製品・連携先の commit 参照を維持するため保存します。

## 実装境界

- `src/tsf_shell`: C++17 TSF/COM DLL、候補 UI、ローカルフォールバック、通知領域 UI。
- `src/keyro_service`: Rust 辞書、予測、順位付け、ユーザー頻度、名前付きパイプサービス。
- IPC: `\\.\pipe\KeyroIME.Service.v1`、リトルエンディアンの小型バイナリ形式、1 ページ 5 候補。
- TSF DLL に Rust、完全辞書、ネットワーク処理、AI ランタイムを読み込ませない。
- 商用版固有ロジック、顧客資産、秘密鍵、認証情報を追加しない。

## 公開前監査（2026-09-30）

基準 commit `3b4c5fe` の時点で、`git fetch origin --prune` 後に `python tools/audit_public_repository.py --github` を実行し、18 commits / 374 blobs、6 PR、21 Actions runs を検査しました。GitHub の 3 件の installer artifact（EXE と SHA-256、計 6 ファイル）も `--artifacts-dir` で検査し、SHA-256 が一致しました。Gitleaks 8.30.1 による全ブランチ履歴と作業ツリーの検査も 0 件でした。定義済みの資格情報、秘密鍵、未公開の個人メール、電話番号、個人ディレクトリ、非公開リポジトリ名のパターンに該当する値は検出されませんでした。

公開直前の commit `18d52fa` でも 20 commits / 385 blobs、6 PR、25 Actions runs、4 件の installer artifact を再検査し、検出 0 件でした。公開後に GitHub Secret scanning と push protection を有効化し、初回の alert 一覧は 0 件でした。Secret scanning の非同期処理後も継続確認してください。

公開後の commit `a4a7416` では 21 commits / 386 blobs、6 PR、27 Actions runs、5 件の installer artifact を再検査しました。定義済みパターンの検出は 0 件で、最新 installer の SHA-256 も一致しました。Windows CI と Installer Packaging は双方成功しました。

監査は既知パターンと公開対象の目視確認に基づきます。新しい変更は CI の履歴監査を通し、公開前に再確認します。辞書の人名は製品機能の公開語彙であり、連絡先や顧客レコードは含めません。会社の公開連絡先と GPL 日本語訳の翻訳者名・公開連絡先は、権利表示として維持します。

## 検証と配布

```powershell
python tools/audit_public_repository.py
python tools/check_protocol_constants.py
cargo fmt --manifest-path src/keyro_service/Cargo.toml --all -- --check
```

Windows Release build と Rust/C++ smoke は `build_release.bat`、installer は `tools/build_installer.ps1`。GitHub Actions の `Windows CI` と `Windows Installer Packaging` は `main` の変更時に実行されます。現時点で Git tag と GitHub Release はありません。過去の Actions artifact は検証用で、正式な Release 配布物ではありません。

## 次の作業

1. GitHub Secret scanning の非同期結果と新しい PR / Actions を継続確認する。
2. 専用 Windows VM で管理者 install、update、uninstall、`/PURGEDATA` を確認する。
3. インストーラーの一時画像を正式画像へ差し替え、署名工程を整備する。
4. 配布版を作る場合は `VERSION` と tag を一致させ、署名、SHA-256、ライセンス表示を検証する。
