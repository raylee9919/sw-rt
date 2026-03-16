@echo off
setlocal
cd "%~dp0"



:: Get cl.exe
where cl >nul 2>nul
if %errorlevel%==1 (
    echo Looking for 'vcvars64.bat'.. Recommended to run from the Developer Command Prompt.
    @call "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
)

where /q cl || (
    echo [ERROR]: "cl" not found - please run this from the MSVC x64 native tools command prompt.
    exit /b 1
)



for %%a in (%*) do set "%%a=1"

set flags_common=/nologo /Od /Zi /W4 /wd4100 /wd4201 /wd4505 /wd4146 /wd4456 /wd4244 /EHsc- /D_CRT_SECURE_NO_WARNINGS 
set flags_linker=/incremental:no /opt:ref

if not exist build mkdir build
pushd build

if "%debug%"=="1" (
    echo Debug build
    call cl %flags_common% ../code/main.cpp ../code/obj.cpp /Fe:swrt.exe /link %flags_linker%
) else (
    echo Release build
    call cl %flags_common% ../code/main.cpp ../code/obj.cpp /Fe:swrt.exe /link %flags_linker%
)

popd
