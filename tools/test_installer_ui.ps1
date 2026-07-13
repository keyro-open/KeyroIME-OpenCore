param(
    [string]$InstallerPath = '',
    [string]$OutputDirectory = (Join-Path $PSScriptRoot '..\target\installer_ui_test\screenshots')
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$version = (Get-Content -Raw -LiteralPath (Join-Path $root 'VERSION')).Trim()
if ([string]::IsNullOrWhiteSpace($InstallerPath)) {
    $InstallerPath = Join-Path $root "target\installer_ui_test\KeyroIME_UI_Test_v$version.exe"
}
$installer = (Resolve-Path -LiteralPath $InstallerPath).Path
if ([IO.Path]::GetFileName($installer) -ne "KeyroIME_UI_Test_v$version.exe") {
    throw 'Only an installer compiled with -UiTestMode can be used for UI capture.'
}

Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes
Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.Windows.Forms
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;

public static class InstallerUiNative
{
    [DllImport("user32.dll")]
    public static extern bool SetForegroundWindow(IntPtr windowHandle);

    [DllImport("user32.dll")]
    public static extern bool SetWindowPos(
        IntPtr windowHandle,
        IntPtr insertAfter,
        int x,
        int y,
        int width,
        int height,
        uint flags);
}
'@

$output = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Path $output -Force | Out-Null

function Save-WindowScreenshot {
    param(
        [System.Windows.Automation.AutomationElement]$Window,
        [string]$Name
    )

    $bounds = $Window.Current.BoundingRectangle
    $width = [Math]::Max(1, [int][Math]::Ceiling($bounds.Width))
    $height = [Math]::Max(1, [int][Math]::Ceiling($bounds.Height))
    $bitmap = New-Object System.Drawing.Bitmap(
        $width,
        $height,
        [System.Drawing.Imaging.PixelFormat]::Format24bppRgb)
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    try {
        $graphics.CopyFromScreen(
            [int]$bounds.Left,
            [int]$bounds.Top,
            0,
            0,
            $bitmap.Size,
            [System.Drawing.CopyPixelOperation]::SourceCopy)
        $bitmap.Save((Join-Path $output $Name), [System.Drawing.Imaging.ImageFormat]::Png)
    }
    finally {
        $graphics.Dispose()
        $bitmap.Dispose()
    }
}

$process = Start-Process -FilePath $installer -ArgumentList '/LANG=english' -PassThru
$uiProcessId = 0
try {
    $window = $null
    for ($attempt = 0; $attempt -lt 100; $attempt++) {
        $desktop = [System.Windows.Automation.AutomationElement]::RootElement
        $windows = $desktop.FindAll(
            [System.Windows.Automation.TreeScope]::Children,
            [System.Windows.Automation.Condition]::TrueCondition)
        foreach ($candidate in $windows) {
            if ($candidate.Current.Name -like "Setup - KeyroIME OpenCore $version*") {
                $window = $candidate
                $uiProcessId = $candidate.Current.ProcessId
                break
            }
        }
        if ($null -ne $window) { break }
        Start-Sleep -Milliseconds 100
    }
    if ($null -eq $window) {
        throw 'The installer window did not appear.'
    }

    $windowHandle = [IntPtr]$window.Current.NativeWindowHandle
    [void][InstallerUiNative]::SetWindowPos(
        $windowHandle,
        [IntPtr](-1),
        0,
        0,
        0,
        0,
        0x0013)

    Start-Sleep -Milliseconds 500
    Save-WindowScreenshot -Window $window -Name '00-cover.png'

    [void][InstallerUiNative]::SetForegroundWindow($windowHandle)
    Start-Sleep -Milliseconds 100
    [System.Windows.Forms.SendKeys]::SendWait('{ENTER}')

    Start-Sleep -Milliseconds 700
    Save-WindowScreenshot -Window $window -Name '01-product.png'
    Start-Sleep -Milliseconds 3000
    Save-WindowScreenshot -Window $window -Name '02-product.png'
    Start-Sleep -Milliseconds 3000
    Save-WindowScreenshot -Window $window -Name '03-advertisement.png'
    Start-Sleep -Milliseconds 3000
    Save-WindowScreenshot -Window $window -Name '04-advertisement.png'
    Start-Sleep -Milliseconds 2300
    Save-WindowScreenshot -Window $window -Name '05-complete.png'
}
finally {
    if ($uiProcessId -ne 0) {
        $uiProcess = Get-Process -Id $uiProcessId -ErrorAction SilentlyContinue
        if ($uiProcess) {
            Stop-Process -Id $uiProcess.Id -Force
            $uiProcess.WaitForExit()
        }
    }
    if (-not $process.HasExited) {
        Stop-Process -Id $process.Id -Force
        $process.WaitForExit()
    }
}

Write-Host "UI screenshots: $output"
