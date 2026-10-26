[CmdletBinding()]
param(
    [switch]$SkipBuild,
    [switch]$WithoutEngine,
    [switch]$SkipVerification,
    [ValidatePattern('^([0-9A-Za-z][0-9A-Za-z._-]*)?$')][string]$Version='',
    [string[]]$Languages=@('en-US','zh-CN','zh-TW')
)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
. (Join-Path $PSScriptRoot 'portable-tools.ps1')
Add-Type -AssemblyName System.IO.Compression.FileSystem
$repoPath=[IO.Path]::GetFullPath((Split-Path -Parent $PSScriptRoot))
$release=Join-Path $repoPath 'build/winui/Release'
$workRoot=Join-Path $repoPath 'build/packages'
[IO.Directory]::CreateDirectory($workRoot)|Out-Null
$untracked=@(& git -C $repoPath -c advice.useCoreFSMonitorConfig=false ls-files --others --exclude-standard -- app src include assets external scripts packaging cmake)
if($LASTEXITCODE -ne 0){throw 'Packaging requires a Git working tree'}
if($untracked.Count){throw "Stage or ignore new source files before packaging: $($untracked -join ', ')"}
if(-not $SkipBuild){
    & (Join-Path $PSScriptRoot 'build.ps1') -Configuration Release -SkipTests
    if($LASTEXITCODE -ne 0){throw 'Release build failed'}
}
$manifest=Join-Path $repoPath 'build/winui/obj/Release/XiangqiNative.vcxproj.FileListAbsolute.txt'
if(-not(Test-Path -LiteralPath $manifest)){throw 'Build Release first; the MSBuild output manifest is missing.'}
$baseName='XiangqiNative'+$(if($Version){"-$Version"}else{''})+$(if($WithoutEngine){'-no-engine'}else{''})
$zip=Join-Path $workRoot "$baseName-win-x64-portable.zip"
$sourceZip=Join-Path $workRoot "$baseName-source.zip"
$stage=Join-Path $workRoot ('.portable-stage-'+[guid]::NewGuid().ToString('N'))
$verify=Join-Path $workRoot ('.portable-verify-'+[guid]::NewGuid().ToString('N'))
[IO.Directory]::CreateDirectory($stage)|Out-Null
$app=Join-Path $stage 'binary/XiangqiNative'
$source=Join-Path $stage 'source/XiangqiNative-source'
[IO.Directory]::CreateDirectory($app)|Out-Null
[IO.Directory]::CreateDirectory($source)|Out-Null
try {
    # Only build-produced runtime files are eligible. Never copy the output tree wholesale.
    $prefix=$release.TrimEnd('\','/')+[IO.Path]::DirectorySeparatorChar
    $runtime=[Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    foreach($line in Get-Content -LiteralPath $manifest){
        if(-not $line.Trim()){continue}
        $full=[IO.Path]::GetFullPath($line.Trim())
        if(-not $full.StartsWith($prefix,[StringComparison]::OrdinalIgnoreCase)){continue}
        $relative=$full.Substring($prefix.Length).Replace('\','/')
        $include=$false
        if($relative -notmatch '/'){
            $include=([IO.Path]::GetExtension($relative) -in @('.dll','.winmd','.pri')) -or
                     $relative -in @('XiangqiNative.exe','RestartAgent.exe')
            if($relative -eq 'Microsoft.UI.Designer.dll' -or
               ($relative -like 'XiangqiNative*' -and $relative -notin @('XiangqiNative.exe','XiangqiNative.pri'))){$include=$false}
        }elseif($relative -like 'Microsoft.UI.Xaml/Assets/*'){$include=$true}
        elseif($relative -match '^([^/]+)/[^/]+\.mui$'){
            $include=('*' -in $Languages) -or ($Matches[1] -in $Languages)
        }
        if($include -and $runtime.Add($relative)){Copy-XqnFile $full (Join-Path $app $relative)}
    }
    Copy-XqnFile (Join-Path $release 'XiangqiNative.exe') (Join-Path $app 'XiangqiNative.exe')
    foreach($required in @('XiangqiNative.exe','XiangqiNative.pri','Microsoft.ui.xaml.dll','Microsoft.UI.Xaml.Controls.dll','Microsoft.WindowsAppRuntime.dll')){
        if(-not(Test-Path -LiteralPath (Join-Path $app $required))){throw "Required runtime missing from build manifest: $required"}
    }
    foreach($name in @('board-grid.svg','board-grid-flipped.svg')){
        Copy-XqnFile (Join-Path $release "assets/$name") (Join-Path $app "assets/$name")
    }
    foreach($side in @('red','black')){
        foreach($piece in @('king','advisor','elephant','horse','rook','cannon','pawn')){
            $relative='assets/pieces/'+$side+'_'+$piece+'.svg'
            Copy-XqnFile (Join-Path $release $relative) (Join-Path $app $relative)
        }
    }
    Copy-XqnFile (Join-Path $release 'LICENSE') (Join-Path $app 'LICENSE')
    foreach($file in Get-ChildItem -LiteralPath (Join-Path $release 'licenses') -File){
        Copy-XqnFile $file.FullName (Join-Path $app "licenses/$($file.Name)")
    }
    Copy-XqnFile (Join-Path $repoPath 'packaging/README.zh-CN.txt') (Join-Path $app 'README.txt')
    Copy-XqnFile (Join-Path $repoPath 'packaging/guide.zh-CN.html') (Join-Path $app '使用指南.html')
    Copy-XqnFile (Join-Path $repoPath 'packaging/THIRD-PARTY-NOTICES.txt') (Join-Path $app 'THIRD-PARTY-NOTICES.txt')
    [IO.File]::WriteAllText((Join-Path $app 'portable.flag'),'Portable settings: Data/settings.json',[Text.UTF8Encoding]::new($false))
    if($WithoutEngine){
        [IO.File]::AppendAllText((Join-Path $app 'README.txt'),[Environment]::NewLine+'此包未附带引擎，请在“分析 → 选择引擎”中指定皮卡鱼。'+[Environment]::NewLine,[Text.UTF8Encoding]::new($false))
    }
    if(-not $WithoutEngine){
        foreach($name in @('Pikafish-Windows-x86-64-universal.exe','pikafish.nnue','Copying.txt','NNUE-License.md')){
            $engineInput=Join-Path $release "engines/$name"
            if(-not(Test-Path -LiteralPath $engineInput)){throw "Missing $name; run scripts/setup-engine.ps1 -Configuration Release, or use -WithoutEngine."}
            Copy-XqnFile $engineInput (Join-Path $app "engines/$name")
        }
    }
    # Audit normal and delayed PE imports; copy a VC redistributable only when actually imported.
    $queue=[Collections.Generic.Queue[string]]::new()
    Get-ChildItem -LiteralPath $app -Recurse -File | Where-Object Extension -In '.dll','.exe' | ForEach-Object {$queue.Enqueue($_.FullName)}
    $checked=[Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    $crtDirectory=$null
    while($queue.Count){
        $file=$queue.Dequeue()
        if(-not $checked.Add($file)){continue}
        foreach($dependency in Get-XqnPeImports $file){
            if($dependency -match '^(api-ms-|ext-ms-)'){continue}
            if(Test-Path -LiteralPath (Join-Path $app $dependency)){continue}
            if($dependency -match '^(msvcp[0-9]|vcruntime[0-9]|concrt[0-9]|vcomp[0-9])'){
                if(-not $crtDirectory){
                    $vswhere=Join-Path ([Environment]::GetFolderPath('ProgramFilesX86')) 'Microsoft Visual Studio/Installer/vswhere.exe'
                    if(-not(Test-Path -LiteralPath $vswhere)){throw "VC runtime required: $dependency; install the matching Visual Studio redist files on the packaging machine."}
                    $vsRoot=& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
                    $redistRoot=Join-Path $vsRoot 'VC/Redist/MSVC'
                    $crtDirectory=Get-ChildItem -LiteralPath $redistRoot -Recurse -Directory |
                        Where-Object {$_.Name -like 'Microsoft.VC*.CRT' -and $_.Parent.Name -eq 'x64'} |
                        Sort-Object FullName -Descending | Select-Object -First 1 -ExpandProperty FullName
                }
                $from=Join-Path $crtDirectory $dependency
                Copy-XqnFile $from (Join-Path $app $dependency)
                $queue.Enqueue((Join-Path $app $dependency))
            }elseif(-not(Test-Path -LiteralPath (Join-Path ([Environment]::GetFolderPath('System')) $dependency))){
                throw "Unresolved runtime import $dependency in $file"
            }
        }
    }
    # Source companion uses current tracked contents, never Git metadata or personal/build directories.
    $tracked=& git -C $repoPath -c advice.useCoreFSMonitorConfig=false -c core.quotepath=false ls-files
    if($LASTEXITCODE -ne 0){throw 'Git is required to collect the corresponding source'}
    foreach($relative in $tracked){
        $top=($relative -split '/')[0]
        if($top -notin @('app','assets','cmake','external','include','licenses','packaging','scripts','src','tests','docs') -and
           $relative -notin @('CMakeLists.txt','LICENSE','README.md','.gitattributes')){continue}
        $from=Join-Path $repoPath $relative
        if(Test-Path -LiteralPath $from -PathType Leaf){Copy-XqnFile $from (Join-Path $source $relative)}
    }
    $propsPath=Join-Path $repoPath 'build/native/winui/Release/NativeCore.props'
    [xml]$props=[IO.File]::ReadAllText($propsPath)
    $license=$props.SelectSingleNode("//*[local-name()='XiangqiOxqfLicense']").InnerText
    Copy-XqnSourceTree (Split-Path -Parent $license) (Join-Path $source 'third_party/OXQF')
    if(-not $WithoutEngine){
        $engineSource=Join-Path $repoPath '.packages/Pikafish.2026-09-06-source.tar.gz'
        $expected='4D905F5BB9D96632900AA88505D816E2D5997919E1375077A591CFEB0E6C7A10'
        $includedSource=Join-Path $repoPath 'third_party/Pikafish-2026-09-06-source.tar.gz'
        if(-not(Test-Path -LiteralPath $engineSource) -and (Test-Path -LiteralPath $includedSource)){Copy-XqnFile $includedSource $engineSource}
        if(-not(Test-Path -LiteralPath $engineSource)){
            [Net.ServicePointManager]::SecurityProtocol=[Net.ServicePointManager]::SecurityProtocol -bor [Net.SecurityProtocolType]::Tls12
            Invoke-WebRequest -UseBasicParsing -Uri 'https://codeload.github.com/official-pikafish/Pikafish/tar.gz/refs/tags/Pikafish-2026-09-06' -OutFile $engineSource -TimeoutSec 60
        }
        if((Get-FileHash -LiteralPath $engineSource -Algorithm SHA256).Hash -ne $expected){throw 'Pikafish source archive hash mismatch'}
        Copy-XqnFile $engineSource (Join-Path $source 'third_party/Pikafish-2026-09-06-source.tar.gz')
    }
    Copy-XqnFile (Join-Path $repoPath 'packaging/SOURCE-BUILD.txt') (Join-Path $source 'SOURCE-BUILD.txt')
    $sourceNotice="Corresponding source: $([IO.Path]::GetFileName($sourceZip))"+[Environment]::NewLine+
        'https://github.com/pkuimyy/xiangqi-native'+[Environment]::NewLine
    [IO.File]::WriteAllText((Join-Path $app 'SOURCE.txt'),$sourceNotice,[Text.UTF8Encoding]::new($false))
    $files=@(Get-ChildItem -LiteralPath $app -Recurse -File | Sort-Object FullName | ForEach-Object {
        $relative=$_.FullName.Substring($app.Length+1).Replace('\','/')
        if($relative -match '(^|/)(Data|smoke|layout-preview|\.git|\.packages|obj)/' -or
           $_.Extension -in @('.pdb','.ilk','.obj','.lib','.exp','.log','.ps1','.oxq','.pgn')){
            throw "Forbidden file in portable package: $relative"
        }
        [pscustomobject]@{path=$relative;bytes=$_.Length;sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash}
    })
    $temporaryZip=Join-Path $stage 'portable.zip'
    Compress-XqnFolder (Join-Path $stage 'binary') $temporaryZip
    $temporarySource=Join-Path $stage 'source.zip'
    Compress-XqnFolder (Join-Path $stage 'source') $temporarySource
    $verification=@()
    if(-not $SkipVerification){
        [IO.Directory]::CreateDirectory($verify)|Out-Null
        $first=Join-Path $verify '位置 A'
        $second=Join-Path $verify '迁移位置 B'
        [IO.Compression.ZipFile]::ExtractToDirectory($temporaryZip,$first)
        $firstApp=Join-Path $first 'XiangqiNative'
        $verification+=Test-XqnPortable $firstApp
        [IO.Compression.ZipFile]::ExtractToDirectory($temporaryZip,$second)
        $secondApp=Join-Path $second 'XiangqiNative'
        Copy-XqnFile (Join-Path $firstApp 'Data/settings.json') (Join-Path $secondApp 'Data/settings.json')
        $verification+=Test-XqnPortable $secondApp
        if(-not $verification[1].restored){throw 'Portable relocation did not restore settings'}
    }
    # Publish only after archive and relocation checks have succeeded.
    Move-Item -LiteralPath $temporaryZip -Destination $zip -Force
    Move-Item -LiteralPath $temporarySource -Destination $sourceZip -Force
    $revision=& git -C $repoPath -c advice.useCoreFSMonitorConfig=false rev-parse HEAD
    $summary=[pscustomobject]@{
        name='XiangqiNative';version=$Version;revision=$revision;architecture='x64';
        engine=(-not $WithoutEngine);verified=(-not $SkipVerification);
        files=$files;verification=$verification
    }
    [IO.File]::WriteAllText((Join-Path $workRoot "$baseName-win-x64-portable.files.json"),($summary|ConvertTo-Json -Depth 8),[Text.UTF8Encoding]::new($false))
    $hashes=@($zip,$sourceZip) | ForEach-Object {"$((Get-FileHash -LiteralPath $_ -Algorithm SHA256).Hash) *$([IO.Path]::GetFileName($_))"}
    [IO.File]::WriteAllText((Join-Path $workRoot "$baseName-SHA256SUMS.txt"),($hashes -join [Environment]::NewLine)+[Environment]::NewLine,[Text.Encoding]::ASCII)
    [pscustomobject]@{
        PortableZip=$zip;SourceZip=$sourceZip;FileCount=$files.Count;
        UncompressedMB=[Math]::Round((($files|Measure-Object bytes -Sum).Sum/1MB),2);
        ZipMB=[Math]::Round((Get-Item -LiteralPath $zip).Length/1MB,2);
        Verified=(-not $SkipVerification)
    } | Format-List
}finally{
    Remove-XqnWorkTree $stage $workRoot
    Remove-XqnWorkTree $verify $workRoot
}
