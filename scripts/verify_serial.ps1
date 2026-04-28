param(
    [Parameter(Mandatory = $true)]
    [string]$ComPort,
    [int]$BaudRate = 921600,
    [int]$Duration = 15,
    [string[]]$PassRegex = @("\+ARDUINO: AIR780EPM,READY", "\+ARDUINO: CTOR,(PASS|SKIP)"),
    [switch]$RequirePass
)

$ErrorActionPreference = "Stop"

$deadline = (Get-Date).AddSeconds($Duration)
$matched = @{}
foreach ($regex in $PassRegex) {
    $matched[$regex] = $false
}
$port = [System.IO.Ports.SerialPort]::new($ComPort, $BaudRate)
$port.ReadTimeout = 500
$port.NewLine = "`n"

try {
    $port.Open()
    while ((Get-Date) -lt $deadline) {
        try {
            $line = $port.ReadLine()
            Write-Output $line.TrimEnd("`r", "`n")
            foreach ($regex in $PassRegex) {
                if ($line -match $regex) {
                    $matched[$regex] = $true
                }
            }
        }
        catch [System.TimeoutException] {
        }
    }
}
finally {
    if ($port.IsOpen) {
        $port.Close()
    }
    $port.Dispose()
}

$missing = @($PassRegex | Where-Object { -not $matched[$_] })
$allMatched = ($missing.Count -eq 0)

if ($RequirePass -and -not $allMatched) {
    throw "Pass regex was not observed on ${ComPort}: $($missing -join ', ')"
}

if ($allMatched) {
    Write-Output "SERIAL_VERIFY: PASS"
}
else {
    Write-Output "SERIAL_VERIFY: NO_MATCH $($missing -join ', ')"
}
