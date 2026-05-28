param(
    [switch]$CheckOnly,
    [string]$WslDistro = "Ubuntu",
    [switch]$SkipWsl
)

$ErrorActionPreference = "Stop"
$DoInstall = -not $CheckOnly

function Write-Status {
    param(
        [string]$Name,
        [bool]$Ok,
        [string]$Detail
    )

    $mark = if ($Ok) { "OK" } else { "MISSING" }
    Write-Host ("[{0}] {1}: {2}" -f $mark, $Name, $Detail)
}

function Has-Command {
    param([string]$Name)
    return $null -ne (Get-Command $Name -ErrorAction SilentlyContinue)
}

function Test-VisualStudioCmakeGenerator {
    if (-not (Has-Command "cmake")) {
        return $false
    }

    $generators = (& cmake -G 2>&1 | Out-String)
    return $generators -match "Visual Studio (18 2026|17 2022|16 2019)"
}

function Install-WithWinget {
    param(
        [string]$Id,
        [string]$Override = ""
    )

    if (-not (Has-Command "winget")) {
        Write-Host "winget is not available; install $Id manually."
        return
    }

    $args = @(
        "install",
        "--id", $Id,
        "-e",
        "--source", "winget",
        "--accept-package-agreements",
        "--accept-source-agreements"
    )

    if ($Override.Trim() -ne "") {
        $args += @("--override", $Override)
    }

    & winget @args
}

function Get-WslDistros {
    if (-not (Has-Command "wsl")) {
        return @()
    }

    $raw = (& wsl -l -q 2>$null | Out-String).Replace("`0", "")
    return @($raw -split "`r?`n" | ForEach-Object { $_.Trim() } | Where-Object { $_ -ne "" })
}

function Ensure-WslDistro {
    if (-not (Has-Command "wsl")) {
        Write-Host "WSL command is missing. Enable WSL in Windows features or install it from Microsoft Store."
        return $false
    }

    $distros = Get-WslDistros

    if ($distros -contains $WslDistro) {
        return $true
    }

    if (-not $DoInstall) {
        return $false
    }

    Write-Host "Installing WSL distro: $WslDistro"
    & wsl --install -d $WslDistro --no-launch
    Write-Host "If Windows asks for a reboot or first-time Linux user setup, complete that and rerun this script."

    $distros = Get-WslDistros
    return $distros -contains $WslDistro
}

function Test-WslCommand {
    param([string]$Command)

    & wsl -d $WslDistro -- bash -lc "command -v $Command >/dev/null 2>&1" *> $null
    return $LASTEXITCODE -eq 0
}

function Invoke-Wsl {
    param([string]$Command)

    & wsl -d $WslDistro -- bash -lc $Command
}

Write-Host "KiCad Backport C++ minimal cross-build environment setup"
Write-Host "Mode: $(if ($DoInstall) { 'setup missing tools' } else { 'check only' })"
Write-Host ""

$cmakeOk = Has-Command "cmake"
Write-Status "CMake" $cmakeOk ($(if ($cmakeOk) { (cmake --version | Select-Object -First 1) } else { "Required for Windows builds." }))

if ($DoInstall -and -not $cmakeOk) {
    Install-WithWinget "Kitware.CMake"
    $cmakeOk = Has-Command "cmake"
}

$vsOk = Test-VisualStudioCmakeGenerator
Write-Status "Visual Studio C++ CMake generator" $vsOk "Required for windows-amd64/windows-arm64."

if ($DoInstall -and -not $vsOk) {
    Install-WithWinget "Microsoft.VisualStudio.2022.BuildTools" `
        "--quiet --wait --norestart --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
    $vsOk = Test-VisualStudioCmakeGenerator
}

if (-not $SkipWsl) {
    $wslOk = Has-Command "wsl"
    Write-Status "WSL command" $wslOk "Required on Windows for local Linux target builds."

    if ($DoInstall -and -not $wslOk) {
        Write-Host "Installing WSL base feature."
        & wsl --install --no-distribution
        Write-Host "If Windows asks for a reboot, reboot and rerun this script."
        $wslOk = Has-Command "wsl"
    }

    $distroOk = $false

    if ($wslOk) {
        $distroOk = Ensure-WslDistro
    }

    Write-Status "WSL distro $WslDistro" $distroOk "Required for linux-amd64/linux-arm64 build environment."

    if ($distroOk) {
        $wslCmakeOk = Test-WslCommand "cmake"
        $wslGxxOk = Test-WslCommand "g++"
        $wslArmOk = Test-WslCommand "aarch64-linux-gnu-g++"

        Write-Status "WSL cmake" $wslCmakeOk "Required for Linux CMake builds."
        Write-Status "WSL g++" $wslGxxOk "Required for linux-amd64 native builds."
        Write-Status "WSL aarch64-linux-gnu-g++" $wslArmOk "Required for linux-arm64 cross-builds."

        if ($DoInstall -and (-not $wslCmakeOk -or -not $wslGxxOk -or -not $wslArmOk)) {
            Write-Host "Installing minimal WSL packages for Linux amd64/arm64 builds."
            Invoke-Wsl "sudo env DEBIAN_FRONTEND=noninteractive apt-get update && sudo env DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends cmake g++ ninja-build gcc-aarch64-linux-gnu g++-aarch64-linux-gnu"
        }
    }
}

Write-Host ""
Write-Host "Summary:"
Write-Host "- Windows amd64/arm64: Visual Studio C++ Build Tools + CMake."
Write-Host "- Linux amd64: WSL $WslDistro + cmake + g++."
Write-Host "- Linux arm64: WSL $WslDistro + aarch64-linux-gnu-g++ cross toolchain."
Write-Host "- Darwin amd64/arm64: must be built on macOS with Apple Command Line Tools."
Write-Host ""
Write-Host "Next:"
Write-Host "  .\build.ps1"
