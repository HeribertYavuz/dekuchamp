::Parameters:
::  %1 - x86 or x86_amd64

call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" %1

if "%1"=="x86" (
    set Dekuchamp_catalog=Dekuchampx86.cat
    set Dekuchamp_OS=8_X86
    set Dekuchamp_builddir=DekuchampDriver\Win8Release\x86
) else (
    set Dekuchamp_catalog=Dekuchampamd64.cat
    set Dekuchamp_OS=8_X64
    set Dekuchamp_builddir=DekuchampDriver\Win8Release\x64
)

cd %~dp0
CALL config.bat

::Delete the release directory if it already exists
if exist %Dekuchamp_builddir% RMDIR /S /Q %Dekuchamp_builddir%

Nmake2MsBuild dirs
MSBuild dirs.sln /p:Configuration="Win8 Release"

%_DEKUCHAMP_SIGNTOOL% %_DEKUCHAMP_SIGN_OPTS_SHA256% %Dekuchamp_builddir%\Dekuchamp.sys
if errorlevel 1 goto error

Inf2cat.exe /driver:%Dekuchamp_builddir%\ /os:%Dekuchamp_OS%

goto end

:error
echo ===== BUILD FAILED! =====
pause
exit /B 1

:end
copy %Dekuchamp_builddir%\Dekuchamp.sys %2
copy %Dekuchamp_builddir%\Dekuchamp.pdb %2
copy %Dekuchamp_builddir%\Dekuchamp.inf %2
copy %Dekuchamp_builddir%\%Dekuchamp_catalog% %2
%_DEKUCHAMP_SIGNTOOL% %_DEKUCHAMP_SIGN_OPTS_SHA256% %2\%Dekuchamp_catalog%
if errorlevel 1 goto error

exit /B 0
