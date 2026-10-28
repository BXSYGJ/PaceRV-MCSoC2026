param()
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$OutputEncoding = [System.Text.UTF8Encoding]::new($true)
$ErrorActionPreference = 'Stop'
try {
    $root = [System.IO.Path]::GetFullPath([System.IO.Path]::Combine($PSScriptRoot, '..'))
    $external = [System.IO.Path]::Combine($root, 'external')
    $target = [System.IO.Path]::Combine($external, 'PaceRV')
    $expected = 'b3ff0f886bb878d8ebac44366f3182eb6cbb5241'
    $gitCommand = Get-Command git -ErrorAction Stop
    if (-not (Test-Path -LiteralPath $target)) {
        New-Item -ItemType Directory -Path $external -Force | Out-Null
        & $gitCommand.Source clone --branch v1.0.0 --single-branch https://github.com/BXSYGJ/PaceRV.git $target
        if ($LASTEXITCODE -ne 0) { throw 'PaceRV clone failed.' }
    }
    $actual = & $gitCommand.Source -C $target rev-parse HEAD
    if ($LASTEXITCODE -ne 0 -or $actual.Trim() -ne $expected) {
        throw 'Existing PaceRV checkout differs from the paper release. Preserve it and select a separate checkout.'
    }
    Write-Output ('Verified PaceRV release: ' + $actual.Trim())
    Write-Output $target
} catch {
    Write-Error $_
    exit 1
}
