call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x86_amd64

set Dekuchamp_catalog=Dekuchampamd64.cat
set Dekuchamp_OS=8_X64
set Dekuchamp_builddir=x64\Win8Release\dirs-Package

cd %~dp0
CALL config.bat

::Delete the release directory if it already exists
if exist %Dekuchamp_builddir% RMDIR /S /Q %Dekuchamp_builddir%

Nmake2MsBuild dirs
MSBuild dirs.sln /p:Configuration="Win8 Release"

if errorlevel 1 goto error

::Consider the build a success even if test signing fails
::Actual signing that matter is release one which for obvious reasons
::is done using non-public keys
%_DEKUCHAMP_SIGNTOOL% %_DEKUCHAMP_SIGN_OPTS_SHA256% %Dekuchamp_builddir%\Dekuchamp.sys
Inf2cat.exe /driver:%Dekuchamp_builddir%\ /os:%Dekuchamp_OS%

copy %Dekuchamp_builddir%\Dekuchamp.sys %2
copy %Dekuchamp_builddir%\Dekuchamp.inf %2
copy %Dekuchamp_builddir%\%Dekuchamp_catalog% %2
%_DEKUCHAMP_SIGNTOOL% %_DEKUCHAMP_SIGN_OPTS_SHA256% %2\%Dekuchamp_catalog%

exit /B 0

:error
echo ===== BUILD FAILED! =====
exit /B 1
