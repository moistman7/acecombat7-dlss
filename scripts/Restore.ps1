param([Parameter(Mandatory=$true)][string]$GameDirectory,[string]$BackupDirectory)
$ErrorActionPreference='Stop'
$game=(Resolve-Path -LiteralPath $GameDirectory).Path
$running=Get-Process Ace7Game -ErrorAction SilentlyContinue | Where-Object { -not $_.Path -or [IO.Path]::GetDirectoryName($_.Path) -eq $game }
if($running){throw 'Close Ace Combat 7 before restoring.'}
if(-not $BackupDirectory){$latest=Get-ChildItem -LiteralPath $game -Directory -Filter 'AC7-DLSS-Backup-*' | Where-Object {Test-Path -LiteralPath (Join-Path $_.FullName 'manifest.json')} | Sort-Object Name -Descending | Select-Object -First 1;if(-not $latest){throw 'No installer backup was found.'};$BackupDirectory=$latest.FullName}
$backup=(Resolve-Path -LiteralPath $BackupDirectory).Path
$record=Get-Content -LiteralPath (Join-Path $backup 'manifest.json') -Raw | ConvertFrom-Json
if($record.GameDirectory -ne $game){throw 'This backup belongs to a different game directory.'}
foreach($name in @('Luma-Unreal Engine.addon','ReShade.ini')){if(-not(Test-Path -LiteralPath (Join-Path $backup $name) -PathType Leaf)){throw "Incomplete backup: $name"}}
foreach($name in @('Luma-Unreal Engine.addon','ReShade.ini')){Copy-Item -LiteralPath (Join-Path $backup $name) -Destination (Join-Path $game $name) -Force}
Write-Output 'Restored the backed-up add-on and ReShade.ini. Captures and other mods were retained.'
