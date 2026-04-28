param(
    [string]$MapFile = ".\runner\air780epm_runner\out\air780epm_runner_debug.map",
    [int]$MinimumEntries = 2
)

$ErrorActionPreference = "Stop"
$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$mapFullPath = Resolve-Path (Join-Path $repoRoot $MapFile)

$lines = Get-Content -LiteralPath $mapFullPath
$startIndex = -1
for ($i = 0; $i -lt $lines.Count; $i++) {
    if ($lines[$i] -match "^\.arduino_init_array") {
        $startIndex = $i
        break
    }
}

if ($startIndex -lt 0) {
    throw ".arduino_init_array section was not found in $mapFullPath"
}

$block = New-Object System.Collections.Generic.List[string]
for ($i = $startIndex; $i -lt $lines.Count; $i++) {
    if ($i -gt $startIndex -and $lines[$i] -match "^\.[A-Za-z0-9_]+") {
        break
    }
    $block.Add($lines[$i])
}

$blockText = $block -join "`n"
if ($blockText -notmatch "__arduino_init_array_start" -or $blockText -notmatch "__arduino_init_array_end") {
    throw "Arduino init-array boundary symbols were not found in $mapFullPath"
}

if ($blockText -match "libc(_nano)?\.a") {
    throw "Arduino init-array unexpectedly contains libc entries"
}

$entries = @($block | Where-Object { $_ -match "\s\.init_array\s+0x[0-9a-fA-F]+\s+0x[0-9a-fA-F]+" })
if ($entries.Count -lt $MinimumEntries) {
    throw "Arduino init-array has only $($entries.Count) entries; expected at least $MinimumEntries"
}

Write-Output "STATIC_CTORS_MAP: PASS entries=$($entries.Count)"
foreach ($entry in $entries) {
    Write-Output $entry.Trim()
}
