param(
    [string]$Version = "latest",
    [string]$InstallDir = ".\tools\luatos-cli-release",
    [switch]$Force
)

$ErrorActionPreference = "Stop"
$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$installFullPath = Join-Path $repoRoot $InstallDir
$downloadDir = Join-Path $installFullPath "downloads"
$exePath = Join-Path $installFullPath "luatos-cli.exe"
$versionPath = Join-Path $installFullPath "VERSION"

if ((Test-Path -LiteralPath $exePath) -and -not $Force) {
    Write-Output "luatos-cli already installed: $exePath"
    if (Test-Path -LiteralPath $versionPath) {
        Write-Output "Version: $([System.IO.File]::ReadAllText($versionPath).Trim())"
    }
    exit 0
}

New-Item -ItemType Directory -Force -Path $downloadDir | Out-Null

$headers = @{ "User-Agent" = "AIR780EXX-ArduinoCore" }
if ($Version -eq "latest") {
    $releaseUri = "https://api.github.com/repos/wendal/luatos-cli/releases/latest"
}
else {
    $releaseUri = "https://api.github.com/repos/wendal/luatos-cli/releases/tags/$Version"
}

$release = Invoke-RestMethod -Headers $headers -Uri $releaseUri
$asset = $release.assets |
    Where-Object { $_.name -like "luatos-cli-x86_64-pc-windows-msvc.zip" } |
    Select-Object -First 1

if ($null -eq $asset) {
    throw "Windows luatos-cli release asset was not found in $($release.html_url)"
}

$zipPath = Join-Path $downloadDir $asset.name
Write-Output "Downloading $($asset.name) from $($release.html_url)"
Invoke-WebRequest -Headers $headers -Uri $asset.browser_download_url -OutFile $zipPath

$extractDir = Join-Path $installFullPath $release.tag_name
if (Test-Path -LiteralPath $extractDir) {
    Remove-Item -LiteralPath $extractDir -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $extractDir | Out-Null
Expand-Archive -LiteralPath $zipPath -DestinationPath $extractDir -Force

$releaseExe = Get-ChildItem -LiteralPath $extractDir -Recurse -Filter "luatos-cli.exe" |
    Select-Object -First 1
if ($null -eq $releaseExe) {
    throw "luatos-cli.exe was not found after extracting $zipPath"
}

Copy-Item -LiteralPath $releaseExe.FullName -Destination $exePath -Force
[System.IO.File]::WriteAllText($versionPath, "$($release.tag_name)`n$($release.html_url)`n")

Write-Output "Installed luatos-cli $($release.tag_name): $exePath"
