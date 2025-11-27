@echo off

:: EGM Emulator Build Script for WSL/Docker
:: Requires C:\.config\wsl.ini configuration file

:: Read configuration from wsl.ini
set "CONFIG_FILE=C:\.config\wsl.ini"

:: Check if config file exists
if not exist "%CONFIG_FILE%" (
    echo Error: Configuration file "%CONFIG_FILE%" not found.
    echo.
    echo Please create C:\.config\wsl.ini with the following content:
    echo.
    type "%~dp0wsl_config.ini.template"
    echo.
    echo See wsl_config.ini.template for reference.
    exit /b 1
)

:: Initialize variables
set "EGM_SOURCE_BASE="
set "DEST_BASE="
set "WSL_BASE="
set "UBUNTU_VERSION="

:: Read config file
for /f "usebackq tokens=1,2 delims==" %%a in ("%CONFIG_FILE%") do (
    set "LINE=%%a"
    if not "!LINE:~0,1!"=="#" (
        if "%%a"=="EGM_SOURCE_BASE" set "EGM_SOURCE_BASE=%%b"
        if "%%a"=="DEST_BASE" set "DEST_BASE=%%b"
        if "%%a"=="WSL_BASE" set "WSL_BASE=%%b"
        if "%%a"=="UBUNTU_VERSION" set "UBUNTU_VERSION=%%b"
    )
)

:: Validate required variables
if not defined EGM_SOURCE_BASE (
    echo Error: EGM_SOURCE_BASE not defined in config file
    exit /b 1
)
if not defined DEST_BASE (
    echo Error: DEST_BASE not defined in config file
    exit /b 1
)
if not defined WSL_BASE (
    echo Error: WSL_BASE not defined in config file
    exit /b 1
)
if not defined UBUNTU_VERSION (
    echo Error: UBUNTU_VERSION not defined in config file
    exit /b 1
)

:: Handle command-line arguments
:process_args
if "%1"=="" goto :continue_processing

if "%1"=="--docker-target" (
    set "DOCKER_SYSTEM_TARGET=%2"
    shift
    shift
    goto :process_args
)

if "%1"=="--build-option" (
    set "BUILD_OPTION=%2"
    shift
    shift
    goto :process_args
)

:continue_processing

:: Set defaults
if not defined DOCKER_SYSTEM_TARGET set "DOCKER_SYSTEM_TARGET=yocto"
if not defined BUILD_OPTION set "BUILD_OPTION=sentinel"

:: Get current directory
set "SOLUTION_DIR=%CD%"

:: Remove trailing backslash if present
if "%SOLUTION_DIR:~-1%"=="\" set "SOLUTION_DIR=%SOLUTION_DIR:~0,-1%"

:: Calculate relative path from EGM_SOURCE_BASE
setlocal enabledelayedexpansion
set "SOLUTION_DIR_WSL=!SOLUTION_DIR:%EGM_SOURCE_BASE%=!"

set "SOLUTION_DIR_WSL=!SOLUTION_DIR_WSL:\=/!"
setlocal disabledelayedexpansion

:: Build WSL paths
set "WSL_DEST_WIN=%DEST_BASE%%SOLUTION_DIR_WSL%"
set "WSL_DEST=%WSL_BASE%%SOLUTION_DIR_WSL%"
set "WSL_DEST=%WSL_DEST:\=/%"

:: Display configuration
echo ========================================
echo EGM Emulator Build Configuration
echo ========================================
echo Docker Target:   %DOCKER_SYSTEM_TARGET%
echo Build Option:    %BUILD_OPTION%
echo Source Base:     %EGM_SOURCE_BASE%
echo Current Dir:     %SOLUTION_DIR%
echo Relative Path:   %SOLUTION_DIR_WSL%
echo WSL Dest (Win):  %WSL_DEST_WIN%
echo WSL Dest (Linux): %WSL_DEST%
echo ========================================
echo.

:: Check if WSL %UBUNTU_VERSION% is running, start it if not
echo Checking if WSL %UBUNTU_VERSION% is running...
wsl --list --running | findstr "%UBUNTU_VERSION%" >nul
if errorlevel 1 (
    echo WSL %UBUNTU_VERSION% is not running. Starting it now...
    wsl --distribution %UBUNTU_VERSION% --exec echo "WSL started successfully"
    if errorlevel 1 (
        echo Error: Failed to start WSL %UBUNTU_VERSION%. Please check your WSL installation.
        exit /b 1
    )
    echo WSL %UBUNTU_VERSION% is now running.
) else (
    echo WSL %UBUNTU_VERSION% is already running.
)

:: Check if Docker daemon is running in WSL, start it if not
echo Checking Docker daemon status in WSL...
wsl --distribution %UBUNTU_VERSION% --exec bash -c "docker info >/dev/null 2>&1"
if errorlevel 1 (
    echo Docker daemon is not running. Starting Docker service...
    wsl --distribution %UBUNTU_VERSION% --exec bash -c "sudo service docker start"
    if errorlevel 1 (
        echo Error: Failed to start Docker daemon. Please check Docker installation in WSL.
        exit /b 1
    )
    echo Docker daemon started successfully.
) else (
    echo Docker daemon is already running.
)

:: Clean up any existing build containers to prevent conflicts
echo Cleaning up any existing Docker containers...
wsl --distribution %UBUNTU_VERSION% --exec bash -c "CONTAINERS=$(docker ps -a --filter 'name=egmemulator-build-with-docker' --format '{{.ID}}'); if [ -n \"$CONTAINERS\" ]; then echo 'Removing containers...'; echo \"$CONTAINERS\" | xargs -r docker rm -f 2>/dev/null || true; fi"

:: Ensure WSL destination directory exists
if not exist "%WSL_DEST_WIN%" mkdir "%WSL_DEST_WIN%"

:: Sync files to WSL (only build-related files)
echo.
echo Syncing files to WSL...
robocopy "%SOLUTION_DIR%" "%WSL_DEST_WIN%" /E /FFT /R:2 /W:5 /XC /XX /XO /LOG:copy.log /XD .vs build build_manual node_modules Release .git /XF *.o *.bak copy.log package.json package-lock.json *.code-workspace
if errorlevel 8 (
    echo Error: File sync failed!
    exit /b 1
)

:: Run the build
echo.
echo Launching build in WSL/Docker...
wsl --distribution %UBUNTU_VERSION% --exec bash -c "cd %WSL_DEST% && chmod +x *.sh && DOCKER_SYSTEM_TARGET=%DOCKER_SYSTEM_TARGET% ./build-with-docker.sh %BUILD_OPTION% 2>&1 | tee /tmp/egm_build_output.log"

:: Copy log file to Windows side for error checking
wsl --distribution %UBUNTU_VERSION% --exec bash -c "cp /tmp/egm_build_output.log '%WSL_DEST%/build_output.log'" 2>nul
copy "%WSL_DEST_WIN%\build_output.log" build_output.log >nul 2>&1

:: Check for Docker authentication errors and retry if needed
findstr /C:"Unable to find image" build_output.log >nul
if not errorlevel 1 goto auth_retry

findstr /C:"Unauthenticated request" build_output.log >nul
if not errorlevel 1 goto auth_retry

findstr /C:"artifactregistry.repositories.downloadArtifacts" build_output.log >nul
if not errorlevel 1 goto auth_retry

findstr /C:"docker-credential-gcloud" build_output.log >nul
if not errorlevel 1 goto auth_retry

goto end_auth_check

:auth_retry
echo Docker authentication error detected. Running gcloud authentication...
wsl --distribution %UBUNTU_VERSION% --exec bash -l -c "~/google-cloud-sdk/bin/gcloud auth login || /snap/bin/gcloud auth login || gcloud auth login"
wsl --distribution %UBUNTU_VERSION% --exec bash -l -c "~/google-cloud-sdk/bin/gcloud auth configure-docker us-west1-docker.pkg.dev || /snap/bin/gcloud auth configure-docker us-west1-docker.pkg.dev || gcloud auth configure-docker us-west1-docker.pkg.dev"
echo Adding gcloud to PATH and retrying build...
wsl --distribution %UBUNTU_VERSION% --exec bash -c "cd %WSL_DEST% && export PATH=$PATH:~/google-cloud-sdk/bin:/snap/bin && DOCKER_SYSTEM_TARGET=%DOCKER_SYSTEM_TARGET% ./build-with-docker.sh %BUILD_OPTION%"

:end_auth_check

:: Clean up log files
if exist build_output.log del build_output.log

:: Copy sentinel.img back to Windows (from build/dist/)
echo.
if exist "%WSL_DEST_WIN%\build\dist\sentinel.img" (
    echo Copying sentinel.img to Windows...
    copy "%WSL_DEST_WIN%\build\dist\sentinel.img" sentinel.img
    echo.
    echo ========================================
    echo Build SUCCESS!
    echo ========================================
    echo File: %CD%\sentinel.img
    dir sentinel.img | findstr /V "Volume Directory"
    echo.
    echo Next steps:
    echo 1. Copy sentinel.img to SD card root
    echo 2. Boot Zeus device
    echo 3. Check: tail -f /var/log/sentinel.log
    echo ========================================
) else (
    echo.
    echo ========================================
    echo Build FAILED - sentinel.img not found
    echo ========================================
    exit /b 1
)

:: Clean up
if exist copy.log del copy.log
