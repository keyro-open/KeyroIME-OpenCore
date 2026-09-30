"""公開前に Git の全参照先を検査する。検出値そのものは表示しない。"""

from __future__ import annotations

import re
import subprocess
import sys
import json
from io import BytesIO
from pathlib import Path
from collections import defaultdict


def git(*args: str) -> bytes:
    return subprocess.run(
        ["git", *args], check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE
    ).stdout


def gh(*args: str) -> bytes:
    return subprocess.run(
        ["gh", *args], check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE
    ).stdout


PATTERNS = {
    "private_key": re.compile(rb"-----BEGIN (?:RSA |EC |OPENSSH |DSA |ENCRYPTED )?PRIVATE KEY-----"),
    "github_token": re.compile(rb"(?:gh[pousr]_[A-Za-z0-9_]{20,}|github_pat_[A-Za-z0-9_]{30,})"),
    "openai_token": re.compile(rb"sk-(?:proj-|svcacct-)?[A-Za-z0-9_-]{20,}"),
    "aws_access_key": re.compile(rb"(?:AKIA|ASIA)[A-Z0-9]{16}"),
    "google_api_key": re.compile(rb"AIza[A-Za-z0-9_-]{35}"),
    "slack_token": re.compile(rb"xox[baprs]-[A-Za-z0-9-]{20,}"),
    "credential_url": re.compile(rb"https?://[^\s/@:]+:[^\s/@]+@[^\s/]+"),
    "literal_credential": re.compile(
        rb"(?i)(?:password|passwd|api[_-]?key|client[_-]?secret|access[_-]?token)"
        rb"\s*[=:]\s*['\"]?[A-Za-z0-9_+./=-]{12,}['\"]?"
    ),
    "local_user_path": re.compile(rb"(?i)(?:[A-Z]:\\Users\\[^\\\s]+|/Users/[^/\s]+|/home/[^/\s]+)"),
    "local_workspace_path": re.compile(rb"(?i)[A-Z]:\\(?:myCode|Desktop|Documents)\\[^\s'\"]+"),
    "private_repo_name": re.compile(
        rb"(?i)KeyroIME[_-](?:Kernel-Proprietary|Commercial-Products|Kernel|Product)(?![A-Za-z0-9_])"
    ),
    "japanese_phone": re.compile(rb"(?<!\d)0\d{1,4}-\d{1,4}-\d{3,4}(?!\d)"),
}
EMAIL = re.compile(rb"(?i)\b[A-Z0-9._%+-]+@[A-Z0-9.-]+\.[A-Z]{2,}\b")
ALLOWED_EMAILS = {
    "keyro@localpro.jp",
    "licensing@fsf.org",
    "noreply@github.com",
    "mhatta@gnu.org",  # GPL 日本語訳に必要な翻訳者表示
}
ALLOWED_EMAIL_DOMAINS = {"kernel32.dll", "user32.dll"}


def main() -> int:
    objects = {}
    for entry in git("rev-list", "--objects", "--all").splitlines():
        oid, _, path = entry.partition(b" ")
        objects.setdefault(oid.decode("ascii"), path.decode("utf-8", "replace"))

    findings: dict[str, list[str]] = defaultdict(list)
    blob_count = 0
    batch = subprocess.run(
        ["git", "cat-file", "--batch"],
        input="\n".join(objects).encode("ascii") + b"\n",
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=True,
    )
    stream = BytesIO(batch.stdout)
    for oid, path in objects.items():
        header = stream.readline().split()
        if len(header) != 3:
            raise RuntimeError(f"Git object を読み込めません: {oid}")
        data = stream.read(int(header[2]))
        stream.read(1)
        if header[1] != b"blob":
            continue
        blob_count += 1
        if b"\0" in data[:4096]:
            continue
        for line_number, line in enumerate(data.splitlines(), 1):
            for category, pattern in PATTERNS.items():
                if category == "local_user_path" and (
                    b"re.compile" in line
                    or (path == "tools/audit_public_repository.py" and b"re.search" in line)
                ):
                    continue
                if pattern.search(line):
                    findings[category].append(f"{path or '(名前なし)'}:{line_number} ({oid[:12]})")
            for match in EMAIL.finditer(line):
                value = match.group().decode("ascii", "ignore").lower()
                domain = value.rsplit("@", 1)[-1]
                if (
                    value not in ALLOWED_EMAILS
                    and domain not in ALLOWED_EMAIL_DOMAINS
                    and not value.endswith("@users.noreply.github.com")
                ):
                    findings["email_review"].append(
                        f"{path or '(名前なし)'}:{line_number} ({oid[:12]})"
                    )

    commits = git("log", "--all", "--format=%H%x09%an%x09%ae%x09%cn%x09%ce")
    for entry in commits.decode("utf-8", "replace").splitlines():
        parts = entry.split("\t")
        if len(parts) != 5:
            continue
        oid, author, author_email, committer, committer_email = parts
        for name, address in ((author, author_email), (committer, committer_email)):
            if address.lower() not in ALLOWED_EMAILS and not address.endswith("@users.noreply.github.com"):
                findings["commit_identity_review"].append(oid[:12])
            if re.search(r"(?i)(?:[A-Z]:\\Users\\|/Users/|/home/)", name):
                findings["commit_identity_review"].append(oid[:12])

    if "--github" in sys.argv[1:]:
        repository = "keyro-open/KeyroIME-OpenCore"
        pulls = json.loads(
            gh("api", f"repos/{repository}/pulls?state=all&per_page=100")
        )
        runs = json.loads(
            gh("api", f"repos/{repository}/actions/runs?per_page=100")
        )["workflow_runs"]
        for pull in pulls:
            number = pull["number"]
            for field in ("title", "body"):
                check_external(
                    (pull.get(field) or "").encode("utf-8"),
                    f"PR #{number} {field}",
                    findings,
                )
            for endpoint in ("issues", "pulls"):
                comments = json.loads(
                    gh("api", f"repos/{repository}/{endpoint}/{number}/comments?per_page=100")
                )
                for comment in comments:
                    check_external(
                        (comment.get("body") or "").encode("utf-8"),
                        f"PR #{number} {endpoint} comment",
                        findings,
                    )
        for run in runs:
            run_id = run["id"]
            try:
                log = gh("run", "view", str(run_id), "--repo", repository, "--log")
            except subprocess.CalledProcessError:
                findings["log_unavailable"].append(str(run_id))
                continue
            check_external(log, f"Actions run {run_id}", findings)
        print(f"GitHub 監査対象: {len(pulls)} PR、{len(runs)} Actions runs")

    if "--artifacts-dir" in sys.argv[1:]:
        position = sys.argv.index("--artifacts-dir")
        root = Path(sys.argv[position + 1])
        artifacts = sorted(file for file in root.rglob("*") if file.is_file())
        for file in artifacts:
            data = file.read_bytes()
            source = str(file.relative_to(root))
            check_external(data, source, findings)
            if file.suffix.lower() == ".exe":
                # PE の UTF-16LE 文字列にも同じパターンを適用する。
                check_external(data.replace(b"\x00", b""), source + " UTF-16", findings)
        print(f"Actions artifact 監査対象: {len(artifacts)} files")

    print(f"監査対象: {len(objects)} objects、{blob_count} blobs、{len(commits.splitlines())} commits")
    if findings:
        for category in sorted(findings):
            locations = sorted(set(findings[category]))
            print(f"{category}: {len(locations)} 件")
            for location in locations[:30]:
                print(f"  {location}")
            if len(locations) > 30:
                print(f"  ほか {len(locations) - 30} 件")
        print("検出値は表示していません。該当箇所を確認してください。")
        return 1
    print("定義済みパターンに該当する機密情報・個人情報は検出されませんでした。")
    return 0


def check_external(data: bytes, source: str, findings: dict[str, list[str]]) -> None:
    for line_number, line in enumerate(data.splitlines(), 1):
        for category, pattern in PATTERNS.items():
            if category == "local_user_path" and re.search(
                rb"(?i)C:\\Users\\(?:runneradmin|runner)\\", line
            ):
                continue
            if pattern.search(line):
                findings[category].append(f"{source}:{line_number}")
        for match in EMAIL.finditer(line):
            value = match.group().decode("ascii", "ignore").lower()
            domain = value.rsplit("@", 1)[-1]
            if (
                value not in ALLOWED_EMAILS
                and domain not in ALLOWED_EMAIL_DOMAINS
                and not value.endswith("@users.noreply.github.com")
            ):
                findings["email_review"].append(f"{source}:{line_number}")


if __name__ == "__main__":
    sys.exit(main())
