param([string]$PackagePath = "package")
$ErrorActionPreference = "Stop"

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vs) { throw "Visual Studio C++ tools were not found" }
$crt = Get-ChildItem "$vs\VC\Redist\MSVC\*\x64\Microsoft.VC*.CRT" -Directory |
    Sort-Object FullName -Descending | Select-Object -First 1
if (-not $crt) { throw "Redistributable x64 MSVC runtime was not found" }
Copy-Item "$($crt.FullName)\*.dll" $PackagePath

$dumpbin = Get-ChildItem "$vs\VC\Tools\MSVC\*\bin\Hostx64\x64\dumpbin.exe" |
    Sort-Object FullName -Descending | Select-Object -First 1
if (-not $dumpbin) { throw "dumpbin.exe was not found" }
foreach ($binary in Get-ChildItem $PackagePath -Recurse -File | Where-Object { $_.Extension -in ".exe", ".dll" }) {
    $dependencies = & $dumpbin.FullName /dependents $binary.FullName
    if ($LASTEXITCODE -ne 0) { throw "Could not inspect $($binary.FullName)" }
    foreach ($line in $dependencies) {
        if ($line -match '^\s*((?:VCRUNTIME|MSVCP|CONCRT)\S*\.dll)\s*$') {
            if (-not (Test-Path (Join-Path $PackagePath $Matches[1]))) {
                throw "$($binary.Name) requires unpackaged runtime $($Matches[1])"
            }
        }
    }
}
Write-Output "Bundled and verified MSVC runtime from $($crt.FullName)"
