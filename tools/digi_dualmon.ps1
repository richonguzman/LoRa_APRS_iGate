<#
.SYNOPSIS
    Dual serial monitor for the RP2350 iGate digipeater test (two boards on the
    868 MHz island). Reads both USB-CDC serial ports in parallel, tags each line
    with its board label + timestamp, and colour-highlights the key events:

        [digi] RF: ...     magenta   -> the digi computed & is re-TXing a packet
        RX ...,CALL*:...   green     -> a DIGIPEATED packet heard on air (has '*')
        [lora] TX ...      cyan      -> radio transmitting
        [lora] RX ...      yellow    -> a raw packet received
        [aprsis] up: ...   blue      -> gated to APRS-IS
        [hb] ...           dimmed    -> heartbeat noise

.EXAMPLE
    # list the available COM ports first
    .\tools\digi_dualmon.ps1 -List

.EXAMPLE
    # DIGI board on COM5, SOURCE board on COM6
    .\tools\digi_dualmon.ps1 -PortA COM5 -PortB COM6

.NOTES
    Opening the port pulses DTR and reboots the board (arduino-pico USB CDC), so
    the SOURCE board RF-beacons immediately on connect -> instant test stimulus.
    Ctrl+C to stop; ports are closed cleanly on exit.
#>
[CmdletBinding()]
param(
    [string]$PortA,
    [string]$PortB,
    [int]$Baud      = 115200,
    [string]$LabelA = "DIGI",
    [string]$LabelB = "SRC ",
    [switch]$List
)

function Show-Ports {
    Write-Host "Available COM ports:" -ForegroundColor Cyan
    $names = [System.IO.Ports.SerialPort]::GetPortNames() | Sort-Object
    if (-not $names) { Write-Host "  (none found)" -ForegroundColor DarkGray }
    else { $names | ForEach-Object { Write-Host "  $_" } }
}

if ($List) { Show-Ports; return }
if (-not $PortA -or -not $PortB) {
    Write-Host "Need both ports. Example: .\tools\digi_dualmon.ps1 -PortA COM5 -PortB COM6`n" -ForegroundColor Yellow
    Show-Ports
    return
}

function Open-Mon {
    param([string]$Name, [string]$Port, [int]$Baud)
    $sp = New-Object System.IO.Ports.SerialPort $Port, $Baud, ([System.IO.Ports.Parity]::None), 8, ([System.IO.Ports.StopBits]::One)
    $sp.ReadTimeout  = 50
    $sp.NewLine      = "`n"
    $sp.DtrEnable    = $true
    $sp.RtsEnable    = $true
    try { $sp.Open() }
    catch { Write-Host "FAILED to open $Port : $($_.Exception.Message)" -ForegroundColor Red; throw }
    Write-Host "Opened $Port as [$Name] @ $Baud" -ForegroundColor Green
    [pscustomobject]@{ Name = $Name; Port = $sp; Buf = "" }
}

# Pick a colour for a line based on its content (most-significant event wins).
function Get-LineColor {
    param([string]$Line)
    if ($Line -match '\[digi\]')                         { return 'Magenta' }   # digi acting
    # a digipeated packet heard: "...>PATH,SOMECALL*:payload" — '*' before the ':'
    if ($Line -match 'RX:.*>[^:]*\*[^:]*:')              { return 'Green'   }
    if ($Line -match '\[lora\] TX')                      { return 'Cyan'    }
    if ($Line -match '\[lora\] RX|RX:')                  { return 'Yellow'  }
    if ($Line -match '\[aprsis\] up:')                   { return 'Blue'    }
    if ($Line -match '\[hb\]|\[ETH\]|DHCP')              { return 'DarkGray'}
    return 'Gray'
}

$monitors = @()
try {
    $monitors += Open-Mon -Name $LabelA -Port $PortA -Baud $Baud
    $monitors += Open-Mon -Name $LabelB -Port $PortB -Baud $Baud
    Write-Host "`n--- monitoring (Ctrl+C to stop) ---`n" -ForegroundColor Cyan

    while ($true) {
        foreach ($m in $monitors) {
            $chunk = ""
            try { $chunk = $m.Port.ReadExisting() } catch { }
            if ($chunk) {
                $m.Buf += $chunk
                while (($nl = $m.Buf.IndexOf("`n")) -ge 0) {
                    $line = $m.Buf.Substring(0, $nl).TrimEnd("`r")
                    $m.Buf = $m.Buf.Substring($nl + 1)
                    if ($line.Length -eq 0) { continue }
                    $ts    = (Get-Date).ToString("HH:mm:ss.fff")
                    $color = Get-LineColor $line
                    Write-Host ("{0} [{1}] " -f $ts, $m.Name) -ForegroundColor DarkGray -NoNewline
                    Write-Host $line -ForegroundColor $color
                }
            }
        }
        Start-Sleep -Milliseconds 20
    }
}
finally {
    foreach ($m in $monitors) {
        if ($m.Port -and $m.Port.IsOpen) { try { $m.Port.Close() } catch { } }
    }
    Write-Host "`n--- ports closed ---" -ForegroundColor Cyan
}
