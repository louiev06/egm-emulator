@echo off

:: Read configuration from wsl_config.ini
set "CONFIG_FILE=C:\.config\wsl.ini"

:: Check if the config file exists
if not exist "%CONFIG_FILE%" (
    echo Error: Configuration file "%CONFIG_FILE%" not found.
    exit /b 1
)

:: Initialize variables
set "UBUNTU_VERSION="

:: Read the config file
for /f "usebackq delims== tokens=1,2" %%a in ("%CONFIG_FILE%") do (
    if "%%a"=="UBUNTU_VERSION" set "UBUNTU_VERSION=%%b"
)

:: Check if UBUNTU_VERSION is defined
if not defined UBUNTU_VERSION (
    echo Error: UBUNTU_VERSION not defined in "%CONFIG_FILE%".
    echo Sample value below:
    echo UBUNTU_VERSION=Ubuntu-20.04
    exit /b 1
)

:: Run gcloud auth login (same as nCompass wsl_build.bat --gcloud-login)
echo Launching gcloud auth login in WSL...
wsl --distribution %UBUNTU_VERSION% --exec bash -l -c "~/google-cloud-sdk/bin/gcloud auth login || /snap/bin/gcloud auth login || gcloud auth login"
exit /b %errorlevel%
