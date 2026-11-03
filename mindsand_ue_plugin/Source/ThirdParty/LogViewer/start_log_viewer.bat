@echo off
REM Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
REM SPDX-License-Identifier: BSD-3-Clause-Clear

@echo off
setlocal
cd /d "%~dp0"

if not exist ".venv\Scripts\python.exe" (
    echo Python virtual environment not found.
    echo Create it with: py -m venv .venv
    echo Then install dependencies with: .venv\Scripts\python.exe -m pip install -r requirements.txt
    pause
    exit /b 1
)

start "MindSand Viewer Server" /b ".venv\Scripts\python.exe" -m uvicorn backend.main:app --host 127.0.0.1 --port 8000

timeout /t 2 /nobreak >nul
start "" "http://127.0.0.1:8000/"

endlocal
