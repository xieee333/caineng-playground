param(
    [string]$EditorPath = 'D:\代码\caineng-playground\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe',
    [string]$ProjectPath = 'D:\UEProjects\CainengPlayground 5.8\CainengPlayground.uproject',
    [switch]$Practice,
    [switch]$Probe
)
$ErrorActionPreference = 'Stop'
if (!(Test-Path -LiteralPath $EditorPath -PathType Leaf)) { throw "UE editor not found: $EditorPath" }
if (!(Test-Path -LiteralPath $ProjectPath -PathType Leaf)) { throw "Project not found: $ProjectPath" }
if ($Practice -and $Probe) { throw 'Choose Practice or Probe, not both.' }
$runArgs = '"' + $ProjectPath + '" -game -windowed -ResX=1280 -ResY=720 -CanergyCarryPrototype'
if ($Practice) { $runArgs += ' -CanergyCarryPractice' }
if ($Probe) { $runArgs += ' -CanergyCarryProbe' }
# This is the visible interactive game requested by the user, not a background helper.
Start-Process -FilePath $EditorPath -ArgumentList $runArgs
