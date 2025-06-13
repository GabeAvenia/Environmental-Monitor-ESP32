@echo off
REM Script to generate Doxygen documentation for the Environmental Monitor project

echo Generating Doxygen documentation for Environment Monitor...

REM Check if Doxygen is installed (by checking if doxygen command works)
where doxygen >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo Error: Doxygen is not installed or not in the PATH.
    echo Please install Doxygen from http://www.doxygen.nl/download.html
    echo and make sure it's added to your PATH environment variable.
    exit /b 1
)

REM Check if Doxyfile exists
if not exist "Doxyfile" (
    echo Error: Doxyfile not found in the current directory.
    echo Please place the Doxyfile in the project root directory.
    exit /b 1
)

REM Create docs directory if it doesn't exist
if not exist "docs" mkdir docs

REM Run Doxygen
echo Running Doxygen...
doxygen

if %ERRORLEVEL% EQU 0 (
    echo Documentation successfully generated!
    echo You can find the documentation in the docs/html directory.
    echo Opening documentation...
    
    REM Try to open the documentation in the default browser
    start "" "docs\html\index.html"
) else (
    echo Error: Failed to generate documentation.
    exit /b 1
)
