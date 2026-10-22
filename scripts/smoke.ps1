param([ValidateSet('Debug','Release')][string]$Configuration='Debug')
$ErrorActionPreference='Stop'
$repoPath=Split-Path -Parent $PSScriptRoot
$exe=Join-Path $repoPath "build/winui/$Configuration/XiangqiNative.exe"
$result=Join-Path (Split-Path $exe -Parent) 'smoke/result.json'
if(Test-Path -LiteralPath $result){Remove-Item -LiteralPath $result}
$process=Start-Process -FilePath $exe -ArgumentList '--smoke-test' -WorkingDirectory (Split-Path $exe -Parent) -WindowStyle Hidden -PassThru
if(-not $process.WaitForExit(30000)){
 if($process.Path -eq [IO.Path]::GetFullPath($exe)){Stop-Process -Id $process.Id}
 throw 'WinUI smoke test timed out'
}
if($process.ExitCode -ne 0){throw "WinUI process exited with $($process.ExitCode)"}
if(-not(Test-Path -LiteralPath $result)){throw 'WinUI did not produce a smoke result'}
$report=Get-Content -LiteralPath $result -Raw|ConvertFrom-Json
if(-not $report.passed){throw "WinUI smoke test failed: $(Get-Content -LiteralPath $result -Raw)"}
Get-Content -LiteralPath $result
