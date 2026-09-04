function New-ProjectZipArchive {
    param(
        [Parameter(Mandatory = $true)][string]$SourceDirectory,
        [Parameter(Mandatory = $true)][string]$DestinationPath
    )

    $sourceFullPath = [System.IO.Path]::GetFullPath($SourceDirectory)
    $destinationFullPath = [System.IO.Path]::GetFullPath($DestinationPath)
    if (-not (Test-Path -LiteralPath $sourceFullPath -PathType Container)) {
        throw "Archive source directory was not found: $sourceFullPath"
    }
    if (Test-Path -LiteralPath $destinationFullPath -PathType Leaf) {
        Remove-Item -LiteralPath $destinationFullPath -Force
    }

    Add-Type -AssemblyName System.IO.Compression.FileSystem
    [System.IO.Compression.ZipFile]::CreateFromDirectory(
        $sourceFullPath,
        $destinationFullPath,
        [System.IO.Compression.CompressionLevel]::Optimal,
        $true
    )
}
