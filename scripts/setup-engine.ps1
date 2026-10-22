param([ValidateSet('Debug','Release')][string]$Configuration='Debug')
$ErrorActionPreference='Stop'
$repoPath=Split-Path -Parent $PSScriptRoot
$cache=Join-Path $repoPath '.packages/Pikafish.2026-09-06.7z'
$url='https://github.com/official-pikafish/Pikafish/releases/download/Pikafish-2026-09-06/Pikafish.2026-09-06.7z'
$expected='41952BBFE2520FACEB5902C69E6AB4845CC999841D2B49A95CC1BE7867A25E5B'
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $cache)|Out-Null
if(-not(Test-Path -LiteralPath $cache)){Invoke-WebRequest $url -OutFile $cache}
if((Get-FileHash -LiteralPath $cache -Algorithm SHA256).Hash -ne $expected){throw 'Pikafish archive hash mismatch'}
$target=Join-Path $repoPath "build/winui/$Configuration/engines"
New-Item -ItemType Directory -Force -Path $target|Out-Null
$sevenZip=Join-Path $env:ProgramFiles '7-Zip/7z.exe'
if(Test-Path -LiteralPath $sevenZip){
 & $sevenZip e $cache "-o$target" 'Pikafish-Windows-x86-64-universal.exe' 'pikafish.nnue' 'Copying.txt' 'NNUE-License.md' -y
}else{
 tar -xf $cache -C $target 'Pikafish-Windows-x86-64-universal.exe' 'pikafish.nnue' 'Copying.txt' 'NNUE-License.md'
}
if($LASTEXITCODE -ne 0){throw 'Pikafish extraction failed; install 7-Zip and retry'}
Write-Output "Pikafish installed in $target"
