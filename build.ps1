param(
    [ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")]
    [string]$Config = "Release",
    [ValidateSet(
        "windows-amd64",
        "windows-arm64",
        "linux-amd64",
        "linux-arm64",
        "darwin-amd64",
        "darwin-arm64"
    )]
    [string[]]$Targets = @(
        "windows-amd64",
        "windows-arm64",
        "linux-amd64",
        "linux-arm64",
        "darwin-amd64",
        "darwin-arm64"
    ),
    [string]$WslDistro = "Ubuntu",
    [switch]$SetupMissingTools,
    [switch]$Clean
)

$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $Root

$BuildRoot = Join-Path $Root "build"
$DistDir = Join-Path $Root "dist"

if ($Clean) {
    Remove-Item -Recurse -Force -ErrorAction SilentlyContinue $BuildRoot, $DistDir
}

if ($SetupMissingTools) {
    & (Join-Path $Root "scripts\setup-cross.ps1") -WslDistro $WslDistro
}

function Get-VisualStudioGenerator {
    $generators = (& cmake --help 2>&1 | Out-String)

    if ($generators -match "Visual Studio 18 2026") {
        return "Visual Studio 18 2026"
    }

    if ($generators -match "Visual Studio 17 2022") {
        return "Visual Studio 17 2022"
    }

    if ($generators -match "Visual Studio 16 2019") {
        return "Visual Studio 16 2019"
    }

    throw "No supported Visual Studio CMake generator was found."
}

function Build-WindowsTarget {
    param(
        [string]$Target,
        [string]$CMakeArch
    )

    $generator = Get-VisualStudioGenerator
    $buildDir = Join-Path $BuildRoot $Target
    $outputName = "kicad-backport-$Target.exe"
    $outputPath = Join-Path $DistDir $outputName

    cmake -S $Root -B $buildDir -G $generator -A $CMakeArch
    cmake --build $buildDir --config $Config

    $builtExe = Join-Path (Join-Path $buildDir $Config) "kicad-backport.exe"

    if (-not (Test-Path $builtExe)) {
        throw "Cannot find built executable: $builtExe"
    }

    New-Item -ItemType Directory -Force -Path $DistDir | Out-Null
    Copy-Item -LiteralPath $builtExe -Destination $outputPath -Force
    Write-Host "Built $outputPath"
}

function Skip-Target {
    param(
        [string]$Target,
        [string]$Reason
    )

    Write-Host "Skipped ${Target}: $Reason"
}

function Invoke-WslNative {
    param(
        [string[]]$Arguments,
        [switch]$Quiet
    )

    $oldErrorActionPreference = $ErrorActionPreference
    $nativePreference = Get-Variable -Name PSNativeCommandUseErrorActionPreference -ErrorAction SilentlyContinue
    $oldNativePreference = $null

    if ($nativePreference) {
        $oldNativePreference = $PSNativeCommandUseErrorActionPreference
    }

    try {
        $ErrorActionPreference = "Continue"

        if ($nativePreference) {
            $PSNativeCommandUseErrorActionPreference = $false
        }

        if ($Quiet) {
            $output = & wsl @Arguments *> $null
        }
        else {
            $output = & wsl @Arguments 2>&1
            $output | ForEach-Object { Write-Host $_ }
        }

        return $LASTEXITCODE
    }
    finally {
        $ErrorActionPreference = $oldErrorActionPreference

        if ($nativePreference) {
            $PSNativeCommandUseErrorActionPreference = $oldNativePreference
        }
    }
}

function Test-WslCommand {
    param([string]$Command)

    $code = Invoke-WslNative @("-d", $WslDistro, "--", "bash", "-lc",
                               "command -v $Command >/dev/null 2>&1") -Quiet
    return $code -eq 0
}

function Convert-ToWslPath {
    param([string]$Path)

    $resolved = Resolve-Path $Path
    $drive = $resolved.Path.Substring(0, 1).ToLowerInvariant()
    $rest = $resolved.Path.Substring(2).Replace('\', '/')
    return "/mnt/$drive$rest"
}

function Convert-ToBashSingleQuoted {
    param([string]$Value)

    return "'" + $Value.Replace("'", "'\''") + "'"
}

function Build-LinuxViaWsl {
    param([string]$Target)

    if (-not (Get-Command wsl -ErrorAction SilentlyContinue)) {
        Skip-Target $Target "WSL is not installed."
        return
    }

    if (-not (Test-WslCommand "cmake")) {
        Skip-Target $Target "WSL cmake is missing. Run .\scripts\setup-cross.ps1 or .\build.ps1 -SetupMissingTools."
        return
    }

    if ($Target -eq "linux-amd64" -and -not (Test-WslCommand "g++")) {
        Skip-Target $Target "WSL g++ is missing. Run .\scripts\setup-cross.ps1 or .\build.ps1 -SetupMissingTools."
        return
    }

    if ($Target -eq "linux-arm64" -and -not (Test-WslCommand "aarch64-linux-gnu-g++")) {
        Skip-Target $Target "WSL aarch64-linux-gnu-g++ is missing. Run .\scripts\setup-cross.ps1 or .\build.ps1 -SetupMissingTools."
        return
    }

    $rootWsl = Convert-ToWslPath $Root
    $cleanArg = if ($Clean) { "--clean" } else { "" }
    $command = "cd $(Convert-ToBashSingleQuoted $rootWsl) && CONFIG=$(Convert-ToBashSingleQuoted $Config) TARGETS=$(Convert-ToBashSingleQuoted $Target) sh ./build.sh $cleanArg"
    $code = Invoke-WslNative @("-d", $WslDistro, "--", "bash", "-lc", $command)

    if ($code -ne 0) {
        throw "WSL build failed for $Target"
    }
}

foreach ($target in $Targets) {
    switch ($target) {
        "windows-amd64" {
            Build-WindowsTarget -Target $target -CMakeArch "x64"
        }
        "windows-arm64" {
            Build-WindowsTarget -Target $target -CMakeArch "ARM64"
        }
        "linux-amd64" {
            Build-LinuxViaWsl $target
        }
        "linux-arm64" {
            Build-LinuxViaWsl $target
        }
        "darwin-amd64" {
            Skip-Target $target "Darwin binaries require macOS and the Apple SDK. Run ./build.sh on macOS."
        }
        "darwin-arm64" {
            Skip-Target $target "Darwin binaries require macOS and the Apple SDK. Run ./build.sh on macOS."
        }
        default {
            throw "Unsupported target: $target"
        }
    }
}

Write-Host "Done. Generated files are in $DistDir"
