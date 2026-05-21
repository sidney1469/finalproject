@echo off
REM ---------------------------------------------------------------------------
REM  Plane Tracker Dashboard launcher (Windows)
REM
REM  This script:
REM    1. Installs backend Python dependencies (in a local virtualenv)
REM    2. Installs frontend npm dependencies
REM    3. Optionally creates the PostgreSQL database and loads the schema
REM    4. Launches the backend (uvicorn) and frontend (vite) in new windows
REM ---------------------------------------------------------------------------

setlocal EnableDelayedExpansion

set "ROOT=%~dp0"
set "BACKEND=%ROOT%backend"
set "FRONTEND=%ROOT%frontend"
set "SCHEMA=%BACKEND%\app\schemas\flights.sql"

set "DB_NAME=wheat_weywot"
set "DB_USER=postgres"
set "DB_HOST=localhost"
set "DB_PORT=5432"

echo.
echo ============================================================
echo   Plane Tracker Dashboard - Setup ^& Launch
echo ============================================================
echo.

REM ---------------------------------------------------------------------------
REM 1. Backend dependencies
REM ---------------------------------------------------------------------------
echo [1/4] Setting up backend Python environment...
pushd "%BACKEND%" || (echo ERROR: backend folder not found at %BACKEND% & exit /b 1)

if not exist ".venv\Scripts\python.exe" (
    echo   - Creating virtualenv in .venv
    python -m venv .venv
    if errorlevel 1 (
        echo ERROR: failed to create virtualenv. Is Python 3.12+ installed and on PATH?
        popd
        exit /b 1
    )
)

echo   - Installing Python packages
call ".venv\Scripts\python.exe" -m pip install --upgrade pip >nul
if exist "requirements.txt" (
    call ".venv\Scripts\python.exe" -m pip install -r requirements.txt
) else (
    call ".venv\Scripts\python.exe" -m pip install fastapi uvicorn psycopg[binary] pyserial h3
)
if errorlevel 1 (
    echo ERROR: pip install failed.
    popd
    exit /b 1
)
popd

REM ---------------------------------------------------------------------------
REM 2. Frontend dependencies
REM ---------------------------------------------------------------------------
echo.
echo [2/4] Installing frontend npm dependencies...
pushd "%FRONTEND%" || (echo ERROR: frontend folder not found at %FRONTEND% & exit /b 1)
call npm install
if errorlevel 1 (
    echo ERROR: npm install failed.
    popd
    exit /b 1
)
popd

REM ---------------------------------------------------------------------------
REM 3. Database setup (optional)
REM ---------------------------------------------------------------------------
echo.
echo [3/4] PostgreSQL database setup
set /p DB_CREATE="    Create database '%DB_NAME%' and load schema now? (y/N): "
if /I "!DB_CREATE!"=="y" (
    where psql >nul 2>nul
    if errorlevel 1 (
        echo ERROR: 'psql' was not found on PATH. Install PostgreSQL or add its 'bin' folder to PATH, then re-run.
        exit /b 1
    )

    set /p DB_USER_IN="    Postgres user [%DB_USER%]: "
    if not "!DB_USER_IN!"=="" set "DB_USER=!DB_USER_IN!"

    set /p DB_HOST_IN="    Postgres host [%DB_HOST%]: "
    if not "!DB_HOST_IN!"=="" set "DB_HOST=!DB_HOST_IN!"

    set /p DB_PORT_IN="    Postgres port [%DB_PORT%]: "
    if not "!DB_PORT_IN!"=="" set "DB_PORT=!DB_PORT_IN!"

    echo     You will be prompted for the Postgres password by psql.
    echo     - Creating database '%DB_NAME%' (ignored if it already exists)...
    psql -U "!DB_USER!" -h "!DB_HOST!" -p "!DB_PORT!" -d postgres -v ON_ERROR_STOP=0 -c "CREATE DATABASE %DB_NAME%;"

    if not exist "%SCHEMA%" (
        echo ERROR: schema file not found at %SCHEMA%
        exit /b 1
    )

    echo     - Applying schema from %SCHEMA% ...
    psql -U "!DB_USER!" -h "!DB_HOST!" -p "!DB_PORT!" -d "%DB_NAME%" -v ON_ERROR_STOP=1 -f "%SCHEMA%"
    if errorlevel 1 (
        echo ERROR: failed to apply schema.
        exit /b 1
    )
    echo     Database is ready.
) else (
    echo     Skipping database setup.
)

REM ---------------------------------------------------------------------------
REM 4. Launch backend and frontend
REM ---------------------------------------------------------------------------
echo.
echo [4/4] Starting backend and frontend in separate windows...

start "Plane Tracker Backend" cmd /k "cd /d "%BACKEND%" && call .venv\Scripts\activate.bat && python -m uvicorn app.main:app --host localhost --port 8000 --reload"

start "Plane Tracker Frontend" cmd /k "cd /d "%FRONTEND%" && npm run dev"

echo.
echo Done. Backend: http://localhost:8000    Frontend: http://localhost:5173
echo Close the spawned windows to stop the servers.
endlocal
