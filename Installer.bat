@echo off
setlocal

:: Check if Python is installed
python --version >nul 2>&1
if %ERRORLEVEL% equ 0 (
    echo Python is already installed.
    echo Installing requirements...
    goto InstallRequirements
) else (
    echo Python is not installed.
    echo Installing Python...
)

:: Install Python from the Dependencies folder
set "PYTHON_INSTALLER=Dependencies\python-installer.exe"
if exist "%PYTHON_INSTALLER%" (
    "%PYTHON_INSTALLER%" /quiet InstallAllUsers=1 PrependPath=1
    if %ERRORLEVEL% neq 0 (
        echo Error: Python installation successful.
        @REM pause
        @REM exit /b %ERRORLEVEL%
    )
) else (
    echo Error: Python installer not found in Dependencies folder.
    pause
    exit /b 1
)

:InstallRequirements
:: Install the requirements using pip
pip install -r Dependencies\requirements.txt
if %ERRORLEVEL% neq 0 (
    echo Error: Failed to install Python packages from requirements.txt.
    pause
    exit /b %ERRORLEVEL%
)

:InstallDrivers
:: Install the drivers from the drivers folder
echo Installing drivers...
set "DRIVERS_DIR=Dependencies\drivers"
set "DRIVER1=ftdiport.inf"
set "DRIVER2=ftdibus.inf"

if exist "%DRIVERS_DIR%\%DRIVER1%" (
    pnputil /add-driver "%DRIVERS_DIR%\%DRIVER1%" /install
    if %ERRORLEVEL% neq 0 (
        echo Error: Failed to install %DRIVER1%.
        @REM pause
        @REM exit /b %ERRORLEVEL%
    )
) else (
    echo Error: %DRIVER1% not found in drivers folder.
    pause
    exit /b 1
)

if exist "%DRIVERS_DIR%\%DRIVER2%" (
    pnputil /add-driver "%DRIVERS_DIR%\%DRIVER2%" /install
    if %ERRORLEVEL% neq 0 (
        echo Error: Failed to install %DRIVER2%.
        @REM pause
        @REM exit /b %ERRORLEVEL%
    )
) else (
    echo Error: %DRIVER2% not found in drivers folder.
    pause
    exit /b 1
)

echo Installation successful!
echo The following packages were installed:
pip freeze

pause
exit /b 0