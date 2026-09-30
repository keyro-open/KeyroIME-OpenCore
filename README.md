# KeyroIME OpenCore

Windows x64 向け日本語入力システム KeyroIME のオープンソース版です。ローカルで動作する辞書サービスと軽量な TSF シェルを組み合わせ、日本語・英語・数字・記号を続けて入力できます。

詳細な README は言語別に用意しています。

- [日本語 README](README.ja.md)
- [English README](README.en.md)

## 主な機能

- かな漢字変換、カタカナ語、名前・地名、翻訳候補と短いかなの予測。
- かな入力中の記号入力と、複数候補ページがある場合の `,` / `.` ページ送り。
- JIS / ANSI キーボード切り替え、半角・全角切り替え、ローカル入力フォールバック。
- Windows TSF 用 C++17 シェル、Rust 辞書サービス、通知領域 UI。

## 開発とライセンス

Windows x64、Visual Studio 2022、Windows SDK、CMake、Rust が必要です。ビルド・インストール手順は [日本語 README](README.ja.md) または [English README](README.en.md) を参照してください。

本体は [GNU GPLv3](LICENSE) に基づく自由ソフトウェアです。第三者辞書の条件は [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) と辞書 manifest に記載しています。

セキュリティ上の問題は [SECURITY.md](SECURITY.md) に従って非公開で報告してください。公開前監査の範囲と再実行手順は [PROJECT_HANDOFF.md](PROJECT_HANDOFF.md) に記録しています。
