# Windows EXE インストーラーと自動パッケージ

## 配布形式

GitHub Actions の `Windows Installer Packaging` ワークフローは、Windows x64 向けの単一 EXE インストーラーを生成します。

- ファイル名: `KeyroIME_Setup_v<VERSION>.exe`
- `main` への push: 30 日間保持する Actions artifact を生成
- pull request: パッケージの再現性とインストーラー契約を検証
- `v*` tag: artifact に加えて GitHub Release へ EXE と SHA-256 ファイルを公開
- 手動実行: `workflow_dispatch` から任意の branch を検証

tag は `v` と `VERSION` の値を連結した文字列と一致する必要があります。例: `VERSION` が `1.0.6.15` の場合、tag は `v1.0.6.15` です。

## 対話式インストール

Inno Setup 6 のウィザードは次の順序で表示されます。

1. 案内画面: カバー画像、インストール先、著作権・ライセンス表示
2. 製品紹介 1: 3 秒
3. 製品紹介 2: 3 秒
4. 広告枠 3: 3 秒
5. 広告枠 4: 3 秒
6. システム統合: TSF 登録、`LocalService` サービス登録、ACL 設定
7. 完了画面: 完了画像、基本操作、tray 起動オプション

2 から 5 は 100 ms 単位の仮想進捗です。実際のファイル展開時間に関係なく、12 秒の一巡が完了するまで戻る、進む、キャンセル、ウィンドウを閉じる操作はできません。サイレントインストールでは紹介画面を省略します。

## インストール対象の許可リスト

インストール先へコピーするファイルは次の 7 件に限定します。

- `KeyroIME.dll`
- `keyro_service.exe`
- `keyro_tray.exe`
- `LICENSE_ja.txt`
- `LICENSE_en.txt`
- `THIRD_PARTY_NOTICES.md`
- `dictionary_manifest.json`

`doc/`、`PROJECT_HANDOFF.md`、build/test/smoke/bench の実行ファイル、`.bat`、`.ps1`、`.py` はインストール対象外です。6 枚の UI bitmap は Setup EXE に埋め込み、製品ディレクトリへはコピーしません。`tools/check_installer_contract.py` がこの許可リストを CI で検証します。

## 画像の差し替え

一時的な画像は `installer/assets/` にあります。正式画像も Windows bitmap、`640 x 280` pixel で作成し、既存ファイル名を保って差し替えます。

- `cover.bmp`
- `progress-1.bmp`
- `progress-2.bmp`
- `progress-3.bmp`
- `progress-4.bmp`
- `finish.bmp`

プレースホルダーを再生成する場合:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/generate_installer_placeholders.ps1
```

## ローカル生成と検証

Visual Studio 2022、Windows SDK、CMake、Rust 1.96.0、Python 3、Inno Setup 6 が必要です。

```powershell
build_release.bat
powershell -NoProfile -ExecutionPolicy Bypass -File tools/build_installer.ps1
python tools/check_installer_contract.py --installer dist/KeyroIME_Setup_v1.0.6.15.exe
```

システムを変更せずに画面遷移を確認する場合:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/build_installer.ps1 -UiTestMode -OutputDirectory target/installer_ui_test
powershell -NoProfile -ExecutionPolicy Bypass -File tools/test_installer_ui.ps1
```

UI test 版は管理者権限を要求せず、12 秒後に自動インストールせず停止します。画面 capture は `target/installer_ui_test/screenshots/` に生成されます。正式配布には使用しません。

出力 EXE と `release/`、`dist/` は Git 管理対象外です。

## アンインストールとデータ保持

Windows の「インストールされているアプリ」からアンインストールできます。既定では `%ProgramData%\KeyroIME` の学習辞書を保持します。学習辞書も削除する場合は、管理者の command prompt から uninstaller に `/PURGEDATA` を指定します。

## 公開前の残作業

- LocalPro の code-signing 証明書を GitHub Actions secret として登録し、Setup EXE と uninstaller の署名工程を追加する。
- 専用のクリーンな Windows 10/11 VM で install、update、uninstall、`/PURGEDATA`、日本語・英語表示、`LocalService` と ACL を確認する。
- 正式なカバー、製品紹介、広告、完了画像へ差し替える。
