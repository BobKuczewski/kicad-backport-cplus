param(
    [ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")]
    [string]$Config = "Release",
    [string]$Compiler = "auto",
    [ValidateSet("auto", "on", "off")]
    [string]$StaticRuntime = "auto",
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
$SourceList = Join-Path $Root "kicad_backport_sources.txt"
$Target = "windows-amd64"
$OutputPath = Join-Path $DistDir "kicad-backport-$Target.exe"

function Get-WindowsGxx {
    if ($Compiler -ne "auto") {
        return $Compiler
    }

    $native = Get-Command "g++" -ErrorAction SilentlyContinue
    if ($native) {
        return $native.Source
    }

    $msysGxx = "C:\msys64\ucrt64\bin\g++.exe"
    if (Test-Path $msysGxx) {
        return $msysGxx
    }

    throw "g++ is required. Install MSYS2 UCRT64 GCC or pass -Compiler <path-to-g++>."
}

function Add-CompilerPath {
    param([string]$CompilerPath)

    $dir = Split-Path -Parent $CompilerPath
    if ($dir -and (Test-Path $dir)) {
        $env:PATH = "$dir;$env:PATH"
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
    param([string]$CompilerPath)

    $tempDir = Join-Path ([System.IO.Path]::GetTempPath()) ("kicad-backport-cxx-std-" + [System.Guid]::NewGuid().ToString("N"))
    New-Item -ItemType Directory -Force -Path $tempDir | Out-Null
    $source = Join-Path $tempDir "probe.cpp"
    $object = Join-Path $tempDir "probe.o"
    @"
#include <memory>
#include <string>
#include <vector>
int main() { std::unique_ptr<int> p(new int(1)); std::vector<std::string> v; v.push_back("x"); return *p == 1 && !v.empty() ? 0 : 1; }
"@ | Set-Content -LiteralPath $source -Encoding ASCII

    try {
        & $CompilerPath -std=c++17 -c $source -o $object *> $null
        if ($LASTEXITCODE -eq 0) {
            return "-std=c++17"
        }

        & $CompilerPath -std=c++1z -c $source -o $object *> $null
        if ($LASTEXITCODE -eq 0) {
            return "-std=c++1z"
        }
    }
    finally {
        Remove-Item -Recurse -Force -ErrorAction SilentlyContinue $tempDir
    }

    throw "$CompilerPath cannot compile the required C++ mode probe."
}

function Test-CompileFlag {
    param(
        [string]$CompilerPath,
        [string]$StandardFlag,
        [string]$Flag
    )

    $tempDir = Join-Path ([System.IO.Path]::GetTempPath()) ("kicad-backport-cxx-flag-" + [System.Guid]::NewGuid().ToString("N"))
    New-Item -ItemType Directory -Force -Path $tempDir | Out-Null
    $source = Join-Path $tempDir "probe.cpp"
    $object = Join-Path $tempDir "probe.o"
    "int main() { return 0; }" | Set-Content -LiteralPath $source -Encoding ASCII

    try {
        & $CompilerPath $StandardFlag $Flag -Werror -c $source -o $object *> $null
        return $LASTEXITCODE -eq 0
    }
    finally {
        Remove-Item -Recurse -Force -ErrorAction SilentlyContinue $tempDir
    }
}

function Test-LinkFlag {
    param(
        [string]$CompilerPath,
        [string]$StandardFlag,
        [string]$Flag
    )

    $tempDir = Join-Path ([System.IO.Path]::GetTempPath()) ("kicad-backport-ld-flag-" + [System.Guid]::NewGuid().ToString("N"))
    New-Item -ItemType Directory -Force -Path $tempDir | Out-Null
    $source = Join-Path $tempDir "probe.cpp"
    $object = Join-Path $tempDir "probe.o"
    $output = Join-Path $tempDir "probe.exe"
    "int main() { return 0; }" | Set-Content -LiteralPath $source -Encoding ASCII

    try {
        & $CompilerPath $StandardFlag -c $source -o $object *> $null
        if ($LASTEXITCODE -ne 0) {
            return $false
        }

        & $CompilerPath $object $Flag -o $output *> $null
        return $LASTEXITCODE -eq 0
    }
    finally {
        Remove-Item -Recurse -Force -ErrorAction SilentlyContinue $tempDir
    }
}

function Get-SizeCompileFlags {
    param(
        [string]$CompilerPath,
        [string]$StandardFlag
    )

    if ($Config -eq "Debug") {
        return @()
    }

    $flags = @()
    foreach ($flag in @("-ffunction-sections", "-fdata-sections")) {
        if (Test-CompileFlag $CompilerPath $StandardFlag $flag) {
            $flags += $flag
        }
    }

    return $flags
}

function Get-SizeLinkFlags {
    param(
        [string]$CompilerPath,
        [string]$StandardFlag
    )

    if ($Config -eq "Debug") {
        return @()
    }

    $flags = @()
    if (Test-LinkFlag $CompilerPath $StandardFlag "-Wl,--gc-sections") {
        $flags += "-Wl,--gc-sections"
    }

    if (($Config -eq "Release" -or $Config -eq "MinSizeRel") -and
        (Test-LinkFlag $CompilerPath $StandardFlag "-Wl,-s")) {
        $flags += "-Wl,-s"
    }

    return $flags
}

if (-not (Test-Path $SourceList)) {
    throw "Missing source list: $SourceList"
}

if ($Clean) {
    Remove-Item -Recurse -Force -ErrorAction SilentlyContinue (Join-Path $BuildRoot "$Target-direct"), $OutputPath
}

$gxx = Get-WindowsGxx
Add-CompilerPath $gxx

$buildDir = Join-Path $BuildRoot "$Target-direct"
$objectDir = Join-Path $buildDir "obj"
New-Item -ItemType Directory -Force -Path $objectDir, $DistDir | Out-Null

$stdFlag = Get-CxxStandardFlag $gxx
$sizeCxxFlags = Get-SizeCompileFlags $gxx $stdFlag
$sizeLdFlags = Get-SizeLinkFlags $gxx $stdFlag
$commonFlags = @(
    $stdFlag,
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
if ($StaticRuntime -ne "off") {
    $staticFlags = @("-static", "-static-libstdc++", "-static-libgcc")
}

Write-Host "Compiler: $(& $gxx --version | Select-Object -First 1)"
Write-Host "Target: $Target"

$objects = @()
foreach ($line in Get-Content -LiteralPath $SourceList) {
    $source = $line.Trim()
    if ($source -eq "" -or $source.StartsWith("#")) {
        continue
    }

    $object = Join-Path $objectDir (($source -replace "/", "\") -replace "\.cpp$", ".o")
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $object) | Out-Null

    $sourcePath = Join-Path $Root ($source -replace "/", "\")
    & $gxx @commonFlags @configFlags @sizeCxxFlags @extraCxxFlags -c $sourcePath -o $object
    if ($LASTEXITCODE -ne 0) {
        throw "Compile failed: $source"
    }

    $objects += $object
}

function Invoke-LinkAttempt {
    param([string[]]$Flags)

    & $gxx @objects @extraLdFlags @sizeLdFlags @Flags -o $OutputPath
    return $LASTEXITCODE -eq 0
}

$linked = $false
if ($StaticRuntime -eq "on") {
    $linked = Invoke-LinkAttempt $staticFlags
}
elseif ($StaticRuntime -eq "off") {
    $linked = Invoke-LinkAttempt @()
}
else {
    $linked = (Invoke-LinkAttempt $staticFlags) -or
              (Invoke-LinkAttempt @())
}

if (-not $linked) {
    throw "Direct g++ link failed."
}

Write-Host "Built $OutputPath"
Write-Host "Done. Generated files are in $DistDir"
