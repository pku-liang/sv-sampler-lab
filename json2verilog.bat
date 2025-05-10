@echo off
setlocal enabledelayedexpansion

if "%1"=="" (
    goto :compile
) else if "%1"=="run" (
    if "%2"=="" (
        echo Please specify a JSON file path
        exit /b 1
    )
    goto :run
) else if "%1"=="run-basic" (
    goto :run_basic
) else if "%1"=="run-opt" (
    goto :run_opt
) else if "%1"=="clean" (
    goto :clean
) else (
    echo Unknown command: %1
    echo Available commands: [no args=compile] run [JSON file] run-basic run-opt clean
    exit /b 1
)

:compile
echo Compiling json2verilog...
if not exist build mkdir build
if not exist run_dir mkdir run_dir
g++ -std=c++11 -Wall -O2 -I./include srcs/json2verilog.cpp -o json2verilog
if %errorlevel% neq 0 (
    echo Compilation failed!
    exit /b %errorlevel%
)
echo Compilation successful!
goto :eof

:run
echo Processing file %2...
call :compile
json2verilog %2
if %errorlevel% neq 0 (
    echo Processing failed!
    exit /b %errorlevel%
)
echo Successfully processed file %2
goto :eof

:run_basic
call :compile
for %%f in (basic\*.json) do (
    echo Processing file %%f...
    json2verilog %%f
)
echo All basic test files processed!
goto :eof

:run_opt
call :compile
for %%f in (opt1\*.json) do (
    echo Processing file %%f...
    json2verilog %%f
)
echo All optimization test files processed!
goto :eof

:clean
echo Cleaning generated files...
if exist json2verilog.exe del json2verilog.exe
if exist run_dir\*.v del run_dir\*.v
echo Cleanup complete!
goto :eof