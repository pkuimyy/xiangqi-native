# PowerShell 5.1-compatible helpers for portable packaging.
function Get-XqnPeImports([string]$Path) {
    $data = [IO.File]::ReadAllBytes($Path)
    if ($data.Length -lt 256 -or [BitConverter]::ToUInt16($data,0) -ne 0x5a4d) { throw "Not a PE file: $Path" }
    $pe = [BitConverter]::ToInt32($data,0x3c)
    if ([BitConverter]::ToUInt32($data,$pe) -ne 0x4550 -or [BitConverter]::ToUInt16($data,$pe+4) -ne 0x8664) { throw "Expected x64 PE: $Path" }
    $optional = $pe + 24
    if ([BitConverter]::ToUInt16($data,$optional) -ne 0x20b) { throw "Expected PE32+: $Path" }
    $sectionCount = [BitConverter]::ToUInt16($data,$pe+6)
    $sectionTable = $optional + [BitConverter]::ToUInt16($data,$pe+20)
    $sections = @()
    for ($i=0;$i -lt $sectionCount;$i++) {
        $at=$sectionTable+40*$i
        $sections += [pscustomobject]@{
            Rva=[BitConverter]::ToUInt32($data,$at+12)
            Size=[Math]::Max([BitConverter]::ToUInt32($data,$at+8),[BitConverter]::ToUInt32($data,$at+16))
            Offset=[BitConverter]::ToUInt32($data,$at+20)
        }
    }
    $rvaOffset = {
        param([uint32]$rva)
        if ($rva -lt [BitConverter]::ToUInt32($data,$optional+60)) { return [int]$rva }
        foreach($section in $sections) {
            if($rva -ge $section.Rva -and $rva -lt $section.Rva+$section.Size) { return [int]($section.Offset+$rva-$section.Rva) }
        }
        throw "Invalid import RVA in $Path"
    }
    $names=[Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    foreach($table in @(@(1,20,12),@(13,32,4))) {
        $rva=[BitConverter]::ToUInt32($data,$optional+112+8*$table[0])
        if(-not $rva){continue}
        $at=& $rvaOffset $rva
        for($i=0;$i -lt 4096;$i++) {
            $nameRva=[BitConverter]::ToUInt32($data,$at+$table[2])
            if(-not $nameRva){break}
            if($table[0] -eq 13 -and -not([BitConverter]::ToUInt32($data,$at) -band 1)) { throw "Unsupported delay imports: $Path" }
            $nameAt=& $rvaOffset $nameRva
            $end=$nameAt
            while($end -lt $data.Length -and $data[$end]){$end++}
            [void]$names.Add([Text.Encoding]::ASCII.GetString($data,$nameAt,$end-$nameAt))
            $at += $table[1]
        }
    }
    return @($names | Sort-Object)
}
function Copy-XqnFile([string]$Source,[string]$Destination) {
    if(-not(Test-Path -LiteralPath $Source -PathType Leaf)){throw "Missing package input: $Source"}
    if((Get-Item -LiteralPath $Source).Attributes -band [IO.FileAttributes]::ReparsePoint){throw "Package input is a link: $Source"}
    [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($Destination)) | Out-Null
    [IO.File]::Copy($Source,$Destination,$true)
}
function Remove-XqnWorkTree([string]$Path,[string]$WorkRoot) {
    $absolute=[IO.Path]::GetFullPath($Path)
    $parent=[IO.Path]::GetFullPath($WorkRoot).TrimEnd('\','/')+[IO.Path]::DirectorySeparatorChar
    if(-not $absolute.StartsWith($parent,[StringComparison]::OrdinalIgnoreCase) -or
       [IO.Path]::GetFileName($absolute) -notmatch '^\.portable-(stage|verify)-[a-f0-9]{32}$') {throw "Unsafe cleanup target: $absolute"}
    if(Test-Path -LiteralPath $absolute){
        if((Get-Item -LiteralPath $absolute).Attributes -band [IO.FileAttributes]::ReparsePoint){throw "Refusing linked cleanup target: $absolute"}
        Remove-Item -LiteralPath $absolute -Recurse -Force
    }
}
function Test-XqnPortable([string]$Application) {
    $exe=Join-Path $Application 'XiangqiNative.exe'
    $report=Join-Path $Application 'portable-check.json'
    $process=Start-Process -FilePath $exe -ArgumentList '--portable-self-test' -WorkingDirectory ([IO.Path]::GetTempPath()) -WindowStyle Hidden -PassThru
    if(-not $process.WaitForExit(30000)){
        if($process.Path -eq $exe){Stop-Process -Id $process.Id}
        throw 'Portable startup verification timed out'
    }
    if($process.ExitCode -ne 0 -or -not(Test-Path -LiteralPath $report)){throw "Portable startup failed: $($process.ExitCode)"}
    $result=Get-Content -LiteralPath $report -Raw | ConvertFrom-Json
    if(-not $result.passed){throw "Portable self-check failed: $(Get-Content -LiteralPath $report -Raw)"}
    return $result
}
function Copy-XqnSourceTree([string]$Source,[string]$Destination) {
    [IO.Directory]::CreateDirectory($Destination) | Out-Null
    foreach($item in Get-ChildItem -LiteralPath $Source -Force){
        if($item.Name -in @('.git','build','node_modules','.packages')){continue}
        if($item.Attributes -band [IO.FileAttributes]::ReparsePoint){throw "Linked source input: $($item.FullName)"}
        $target=Join-Path $Destination $item.Name
        if($item.PSIsContainer){Copy-XqnSourceTree $item.FullName $target}else{Copy-XqnFile $item.FullName $target}
    }
}
function Compress-XqnFolder([string]$Root,[string]$Destination) {
    $prefix=[IO.Path]::GetFullPath($Root).TrimEnd('\','/')+[IO.Path]::DirectorySeparatorChar
    $stream=[IO.File]::Open($Destination,[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::None)
    try {
        $zip=[IO.Compression.ZipArchive]::new($stream,[IO.Compression.ZipArchiveMode]::Create,$true,[Text.Encoding]::UTF8)
        try {
            foreach($file in Get-ChildItem -LiteralPath $Root -Recurse -File | Sort-Object FullName) {
                $name=$file.FullName.Substring($prefix.Length).Replace('\','/')
                $entry=$zip.CreateEntry($name,[IO.Compression.CompressionLevel]::Optimal)
                if($file.LastWriteTime.Year -ge 1980 -and $file.LastWriteTime.Year -le 2107){
                    $entry.LastWriteTime=[DateTimeOffset]$file.LastWriteTime
                }
                $inputStream=[IO.File]::OpenRead($file.FullName)
                $outputStream=$entry.Open()
                try{$inputStream.CopyTo($outputStream)}finally{$outputStream.Dispose();$inputStream.Dispose()}
            }
        }finally{$zip.Dispose()}
    }finally{$stream.Dispose()}
}
