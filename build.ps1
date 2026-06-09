param(
    [ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")]
    [string]$Config = "Release",
    [ValidateSet(
        "windows-amd64",
        "windows-arm64",
        "linux-amd64",
        "linux-arm64",
        "linux-armhf"
    )]
    [string[]]$Targets = @(
        "windows-amd64",
        "windows-arm64",
        "linux-amd64",
        "linux-arm64",
        "linux-armhf"
    ),
    [string]$WslDistro = "Ubuntu",
    [ValidateSet("auto", "on", "off")]
    [string]$StaticRuntime = "auto",
    [int]$Jobs = 0,
    [switch]$SetupMissingTools,
    [switch]$Clean
)

$ErrorActionPreference = "Stop"
if (Get-Variable -Name PSNativeCommandUseErrorActionPreference -ErrorAction SilentlyContinue) {
    $PSNativeCommandUseErrorActionPreference = $false
}

$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $Root

$BuildRoot = Join-Path $Root "build"
$DistDir = Join-Path $Root "dist"
$SourceList = Join-Path $Root "cmake\kicad_backport_sources.txt"

if ($Clean) {
    Remove-Item -Recurse -Force -ErrorAction SilentlyContinue $BuildRoot, $DistDir
}

function Skip-Target {
    param(
        [string]$Target,
        [string]$Reason
    )

    Write-Host "Skipped ${Target}: $Reason"
}

function Get-VisualStudioGenerator {
    if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
        return $null
    }

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

    return $null
}

function Get-WindowsGxx {
    $native = Get-Command "g++" -ErrorAction SilentlyContinue
    if ($native) {
        return $native.Source
    }

    $msysGxx = "C:\msys64\ucrt64\bin\g++.exe"
    if (Test-Path $msysGxx) {
        return $msysGxx
    }

    return $null
}

function Install-Msys2Gcc {
    if (-not (Get-Command "winget" -ErrorAction SilentlyContinue)) {
        Write-Host "winget is not available. Install Visual Studio Build Tools or MSYS2 manually."
        return
    }

    if (-not (Test-Path "C:\msys64\usr\bin\bash.exe")) {
        & winget install --id MSYS2.MSYS2 -e --source winget --accept-package-agreements --accept-source-agreements
    }

    $bash = "C:\msys64\usr\bin\bash.exe"
    if (Test-Path $bash) {
        & $bash -lc "pacman --noconfirm -Sy --needed mingw-w64-ucrt-x86_64-gcc"
    }
}

function Get-ConfigFlags {
    switch ($Config) {
        "Debug" { return @("-O0", "-g") }
        "Release" { return @("-O3", "-DNDEBUG") }
        "RelWithDebInfo" { return @("-O2", "-g", "-DNDEBUG") }
        "MinSizeRel" { return @("-Os", "-DNDEBUG") }
    }
}

function Split-EnvFlags {
    param([string]$Value)

    if ([string]::IsNullOrWhiteSpace($Value)) {
        return @()
    }

    return $Value -split "\s+"
}

function Get-CxxStandardFlag {
    param([string]$Compiler)

    $tempDir = Join-Path ([System.IO.Path]::GetTempPath()) ("kicad-backport-cxx-std-" + [System.Guid]::NewGuid().ToString("N"))
    New-Item -ItemType Directory -Force -Path $tempDir | Out-Null
    $source = Join-Path $tempDir "probe.cpp"
    $exe = Join-Path $tempDir "probe.exe"
    Set-Content -LiteralPath $source -Encoding ASCII -Value "int main() { return 0; }"

    try {
        & $Compiler -std=c++17 $source -o $exe *> $null
        if ($LASTEXITCODE -eq 0) {
            return "-std=c++17"
        }

        & $Compiler -std=c++1z $source -o $exe *> $null
        if ($LASTEXITCODE -eq 0) {
            return "-std=c++1z"
        }
    }
    finally {
        Remove-Item -Recurse -Force -ErrorAction SilentlyContinue $tempDir
    }

    throw "$Compiler does not accept -std=c++17 or -std=c++1z."
}

function Invoke-DirectGxxBuild {
    param(
        [string]$Compiler,
        [string]$Target,
        [string]$OutputPath
    )

    if (-not (Test-Path $SourceList)) {
        throw "Missing source list: $SourceList"
    }

    if (-not (Get-Command $Compiler -ErrorAction SilentlyContinue)) {
        throw "$Compiler is required for direct native builds."
    }

    $buildDir = Join-Path $BuildRoot "$Target-direct"
    $objectDir = Join-Path $buildDir "obj"

    if ($Clean) {
        Remove-Item -Recurse -Force -ErrorAction SilentlyContinue $buildDir, $OutputPath
    }

    New-Item -ItemType Directory -Force -Path $objectDir | Out-Null
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $OutputPath) | Out-Null

    $commonFlags = @(
        (Get-CxxStandardFlag $Compiler),
        "-Wall",
        "-Wextra",
        "-Wpedantic",
        "-I$(Join-Path $Root 'include')",
        "-I$(Join-Path $Root 'src')"
    )
    $configFlags = Get-ConfigFlags
    $extraCxxFlags = Split-EnvFlags $env:CXXFLAGS
    $extraLdFlags = Split-EnvFlags $env:LDFLAGS

    $staticFlags = @()
    if ($Target -like "windows-*") {
        $staticFlags = @("-static", "-static-libstdc++", "-static-libgcc")
    }

    Write-Host "Compiler: $(& $Compiler --version | Select-Object -First 1)"
    Write-Host "Target: $Target"
    $dumpMachine = & $Compiler -dumpmachine 2>$null
    if ($LASTEXITCODE -eq 0 -and $dumpMachine) {
        Write-Host "Compiler target: $dumpMachine"
    }
    Write-Host "Static runtime: $StaticRuntime"

    $objects = @()
    foreach ($line in Get-Content -LiteralPath $SourceList) {
        $source = $line.Trim()
        if ($source -eq "" -or $source.StartsWith("#")) {
            continue
        }

        $object = Join-Path $objectDir (($source -replace "/", "\") -replace "\.cpp$", ".o")
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $object) | Out-Null

        $sourcePath = Join-Path $Root ($source -replace "/", "\")
        & $Compiler @commonFlags @configFlags @extraCxxFlags -c $sourcePath -o $object
        if ($LASTEXITCODE -ne 0) {
            throw "Compile failed: $source"
        }

        $objects += $object
    }

    function Invoke-LinkAttempt {
        param(
            [string]$Description,
            [string[]]$Flags
        )

        Write-Host "Linking: $Description"
        & $Compiler @objects @extraLdFlags @Flags -o $OutputPath
        return $LASTEXITCODE -eq 0
    }

    $linked = $false
    if ($StaticRuntime -eq "on") {
        $linked = Invoke-LinkAttempt "static libstdc++/libgcc" $staticFlags
        if (-not $linked) {
            $linked = Invoke-LinkAttempt "static libstdc++/libgcc + stdc++fs" ($staticFlags + @("-lstdc++fs"))
        }
    }
    elseif ($StaticRuntime -eq "off") {
        $linked = Invoke-LinkAttempt "dynamic runtime" @()
        if (-not $linked) {
            $linked = Invoke-LinkAttempt "dynamic runtime + stdc++fs" @("-lstdc++fs")
        }
    }
    else {
        $linked = Invoke-LinkAttempt "static libstdc++/libgcc" $staticFlags
        if (-not $linked) {
            $linked = Invoke-LinkAttempt "static libstdc++/libgcc + stdc++fs" ($staticFlags + @("-lstdc++fs"))
        }
        if (-not $linked) {
            $linked = Invoke-LinkAttempt "dynamic runtime" @()
        }
        if (-not $linked) {
            $linked = Invoke-LinkAttempt "dynamic runtime + stdc++fs" @("-lstdc++fs")
        }
    }

    if (-not $linked) {
        throw "Direct g++ link failed."
    }

    Write-Host "Built $OutputPath"
}

function Build-WindowsTarget {
    param(
        [string]$Target,
        [string]$CMakeArch
    )

    $buildDir = Join-Path $BuildRoot $Target
    $outputPath = Join-Path $DistDir "kicad-backport-$Target.exe"
    $generator = Get-VisualStudioGenerator

    if ($null -eq $generator) {
        if ($SetupMissingTools) {
            Install-Msys2Gcc
        }

        $gxx = Get-WindowsGxx
        if ($Target -eq "windows-amd64" -and $gxx) {
            Invoke-DirectGxxBuild -Compiler $gxx -Target $Target -OutputPath $outputPath
            return
        }

        throw "No supported Visual Studio CMake generator was found. Install Visual Studio Build Tools + CMake, or install MinGW/MSYS2 g++ for windows-amd64 native builds."
    }

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

function Invoke-WslNative {
    param(
        [string[]]$Arguments,
        [switch]$Quiet
    )

    $oldErrorActionPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = "Continue"

        if ($Quiet) {
            & wsl @Arguments *> $null
        }
        else {
            & wsl @Arguments 2>&1 | ForEach-Object { Write-Host $_ }
        }

        return $LASTEXITCODE
    }
    finally {
        $ErrorActionPreference = $oldErrorActionPreference
    }
}

function Test-WslCommand {
    param([string]$Command)

    $code = Invoke-WslNative @("-d", $WslDistro, "--", "bash", "-lc",
                               "command -v $Command >/dev/null 2>&1") -Quiet
    return $code -eq 0
}

function Ensure-WslToolchain {
    param([string]$Target)

    if (-not (Get-Command wsl -ErrorAction SilentlyContinue)) {
        return $false
    }

    $packages = @("g++", "cmake", "ninja-build")
    switch ($Target) {
        "linux-arm64" { $packages += @("gcc-aarch64-linux-gnu", "g++-aarch64-linux-gnu") }
        "linux-armhf" { $packages += @("gcc-arm-linux-gnueabihf", "g++-arm-linux-gnueabihf") }
    }

    if ($SetupMissingTools) {
        $packageText = ($packages | ForEach-Object { Convert-ToBashSingleQuoted $_ }) -join " "
        $install = "if command -v apt-get >/dev/null 2>&1; then sudo env DEBIAN_FRONTEND=noninteractive apt-get update && sudo env DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends $packageText; fi"
        $code = Invoke-WslNative @("-d", $WslDistro, "--", "bash", "-lc", $install)
        if ($code -ne 0) {
            throw "WSL toolchain setup failed for $Target."
        }
    }

    switch ($Target) {
        "linux-amd64" { return Test-WslCommand "g++" }
        "linux-arm64" { return Test-WslCommand "aarch64-linux-gnu-g++" }
        "linux-armhf" { return Test-WslCommand "arm-linux-gnueabihf-g++" }
    }

    return $false
}

function Get-WslCompiler {
    param([string]$Target)

    switch ($Target) {
        "linux-amd64" { return "g++" }
        "linux-arm64" { return "aarch64-linux-gnu-g++" }
        "linux-armhf" { return "arm-linux-gnueabihf-g++" }
    }
}

function Build-LinuxViaWsl {
    param([string]$Target)

    if (-not (Ensure-WslToolchain $Target)) {
        Skip-Target $Target "WSL compiler is missing. Rerun with .\build.ps1 -SetupMissingTools, or install the target compiler in WSL."
        return
    }

    $rootWsl = Convert-ToWslPath $Root
    $compiler = Get-WslCompiler $Target
    $cleanArg = if ($Clean) { " --clean" } else { "" }
    $jobsArg = if ($Jobs -gt 0) { " --jobs $Jobs" } else { "" }
    $command = "cd $(Convert-ToBashSingleQuoted $rootWsl) && " +
        "KICAD_BACKPORT_ALLOW_CROSS=1 " +
        "CXX=$(Convert-ToBashSingleQuoted $compiler) " +
        "STATIC_RUNTIME=$(Convert-ToBashSingleQuoted $StaticRuntime) " +
        "sh ./build.sh$cleanArg --config $(Convert-ToBashSingleQuoted $Config) " +
        "--target $(Convert-ToBashSingleQuoted $Target)$jobsArg"

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
        "linux-armhf" {
            Build-LinuxViaWsl $target
        }
        default {
            throw "Unsupported target: $target"
        }
    }
}

Write-Host "Done. Generated files are in $DistDir"
