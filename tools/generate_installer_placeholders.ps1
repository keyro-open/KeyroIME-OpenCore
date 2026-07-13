param(
    [string]$OutputDirectory = (Join-Path $PSScriptRoot '..\installer\assets')
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

Add-Type -AssemblyName System.Drawing

$width = 640
$height = 280
$fontName = 'Yu Gothic UI'
$items = @(
    @{ File = 'cover.bmp'; Accent = '#4D7CFE'; Kicker = 'SETUP GUIDE'; Title = 'KeyroIME OpenCore'; Subtitle = 'Japanese input for Windows | Image placeholder' },
    @{ File = 'progress-1.bmp'; Accent = '#00A887'; Kicker = 'PRODUCT 01'; Title = 'Direct number and symbol input'; Subtitle = 'Fast mixed input without a second candidate selection' },
    @{ File = 'progress-2.bmp'; Accent = '#E09F3E'; Kicker = 'PRODUCT 02'; Title = 'Switch JIS / ANSI dynamically'; Subtitle = 'Keyboard layout and CapsLock controls' },
    @{ File = 'progress-3.bmp'; Accent = '#8B5CF6'; Kicker = 'AD SPACE 03'; Title = 'KeyroIME Pro'; Subtitle = 'On-device AI suggestions | Promotional placeholder' },
    @{ File = 'progress-4.bmp'; Accent = '#E45757'; Kicker = 'AD SPACE 04'; Title = 'Business Services'; Subtitle = 'Commercial licensing and support | Promotional placeholder' },
    @{ File = 'finish.bmp'; Accent = '#1A9C60'; Kicker = 'COMPLETED'; Title = 'Installation completed'; Subtitle = 'KeyroIME OpenCore is ready to use' }
)

New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null

foreach ($item in $items) {
    $bitmap = [System.Drawing.Bitmap]::new($width, $height)
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    $background = [System.Drawing.SolidBrush]::new([System.Drawing.Color]::FromArgb(246, 248, 252))
    $accent = [System.Drawing.SolidBrush]::new([System.Drawing.ColorTranslator]::FromHtml($item.Accent))
    $primary = [System.Drawing.SolidBrush]::new([System.Drawing.Color]::FromArgb(28, 35, 48))
    $secondary = [System.Drawing.SolidBrush]::new([System.Drawing.Color]::FromArgb(91, 101, 118))
    $white = [System.Drawing.SolidBrush]::new([System.Drawing.Color]::White)
    $border = [System.Drawing.Pen]::new([System.Drawing.Color]::FromArgb(210, 216, 226), 2)
    $kickerFont = [System.Drawing.Font]::new($fontName, 12, [System.Drawing.FontStyle]::Bold)
    $titleFont = [System.Drawing.Font]::new($fontName, 25, [System.Drawing.FontStyle]::Bold)
    $subtitleFont = [System.Drawing.Font]::new($fontName, 11, [System.Drawing.FontStyle]::Regular)
    $placeholderFont = [System.Drawing.Font]::new($fontName, 9, [System.Drawing.FontStyle]::Regular)

    try {
        $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
        $graphics.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::ClearTypeGridFit
        $graphics.FillRectangle($background, 0, 0, $width, $height)
        $graphics.FillRectangle($accent, 0, 0, 18, $height)
        $graphics.FillRectangle($accent, 48, 36, 132, 30)
        $graphics.DrawString($item.Kicker, $kickerFont, $white, 60, 41)
        $graphics.DrawString($item.Title, $titleFont, $primary, 48, 94)
        $graphics.DrawString($item.Subtitle, $subtitleFont, $secondary, 50, 158)
        $graphics.DrawRectangle($border, 48, 211, 540, 38)
        $graphics.DrawString('PLACEHOLDER IMAGE / REPLACE LATER', $placeholderFont, $secondary, 62, 220)

        $path = Join-Path $OutputDirectory $item.File
        $bitmap.Save($path, [System.Drawing.Imaging.ImageFormat]::Bmp)
        Write-Host "Generated $path"
    }
    finally {
        $placeholderFont.Dispose()
        $subtitleFont.Dispose()
        $titleFont.Dispose()
        $kickerFont.Dispose()
        $border.Dispose()
        $white.Dispose()
        $secondary.Dispose()
        $primary.Dispose()
        $accent.Dispose()
        $background.Dispose()
        $graphics.Dispose()
        $bitmap.Dispose()
    }
}
