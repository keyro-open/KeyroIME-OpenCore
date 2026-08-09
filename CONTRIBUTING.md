# KeyroIME OpenCore へのコントリビューション

KeyroIME OpenCore は、公開 GPLv3 基準に沿った修正と改善を歓迎します。

## 提出前の確認

- リポジトリ内の新規文章、コメント、コミット、issue および pull request は日本語で記述してください。
- 認証情報、ローカルパス、顧客データ、非公開モデル、インストーラー秘密情報または非公開順位付けロジックを提出しないでください。
- TSF DLL の内部に Rust、完全辞書、ネットワークコンポーネントまたは AI ランタイムを配置しないでください。
- 明示的な移行計画と異言語互換テストがない限り、軽量な `\\.\pipe\KeyroIME.Service.v1` プロトコルを互換に保ってください。
- 辞書追加には、再配布権と由来を記録してください。

## コントリビューションのライセンス

コントリビューションを提出することで、提出する権利を有し、GNU General Public License version 3 の下で配布できることを確認するものとします。第三者素材には各自のライセンスが適用されるため、明確に識別してください。

## 検証

変更に関連する検証を実行してください。Windows Release の全検証は次のコマンドです。

```bat
build_release.bat
```

Before committing:

```powershell
git diff --check
git status --short --branch
```

pull request には、変更理由、利用者への影響、実施したテストおよび既知の制限を記載してください。
