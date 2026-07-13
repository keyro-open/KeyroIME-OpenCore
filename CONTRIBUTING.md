# Contributing to KeyroIME OpenCore

KeyroIME OpenCore welcomes focused fixes and improvements that fit the public, source-available product baseline.

## Before Submitting

- Use Japanese or English in repository files, comments, commits, issues, and pull requests.
- Do not submit credentials, local paths, customer data, private models, commercial installer secrets, or closed-source ranking logic.
- Do not place Rust, full dictionaries, network components, or AI runtimes inside the TSF DLL.
- Keep the compact `\\.\pipe\KeyroIME.Service.v1` protocol compatible unless the change includes an explicit migration plan and cross-language tests.
- Confirm that dictionary additions have documented redistribution rights and provenance.

## License of Contributions

By submitting a contribution, you confirm that you have the right to provide it, agree that it may be distributed under the KeyroIME OpenCore Non-Commercial Source License 1.0, and grant LocalPro Co., Ltd. the additional rights described in Section 7 of that license, including the right to relicense and commercially use the contribution. Third-party materials remain subject to their own licenses and must be clearly identified.

Contributions made by or on behalf of a company or other legal entity require a separate written agreement with LocalPro Co., Ltd. before submission.

## Validation

Run the checks relevant to the change. For a full Windows release validation:

```bat
build_release.bat
```

Before committing:

```powershell
git diff --check
git status --short --branch
```

Describe the reason for the change, user-visible impact, tests performed, and any known limitations in the pull request.
