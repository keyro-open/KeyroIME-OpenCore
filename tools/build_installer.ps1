param(
    [string]$ReleaseDirectory = (Join-Path $PSScriptRoot '..\release'),
    [string]$OutputDirectory = (Join-Path $PSScriptRoot '..\dist'),
    [string]$IsccPath = '',
    [switch]$UiTestMode
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$version = (Get-Content -Raw -LiteralPath (Join-Path $root 'VERSION')).Trim()
if ($version -notmatch '^\d+\.\d+\.\d+\.\d+$') {
    throw "VERSION must contain four numeric components: $version"
}

$release = (Resolve-Path -LiteralPath $ReleaseDirectory).Path
$requiredFiles = @(
    'KeyroIME.dll',
    'keyro_service.exe',
    'keyro_tray.exe',
    'LICENSE_ja.txt',
    'LICENSE_en.txt',
    'THIRD_PARTY_NOTICES.md',
    'dictionary_manifest.json'
)

foreach ($name in $requiredFiles) {
    $path = Join-Path $release $name
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Installer payload is missing: $path"
    }
}

$binaryHashText = (($requiredFiles[0..2] | ForEach-Object {
    (Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $release $_)).Hash
}) -join '')
$sha256 = [System.Security.Cryptography.SHA256]::Create()
try {
    $buildIdBytes = $sha256.ComputeHash([System.Text.Encoding]::ASCII.GetBytes($binaryHashText))
    $buildId = ([System.BitConverter]::ToString($buildIdBytes)).Replace('-', '').Substring(0, 12)
}
finally {
    $sha256.Dispose()
}

if ([string]::IsNullOrWhiteSpace($IsccPath)) {
    $command = Get-Command ISCC.exe -ErrorAction SilentlyContinue
    $candidates = @(@(
            $(if ($command) { $command.Source }),
            (Join-Path $env:LOCALAPPDATA 'Programs\Inno Setup 6\ISCC.exe'),
            'C:\Program Files (x86)\Inno Setup 6\ISCC.exe',
            'C:\Program Files\Inno Setup 6\ISCC.exe'
        ) | Where-Object { $_ -and (Test-Path -LiteralPath $_) })
    if (-not $candidates) {
        throw 'ISCC.exe was not found. Install Inno Setup 6 before building the installer.'
    }
    $IsccPath = $candidates[0]
}
elseif (-not (Test-Path -LiteralPath $IsccPath -PathType Leaf)) {
    throw "ISCC.exe was not found: $IsccPath"
}

$output = [System.IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Path $output -Force | Out-Null

$script = Join-Path $root 'installer\KeyroIME.iss'
$arguments = @(
    '/Qp',
    "/DMyAppVersion=$version",
    "/DBuildId=$buildId",
    "/DReleaseDir=$release",
    "/DOutputDir=$output"
)
if ($UiTestMode) {
    $arguments += '/DUiTestMode=1'
    $arguments += "/FKeyroIME_UI_Test_v$version"
}
$arguments += $script

& $IsccPath @arguments
if ($LASTEXITCODE -ne 0) {
    throw "Inno Setup compilation failed with exit code $LASTEXITCODE."
}

$baseName = if ($UiTestMode) { "KeyroIME_UI_Test_v$version.exe" } else { "KeyroIME_Setup_v$version.exe" }
$installer = Join-Path $output $baseName
if (-not (Test-Path -LiteralPath $installer -PathType Leaf)) {
    throw "Expected installer was not generated: $installer"
}

Write-Host "Installer: $installer"
Write-Host "Build ID: $buildId"
