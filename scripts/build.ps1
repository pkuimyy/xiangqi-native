param([ValidateSet('Debug','Release')][string]$Configuration='Debug',[switch]$CoreOnly,[string]$OutputDirectory)
$ErrorActionPreference='Stop'
$repoPath=Split-Path -Parent $PSScriptRoot
$programFilesX86=[Environment]::GetFolderPath('ProgramFilesX86')
$vswhere=Join-Path $programFilesX86 'Microsoft Visual Studio\Installer\vswhere.exe'
$vsRoot=& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(-not $vsRoot){throw 'Install Visual Studio C++ desktop tools'}
$initCommand='"'+(Join-Path $vsRoot 'Common7\Tools\VsDevCmd.bat')+'" -arch=x64 -host_arch=x64 >nul && set'
$env:VSLANG='1033'
$envLines=& cmd.exe /d /s /c $initCommand
if($LASTEXITCODE -ne 0){throw 'Cannot initialize MSVC'}
foreach($line in $envLines){if($line -match '^([^=]+)=(.*)$'){[Environment]::SetEnvironmentVariable($matches[1],$matches[2],'Process')}}
$env:VSLANG='1033'
$cmake=Join-Path $vsRoot 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
$ctest=Join-Path (Split-Path $cmake -Parent) 'ctest.exe'
$ninja=Join-Path $vsRoot 'Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe'
Push-Location $repoPath
try{
 & $cmake -S . -B build/native -G 'Ninja Multi-Config' "-DCMAKE_MAKE_PROGRAM=$ninja"
 if($LASTEXITCODE -ne 0){throw 'CMake configure failed'}
 & $cmake --build build/native --config $Configuration --target xiangqi_cli xiangqi_core_tests xiangqi_engine_tests xiangqi_live_tests fake_uci xiangqi_parity_tests xiangqi_rules_tests
 if($LASTEXITCODE -ne 0){throw 'Native build failed'}
 & $ctest --test-dir build/native -C $Configuration --output-on-failure
 if($LASTEXITCODE -ne 0){throw 'Native tests failed'}
 if(-not $CoreOnly){
  $msbuild=Join-Path $vsRoot 'MSBuild\Current\Bin\MSBuild.exe'
  $buildArgs=@('app/XiangqiNative.vcxproj','/restore','/m','/v:minimal',"/p:Configuration=$Configuration",'/p:Platform=x64','/p:RestoreLockedMode=true')
  if($OutputDirectory){
   $outputPath=if([IO.Path]::IsPathRooted($OutputDirectory)){$OutputDirectory}else{Join-Path $repoPath $OutputDirectory}
   $buildArgs+=('/p:OutDir='+[IO.Path]::GetFullPath($outputPath)+[IO.Path]::DirectorySeparatorChar)
  }
  & $msbuild @buildArgs
  if($LASTEXITCODE -ne 0){throw 'WinUI build failed'}
 }
}finally{Pop-Location}
