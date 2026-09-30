# KeyroIME OpenCore 技術引き継ぎ

更新日: 2026-09-30（Asia/Tokyo）。公開状態と監査結果は [ルートの引き継ぎ](../PROJECT_HANDOFF.md) を参照してください。

## 実行時構成

```text
Windows text host → KeyroIME.dll (TSF/COM) → 名前付きパイプ → keyro_service.exe
                            ↑                                    ↓
                       keyro_tray.exe                    辞書・順位付け・WAL
```

- `KeyroIME.dll`: C++17。かな入力、候補 UI、非同期 IPC、サービス停止時のローカル入力。
- `keyro_service.exe`: Rust。`LocalService` で動作し、辞書・予測・候補順位・ユーザー頻度を担当。
- `keyro_tray.exe`: ユーザーセッションの通知領域と設定 UI。
- IPC: `\\.\pipe\KeyroIME.Service.v1`。最大入力 4096 bytes、5 候補/ページ、C++/Rust 定数は `tools/check_protocol_constants.py` で照合。
- TSF DLL は Rust、完全辞書、設定ファイル、ネットワーク処理、AI ランタイムをホストプロセス内へ読み込まない。

詳細は [アーキテクチャ](OPENCORE_ARCHITECTURE.md)、[IPC v1](IPC_PROTOCOL_V1.md)、[セキュリティ方針](../SECURITY.md) に分離しています。

## 入力・辞書の基準

- `q` は翻訳候補、`v` は人名・地名・駅名候補を優先する。
- 1～2 文字のかなでは読みの先頭一致を優先する。
- かなの composition 中に記号を入力できる。`,` / `.` は複数候補ページがある場合だけページ操作となる。
- 辞書の公開サンプルと manifest は `src/keyro_service/assets/`。大容量 TSV の通常確認では行数・サイズ・少量サンプルを使用する。
- JMdict/JMnedict 由来データは CC BY-SA 4.0、独自補足は GNU GPLv3。由来と数量は manifest を正とする。

## セキュリティ境界

- パイプは remote client を拒否する。ACL は必要なローカル主体に限定し、不完全な要求は 250 ms 以内に切断する。
- 同じ対話 Windows 環境のアプリ間ではパイプを認証境界として扱わない。
- ユーザー WAL は `%ProgramData%\KeyroIME` に保存し、サイズと入力フィールド長を制限する。
- 署名鍵、証明書、認証情報、個人情報、商用版専用データを Git に入れない。

## ビルドとテスト

```powershell
python tools/audit_public_repository.py
python tools/check_protocol_constants.py
cargo fmt --manifest-path src/keyro_service/Cargo.toml --all -- --check
build_release.bat
```

`build_release.bat` は Rust Release build/test、Visual Studio x64 CMake build、TSF・fallback・候補・入力キー・IPC の smoke、公開辞書 manifest、SHA-256 を検証します。CI は非対話 smoke と secured IPC integration を実行します。installer の契約と手動検証は [パッケージ文書](INSTALLER_PACKAGING.md) を参照してください。

過去の検証基準: 2026-09-06 に Rust Release 56 tests、C++ 7 smoke tests、Release build が成功。今回の公開前作業は文書と監査ツールの変更であり、実インストールは行っていません。

## GitHub 公開運用

- `main` の現行ファイルを配布基準とする。過去 commit の廃止済みライセンス説明は現行文書と混同しない。
- 監査ツールは Git の全ローカル参照先を検査する。GitHub の PR と Actions は `--github`、ダウンロードした artifact は `--artifacts-dir <directory>` で追加検査する。
- `Windows CI` は push/PR ごとに監査ツールを実行する。疑わしい値が検出された場合、値をログへ表示せずに失敗する。
- Git tag と GitHub Release は未作成。Actions artifact を正式版として案内しない。
- 変更時は `git status --short --branch`、`git diff --check`、ライセンス/第三者表示、CI を確認する。
