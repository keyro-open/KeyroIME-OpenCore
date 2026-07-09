# KeyroIME OpenCore v1.0

KeyroIME OpenCore は、Windows 向け日本語 IME「KeyroIME」の公開 OpenCore ソース基準です。

公開リポジトリ:

```text
https://github.com/keyro-open/KeyroIME-OpenCore.git
```

このリポジトリには、非公開の順位付けロジック、企業向け暗号化辞書ペイロード、非公開 SLM モデル、顧客資産、認証情報、商用インストーラー秘密情報を入れません。

KeyroIME は、Windows x64 向けのローカル優先型日本語入力システムです。日本語を書きながら、数字、記号、英単語、半角文字、日本語配列/US 配列のキーボードを頻繁に行き来するユーザーのために設計しています。

コンセプトはシンプルです。文章の流れを止めないこと。KeyroIME は「入力したい内容は分かっているのに、モード切替や全角/半角、記号入力で指が止まる」という小さなストレスを減らします。

## 最初の3秒で伝えたいこと

テキスト欄を開き、入力を始め、そのまま進める。

KeyroIME の第一印象は、設定画面でも学習コストでもありません。かな漢字変換、カタカナ外来語、数字、記号、英語、JIS/ANSI 配列切替が、最初から指の近くにある感覚です。

## プロダクトメッセージ

- モード切替、追加変換、全角/半角の戻し操作を減らし、日本語入力のテンポを上げます。
- 日本語入力中でも、半角数字、記号、英語を自然にすばやく入力できます。
- 日本語キーボードと US キーボードを行き来する職場・開発・海外勤務環境に対応します。
- カタカナ外来語、人名、地名、長い定型句を、混合辞書、曖昧予測、動的な候補連結で見つけやすくします。
- v1.0 はローカル優先です。ログイン、クラウド依存、Windows の入力ホスト内での AI ランタイム読み込みはありません。

## アーキテクチャ

- `KeyroIME.dll`: Windows の入力ホストに読み込まれる C++17 TSF/COM テキストサービス。
- `keyro_service.exe`: 辞書検索、予測、候補順位付け、ユーザー頻度、IPC を担当する Rust Windows サービス。
- `keyro_tray.exe`: 通知領域アイコン、右クリックメニュー、状態 OSD を担当する C++17 ユーザーセッションプロセス。
- IPC: `\\.\pipe\KeyroIME.Service.v1` を使うローカル名前付きパイプの軽量バイナリプロトコル。

TSF DLL は、ホストプロセス内で Rust、辞書、ユーザー設定、ネットワーク処理、AI ランタイムを読み込みません。サービスが利用できない場合も、TSF 層のローカルローマ字かな変換で入力を継続できます。

## 主な機能

- ひらがな、カタカナ、英語入力モード。
- ANSI/JIS 物理キーボード配列の切り替え。`Alt+;` が衝突するアプリではメニューから切り替えられます。
- 日英混在入力向けのデフォルト半角モード。
- 数字・記号の一打入力。
- `Shift+CapsLock` による全角/半角切替。
- `CapsLock` による英語大文字/小文字ロック。
- Shift 単押しによる和文句読点と欧文句読点の切り替え。
- 5 候補固定の縦型候補ウィンドウ、翻訳・地名・人名などのラベル表示。
- システム、高頻度、カタカナ、翻訳、人名、地名辞書の混合候補。
- `q` による翻訳候補、`v` による人名・地名・駅名候補の優先表示。
- 1 文字のかな、または漢字から始まる曖昧予測。
- まれな語、人名、地名を出しやすくする動的な分割・連結候補生成。
- `キーロ` または `ki-ro` 入力時の KeyroIME ヘルプ候補。

## リポジトリ構成

```text
src/keyro_service/         Rust 辞書・候補順位付けサービス
src/tsf_shell/             C++ TSF シェル、トレイ UI、候補ウィンドウ、スモークテスト
src/tsf_shell/resources/   Windows アイコンリソース
tools/                     辞書アセット生成ツール
include/                   FFI 互換ヘッダー
doc/                       公開引き継ぎ、アーキテクチャ、リポジトリ方針
```

## ビルドとテスト

必要環境:

- Windows x64
- Visual Studio 2022 C++ デスクトップ開発環境
- Windows SDK
- Rust stable と `x86_64-pc-windows-msvc`
- CMake 3.20 以上

基本コマンド:

```bat
build_release.bat
```

`src\tsf_shell\build\Release\` に、TSF activation、local fallback、tray menu、runtime input、IPC failover などのスモークテストが生成されます。

## インストール

インストールには管理者権限が必要です。

```bat
release\install.bat /silent
```

TSF DLL を更新した後は、古い DLL を読み込んだホストを残さないように、Explorer/CTF の再起動またはサインアウト・サインインを推奨します。

## データ安全性とソース管理

ビルド成果物、リリースバイナリ、ログ、ローカルパス、認証情報、`.env`、証明書、生成キャッシュは Git 管理対象外です。push 前に確認してください。

```powershell
git status --short
git status --ignored --short
```

## ライセンスと第三者表示

KeyroIME OpenCore ソースコードは GPL v3 でライセンスされます。`LICENSE` を参照してください。

辞書アセットにはプロジェクト独自補足と第三者由来リソースが含まれます。詳細は `THIRD_PARTY_NOTICES.md` と `src/keyro_service/assets/dictionary_manifest.json` を参照してください。
