param([Parameter(Mandatory=$true)][string]$GameDirectory,[switch]$ValidateOnly)
$ErrorActionPreference='Stop'
$game=(Resolve-Path -LiteralPath $GameDirectory).Path
foreach($name in @('Ace7Game.exe','dxgi.dll','nvngx_dlss.dll','ReShade.ini','Luma-Unreal Engine.addon')) {
 if(-not (Test-Path -LiteralPath (Join-Path $game $name) -PathType Leaf)){throw "Missing $name. Install the matching official Luma Unreal Engine package first."}
}
if(-not (Test-Path -LiteralPath (Join-Path $game 'Luma\Unreal Engine') -PathType Container)){throw 'Missing official Luma Unreal Engine shaders.'}
$addon=Join-Path $PSScriptRoot 'Luma-Unreal Engine.addon'
$expected='290DE5048C09954D4ADEA1C3F1E1FB666DB78A94F5418D42A5CE11F828A3ED56'
if((Get-FileHash -LiteralPath $addon -Algorithm SHA256).Hash -ne $expected){throw 'Patch checksum mismatch; download a fresh release archive.'}
if($ValidateOnly){Write-Output 'Package checksum and installation prerequisites passed. No changes made.';return}
$running=Get-Process Ace7Game -ErrorAction SilentlyContinue | Where-Object { -not $_.Path -or [IO.Path]::GetDirectoryName($_.Path) -eq $game }
if($running){throw 'Close Ace Combat 7 before installing.'}
$backup=Join-Path $game ('AC7-DLSS-Backup-'+(Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
New-Item -ItemType Directory -Path $backup | Out-Null
foreach($name in @('Luma-Unreal Engine.addon','ReShade.ini')){Copy-Item -LiteralPath (Join-Path $game $name) -Destination (Join-Path $backup $name)}
$marker=Join-Path $game 'ac7-scaling-experimental.enabled'
$hadMarker=Test-Path -LiteralPath $marker
if($hadMarker){Copy-Item -LiteralPath $marker -Destination (Join-Path $backup 'ac7-scaling-experimental.enabled')}
@{HadScalingMarker=$hadMarker;Version='v0.2.0-beta.1';GameDirectory=$game;Created=(Get-Date).ToString('o')} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $backup 'manifest.json')
$ini=Join-Path $game 'ReShade.ini';$text=[IO.File]::ReadAllText($ini)
$section=[regex]::Match($text,'(?msi)^\[Luma\][^\r\n]*\r?\n.*?(?=^\[|\z)')
$body=if($section.Success){$section.Value}else{"[Luma]`r`n"}
foreach($item in @(@('SRUserType','2'),@('DLSSRenderPreset','13'),@('FirstBoot','0'),@('AC7ScalingMode','1'))){
 $pattern='(?mi)^'+[regex]::Escape($item[0])+'\s*=.*$'
 if([regex]::IsMatch($body,$pattern)){$body=[regex]::Replace($body,$pattern,($item[0]+'='+$item[1]))}
 else{$body=$body.TrimEnd()+"`r`n"+$item[0]+'='+$item[1]+"`r`n"}
}
if($section.Success){$text=$text.Substring(0,$section.Index)+$body+$text.Substring($section.Index+$section.Length)}else{$text=$text.TrimEnd()+"`r`n`r`n"+$body}
try {
 Copy-Item -LiteralPath $addon -Destination (Join-Path $game 'Luma-Unreal Engine.addon') -Force
 [IO.File]::WriteAllText($ini,$text,(New-Object Text.UTF8Encoding($false)))
 [IO.File]::WriteAllText($marker,'')
 if((Get-FileHash -LiteralPath (Join-Path $game 'Luma-Unreal Engine.addon')).Hash -ne $expected){throw 'Installed checksum mismatch.'}
} catch {
 if($hadMarker){Copy-Item -LiteralPath (Join-Path $backup 'ac7-scaling-experimental.enabled') -Destination $marker -Force}elseif(Test-Path -LiteralPath $marker){Remove-Item -LiteralPath $marker}
 foreach($name in @('Luma-Unreal Engine.addon','ReShade.ini')){Copy-Item -LiteralPath (Join-Path $backup $name) -Destination (Join-Path $game $name) -Force}
 throw
}
Write-Output "Installed AC7 DLSS v0.2.0-beta.1 (Quality selected), preset M. Use Home > Luma > AC7 DLSS Mode for Native DLAA, Quality, Balanced or Performance. Backup: $backup"
if(Test-Path -LiteralPath (Join-Path $game 'ac7-color-bridge.disabled')){Write-Warning 'The existing ac7-color-bridge.disabled marker is present; color conversion remains disabled until you remove that marker and restart.'}
