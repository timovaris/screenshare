$ErrorActionPreference = 'Stop'
$installer = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\setup.exe'
$installation = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\2022\BuildTools'
if (!(Test-Path $installer)) { throw 'Install Visual Studio 2022 Build Tools first.' }
$arguments = 'modify --installPath "' + $installation + '" --add Microsoft.VisualStudio.Component.VC.Tools.x86.x64 --passive --norestart'
$process = Start-Process -FilePath $installer -ArgumentList $arguments -Verb RunAs -Wait -PassThru
if ($process.ExitCode -ne 0 -and $process.ExitCode -ne 3010) {
    throw "Visual Studio Installer exited with code $($process.ExitCode)."
}
