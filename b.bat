@echo off
setlocal
cd "%~dp0"

for %%a in (%*) do set "%%a=1"

set flags_common=/nologo /Od /Zi /W4 /wd4100 /wd4201 /wd4505 /wd4146 /wd4456 /wd4244 /EHsc- /D_CRT_SECURE_NO_WARNINGS 
set flags_linker=/incremental:no /opt:ref

if not exist build mkdir build
pushd build

if "%debug%"=="1" (
    echo Debug build
    call cl %flags_common% ../source/main.cpp ../source/obj.cpp /Fe:swrt.exe /link %flags_linker%
) else (
    echo Release build
    call cl %flags_common% ../source/main.cpp ../source/obj.cpp /Fe:swrt.exe /link %flags_linker%
)

popd
