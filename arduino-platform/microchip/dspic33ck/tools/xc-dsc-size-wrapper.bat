@echo off
set "BIN=%~1"
set "ELF=%~2"
if "%ELF%"=="" set "ELF=%~1"
powershell -NoProfile -Command "$out = & '%BIN%xc-dsc-readelf.exe' -S '%ELF%' 2>$null; $text = 0; $data = 0; $bss = 0; foreach ($line in $out) { if ($line -match '\]\s+(\S+)\s+PROGBITS\s+\S+\s+\S+\s+([0-9a-fA-F]+)') { $name = $Matches[1]; $size = [Convert]::ToInt32($Matches[2], 16); if ($name -like '.text*' -or $name -eq '.dinit' -or $name -eq '.reset' -or $name -like '.const*') { $text += $size } elseif ($name -like '.data*') { $data += $size } } elseif ($line -match '\]\s+(\S+)\s+NOBITS\s+\S+\s+\S+\s+([0-9a-fA-F]+)') { $name = $Matches[1]; $size = [Convert]::ToInt32($Matches[2], 16); if ($name -like '.bss*' -or $name -like '.nbss*') { $bss += $size } } }; Write-Host ('.text          ' + $text); Write-Host ('.data          ' + $data); Write-Host ('.rodata        0'); Write-Host ('.bss           ' + $bss); Write-Host ('.noinit        0')"
exit /b 0
