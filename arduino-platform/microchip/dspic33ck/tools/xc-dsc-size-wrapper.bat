@echo off
REM Report flash and RAM usage for the Arduino IDE's size bar.
REM
REM     xc-dsc-size-wrapper.bat <elf>
REM
REM xc-dsc-size does not bucket the dsPIC section layout the way the IDE expects,
REM so this reads the section headers with xc-dsc-readelf and totals them here,
REM printing the five lines recipe.size.regex / .regex.data match against.
REM
REM The compiler directory is resolved here rather than handed in by
REM platform.txt, because platform.txt no longer knows it - see xc-dsc-find.bat.
call "%~dp0xc-dsc-find.bat" xcdsc
if errorlevel 1 exit /b 1

setlocal
set "ELF=%~1"
powershell -NoProfile -Command "$out = & '%XCDSC_DIR%\xc-dsc-readelf.exe' -S '%ELF%' 2>$null; $text = 0; $data = 0; $bss = 0; foreach ($line in $out) { if ($line -match '\]\s+(\S+)\s+PROGBITS\s+\S+\s+\S+\s+([0-9a-fA-F]+)') { $name = $Matches[1]; $size = [Convert]::ToInt32($Matches[2], 16); if ($name -like '.text*' -or $name -eq '.dinit' -or $name -eq '.reset' -or $name -like '.const*') { $text += $size } elseif ($name -like '.data*') { $data += $size } } elseif ($line -match '\]\s+(\S+)\s+NOBITS\s+\S+\s+\S+\s+([0-9a-fA-F]+)') { $name = $Matches[1]; $size = [Convert]::ToInt32($Matches[2], 16); if ($name -like '.bss*' -or $name -like '.nbss*') { $bss += $size } } }; Write-Host ('.text          ' + $text); Write-Host ('.data          ' + $data); Write-Host ('.rodata        0'); Write-Host ('.bss           ' + $bss); Write-Host ('.noinit        0')"
exit /b 0
