#
#   SPDX-FileCopyrightText: 2026 Julian Amann <dev@vertexwahn.de>
#   SPDX-License-Identifier: Apache-2.0
#

<#
.SYNOPSIS
    Reproduces stale _virtual_includes headers after a dependency upgrade (Windows).

.DESCRIPTION
    Builds //:main with mylib 1.0, switches MODULE.bazel to mylib 2.0 (same
    repository, same target labels) and builds again in the same output base.
    MODULE.bazel is restored at the end.

    Exit code 1: the bug was reproduced (the second build failed).
    Exit code 0: the second build succeeded.

.PARAMETER OutputBase
    Bazel output base to use. Defaults to a new, empty directory that is removed
    afterwards, so the result does not depend on earlier builds.

.PARAMETER Prune
    Delete bazel-out/*/bin/_virtual_includes between the two builds (workaround).
#>
param(
    [string]$OutputBase,
    [switch]$Prune
)

# Not 'Stop': Windows PowerShell 5.1 would turn Bazel's progress output on stderr
# into terminating errors.
Set-Location -LiteralPath $PSScriptRoot

$ownOutputBase = -not $OutputBase
if ($ownOutputBase) {
    $OutputBase = Join-Path ([IO.Path]::GetTempPath()) ('stale_virtual_includes_' + [Guid]::NewGuid().ToString('N').Substring(0, 8))
}
$OutputBase = $OutputBase -replace '\\', '/'
$moduleFile = Join-Path $PSScriptRoot 'MODULE.bazel'
$original = [IO.File]::ReadAllText($moduleFile)

function Use-MyLib([string]$Version) {
    $text = [regex]::Replace($original, 'path = "mylib-\d+\.\d+"', "path = `"mylib-$Version`"")
    [IO.File]::WriteAllText($moduleFile, $text)
}

function Invoke-Build([string]$Title) {
    Write-Host "`n=== $Title ===" -ForegroundColor Cyan
    & bazel "--output_base=$OutputBase" build --lockfile_mode=off //:main 2>&1 | ForEach-Object { "$_" } | Write-Host
    return $LASTEXITCODE
}

try {
    Use-MyLib '1.0'
    $first = Invoke-Build 'Build with mylib 1.0'
    if ($first -ne 0) {
        throw "The build with mylib 1.0 failed (exit code $first)."
    }

    Use-MyLib '2.0'
    if ($Prune) {
        Write-Host "`n=== Deleting bazel-out/*/bin/_virtual_includes ===" -ForegroundColor Cyan
        foreach ($config in Get-ChildItem -LiteralPath "$OutputBase/execroot/_main/bazel-out" -Directory) {
            $dir = Join-Path $config.FullName 'bin\_virtual_includes'
            if (Test-Path -LiteralPath $dir) {
                cmd /c "rmdir /s /q `"$dir`""
            }
        }
    }
    $second = Invoke-Build 'Build with mylib 2.0 (same output base)'

    Write-Host "`n=== Copies of scan.h in bazel-out ===" -ForegroundColor Cyan
    Get-ChildItem -LiteralPath "$OutputBase/execroot/_main/bazel-out" -Recurse -Filter scan.h -ErrorAction SilentlyContinue |
        Where-Object { $_.FullName -like '*_virtual_includes*' } |
        ForEach-Object { '{0}: {1}' -f $_.FullName, (Select-String -LiteralPath $_.FullName -Pattern 'scan_reset').Line }

    if ($second -ne 0) {
        Write-Host "`nREPRODUCED: the build with mylib 2.0 failed although a fresh 2.0 build succeeds." -ForegroundColor Red
        exit 1
    }
    Write-Host "`nNOT REPRODUCED: the build with mylib 2.0 succeeded." -ForegroundColor Green
    exit 0
} finally {
    [IO.File]::WriteAllText($moduleFile, $original)
    if ($ownOutputBase) {
        & bazel "--output_base=$OutputBase" clean --expunge 2>&1 | Out-Null
        Remove-Item -LiteralPath $OutputBase -Recurse -Force -ErrorAction SilentlyContinue
    }
}
