@REM @echo off
@REM setlocal

@REM :: make sure python is installed
@REM echo Checking if Python is installed...
@REM where python >nul 2>nul
@REM if %errorlevel% neq 0 (
@REM     echo Python not found. Downloading Python installer...
@REM     powershell -Command "(New-Object System.Net.WebClient).DownloadFile('https://www.python.org/ftp/python/3.12.2/python-3.12.2-amd64.exe', 'python_installer.exe')"

@REM     echo Installing Python...
@REM     start /wait python_installer.exe /quiet InstallAllUsers=1 PrependPath=1 Include_pip=1
@REM     del python_installer.exe
@REM ) else (
@REM     echo Python is already installed.
@REM )

@REM :: make sure pip is installed
@REM echo Updating pip...
@REM python -m ensurepip
@REM python -m pip install --upgrade pip

@REM :: install requests and serial
@REM echo Installing required Python packages...
@REM python -m pip install requests pyserial pytz python-dotenv


@REM :: change this to the command that is used to run the program on the actual Aqusens computer
@REM echo Running AqusensComm.py...
@REM "python3" "AqusensComms.py"

@REM endlocal


@echo off
setlocal

set VENV_DIR=venv

:: Check if Python 3 is installed
echo Checking if Python 3 is installed...
where python3 >nul 2>nul
if %errorlevel% neq 0 (
    echo Python 3 not found. Downloading Python installer...
    powershell -Command "(New-Object System.Net.WebClient).DownloadFile('https://www.python.org/ftp/python/3.12.2/python-3.12.2-amd64.exe', 'python_installer.exe')"

    echo Installing Python...
    start /wait python_installer.exe /quiet InstallAllUsers=1 PrependPath=1 Include_pip=1
    del python_installer.exe
) else (
    echo Python 3 is already installed.
)

:: Update pip
echo Updating pip...
python3 -m ensurepip
python3 -m pip install --upgrade pip


:: Create venv if it doesn't exist
if not exist %VENV_DIR%\Scripts\python.exe (
    echo Creating virtual environment in %VENV_DIR%...
    python3 -m venv %VENV_DIR%
)

:: Activate venv
call %VENV_DIR%\Scripts\activate.bat

:: Install required packages in venv
echo Installing required Python packages...
python -m pip install -r requirements.txt

:: Run your Python script using the venv's Python
echo Running AqusensComm.py...
python AqusensComm.py

endlocal
