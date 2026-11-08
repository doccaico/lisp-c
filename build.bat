@echo off

setlocal

if "%~0"=="" (
    echo build.bat ["" or debug^|leak-check^|release]
    exit /b 1
)

set "EXE_NAME=lisp-c"

set OUT_DIR=build
set OUT_DIR_BIN=%OUT_DIR%\bin
set OUT_DIR_OBJ=%OUT_DIR%\obj

set "C_FLAGS=/nologo"
set "C_FLAGS=%C_FLAGS% /std:c17 /utf-8"
REM set "C_FLAGS=%C_FLAGS% /std:c11 /utf-8"
set "C_FLAGS=%C_FLAGS% -W4 -wd4127 -wd4146 -wd4505 -wd4456 -wd4457"
set "C_FLAGS=%C_FLAGS% /DWIN32 /DUNICODE /D_UNICODE /D_CRT_SECURE_NO_WARNINGS"
set "C_FLAGS=%C_FLAGS% /DNDEBUG /D_NDEBUG"
set "C_FLAGS=%C_FLAGS% /I vendor/STC/include"
REM set "C_FLAGS=%C_FLAGS% /I vendor"

set "L_FLAGS=/link"
set "L_FLAGS=%L_FLAGS% /INCREMENTAL:no"

set "TARGET_MODE=%~1"
if "%TARGET_MODE%"=="" set "TARGET_MODE=debug"

if /i "%TARGET_MODE%"=="debug" (
    set "MODE=[DEBUG]"
    set "C_FLAGS=%C_FLAGS% /Od /Zi /fsanitize=address"
) else if /i "%TARGET_MODE%"=="leak-check" (
    set "MODE=[LEAK CHECK]"
    set "C_FLAGS=%C_FLAGS% /Od /MDd /Zi /DENABLE_LEAK_CHECK"
) else if /i "%TARGET_MODE%"=="release" (
    set "MODE=[RELESE]"
    set "C_FLAGS=%C_FLAGS% /O2 /MD /GF"
) else (
    echo build.bat ["" or debug^|leak-check^|release]
    exit /b 1
)

if not exist %OUT_DIR% mkdir %OUT_DIR%
if not exist %OUT_DIR_BIN% mkdir %OUT_DIR_BIN%
if not exist %OUT_DIR_OBJ% mkdir %OUT_DIR_OBJ%

cl src\main.c /Fd"%OUT_DIR_OBJ%\\" /Fo"%OUT_DIR_OBJ%\\" /Fe"%OUT_DIR_BIN%\\%EXE_NAME%" %C_FLAGS% %L_FLAGS%
if %ERRORLEVEL% neq 0 exit /b 1

for /f %%i in ('cmd /k prompt $e^<nul') do set ESC=%%i

echo %ESC%[1;32m%MODE%%ESC%[0m An executable file has been generated: %ESC%[1m%OUT_DIR%\bin\%EXE_NAME%.exe%ESC%[0m

endlocal
