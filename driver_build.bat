::Parameters:
::  %1 - x86 or x64
::  %2 - WIN7
::  %3 - target directory

call C:\WinDDK\7600.16385.1\bin\setenv.bat C:\WinDDK\7600.16385.1\ fre %1 %2

cd %~dp0
CALL config.bat

if "%_BUILDARCH%"=="x86" (
    set Dekuchamp_arch=i386
    set Dekuchamp_catalog=Dekuchampx86.cat
    if "%DDK_TARGET_OS%"=="WinLH" (
        set Dekuchamp_OS=Vista_X86
    ) else if "%DDK_TARGET_OS%"=="WinXP" (
        set Dekuchamp_OS=XP_X86
    ) else if "%DDK_TARGET_OS%"=="Win7" (
        set Dekuchamp_OS=7_X86
    )
) else (
    set Dekuchamp_arch=amd64
    set Dekuchamp_catalog=Dekuchampamd64.cat
    if "%DDK_TARGET_OS%"=="WinLH" (
        set Dekuchamp_OS=Vista_X64
    ) else if "%DDK_TARGET_OS%"=="WinNET" (
        set Dekuchamp_OS=XP_X64
    ) else if "%DDK_TARGET_OS%"=="Win7" (
        set Dekuchamp_OS=7_X64
    )
)

cd %~dp0
CALL config.bat

build -ceZg
if exist build%BUILD_ALT_DIR%.err goto error

::Sign the DekuchampCMD.exe, it is not critical so do not fail on error
%_DEKUCHAMP_SIGNTOOL% %_DEKUCHAMP_SIGN_OPTS_SHA256% DekuchampCMD\obj%BUILD_ALT_DIR%\%Dekuchamp_arch%\DekuchampCMD.exe

%_DEKUCHAMP_SIGNTOOL% %_DEKUCHAMP_SIGN_OPTS_SHA256% DekuchampDriver\obj%BUILD_ALT_DIR%\%Dekuchamp_arch%\Dekuchamp.sys
if errorlevel 1 goto error

Inf2cat.exe /driver:DekuchampDriver\obj%BUILD_ALT_DIR%\%Dekuchamp_arch%\ /os:%Dekuchamp_OS%

goto end

:error
echo ===== BUILD FAILED! =====
pause
exit /B 1

:end

copy DekuchampDriver\obj%BUILD_ALT_DIR%\%Dekuchamp_arch%\Dekuchamp.sys %3
copy DekuchampDriver\obj%BUILD_ALT_DIR%\%Dekuchamp_arch%\Dekuchamp.pdb %3
copy DekuchampDriver\obj%BUILD_ALT_DIR%\%Dekuchamp_arch%\Dekuchamp.inf %3
copy DekuchampDriver\obj%BUILD_ALT_DIR%\%Dekuchamp_arch%\%Dekuchamp_catalog% %3
%_DEKUCHAMP_SIGNTOOL% %_DEKUCHAMP_SIGN_OPTS_SHA256% %3\%Dekuchamp_catalog%
if errorlevel 1 goto error

exit /B 0
