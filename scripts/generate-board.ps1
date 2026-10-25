# Regenerate the two self-contained board SVGs. No font is required by the application.
# Coordinates derive from board-lines.svg; river paths are from OXDF-editor/assets/board.svg.
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName PresentationCore
Add-Type -AssemblyName WindowsBase
$repoPath = Split-Path -Parent $PSScriptRoot
$invariant = [Globalization.CultureInfo]::InvariantCulture
function Number([double]$value) { $value.ToString('0.######', $invariant) }
function CenteredPath([string]$data, [double]$x, [double]$y, [double]$scale = 1) {
    $bounds = [Windows.Media.Geometry]::Parse($data).Bounds
    $dx = $x - $scale * ($bounds.X + $bounds.Width / 2)
    $dy = $y - $scale * ($bounds.Y + $bounds.Height / 2)
    '<path transform="matrix({0} 0 0 {0} {1} {2})" d="{3}"/>' -f (Number $scale), (Number $dx), (Number $dy), $data
}
function Coordinate([string]$text, [double]$x, [double]$y) {
    $family = if ($text -match '^[1-9]$') { 'Segoe UI' } else { 'Microsoft YaHei' }
    $formatted = [Windows.Media.FormattedText]::new($text, $invariant,
        [Windows.FlowDirection]::LeftToRight, [Windows.Media.Typeface]::new($family),
        14, [Windows.Media.Brushes]::Black, 1.0)
    $geometry = $formatted.BuildGeometry([Windows.Point]::new(0, 0)).GetOutlinedPathGeometry()
    $data = $geometry.ToString($invariant) -replace '^F[01]', ''
    $bounds = $geometry.Bounds
    $scale = [Math]::Min(1.0, [Math]::Min(12 / $bounds.Height, 16 / $bounds.Width))
    if ($bounds.Height * $scale -gt 12.0001 -or $bounds.Width * $scale -gt 16.0001) { throw 'Invalid coordinate bounds' }
    CenteredPath $data $x $y $scale
}
[xml]$lines = [IO.File]::ReadAllText((Join-Path $repoPath 'assets/board-lines.svg'))
[xml]$river = [IO.File]::ReadAllText((Join-Path $repoPath 'assets/board-river.svg'))
$grid = $lines.DocumentElement.InnerXml
$riverPaths = @($river.DocumentElement.ChildNodes | Where-Object LocalName -eq 'path')
# 45px frame * 110% = 49.5px. Rows at y=49/499; labels at y=12/536.
# 12px maximum label height gives 6.25px clearance to the enlarged frame.
$clearance = 49 - 45 * 1.10 / 2 - (12 + 12 / 2)
if ($clearance -lt 6) { throw 'Insufficient coordinate clearance' }
$chinese = '九八七六五四三二一'
foreach ($flipped in @($false, $true)) {
    $parts = [Collections.Generic.List[string]]::new()
    $parts.Add('<svg xmlns="http://www.w3.org/2000/svg" width="450" height="548" viewBox="0 0 450 548">')
    $parts.Add('<title>Plain xiangqi board with outlined river and file coordinates</title>')
    $parts.Add('<desc>50px pitch; intersections x=25+50f, y=49+50r; 45px piece frames at 110%; coordinate clearance 6.25px.</desc>')
    $parts.Add('<g transform="translate(0 24)">' + $grid + '</g>')
    $parts.Add('<g id="river" fill="#636e7d">')
    for ($i = 0; $i -lt 2; $i++) {
        $data = $riverPaths[$i].GetAttribute('d')
        $bounds = [Windows.Media.Geometry]::Parse($data).Bounds
        $parts.Add((CenteredPath $data (125 + 200 * $i) 274 (28 / $bounds.Height)))
    }
    $parts.Add('</g><g id="coordinates" fill="#636e7d">')
    for ($file = 0; $file -lt 9; $file++) {
        $top = if ($flipped) { [string](9 - $file) } else { [string]($file + 1) }
        $bottom = [string]$chinese[$(if ($flipped) { 8 - $file } else { $file })]
        $parts.Add((Coordinate $top (25 + 50 * $file) 12))
        $parts.Add((Coordinate $bottom (25 + 50 * $file) 536))
    }
    $parts.Add('</g></svg>')
    $name = if ($flipped) { 'board-grid-flipped.svg' } else { 'board-grid.svg' }
    [IO.File]::WriteAllText((Join-Path $repoPath "assets/$name"),
        ($parts -join [string][char]10) + [char]10, [Text.UTF8Encoding]::new($false))
}
Write-Output 'Generated 450x548 board SVGs; river centres (125,274)/(325,274); coordinate clearance 6.25px.'
