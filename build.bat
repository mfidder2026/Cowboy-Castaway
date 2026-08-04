@echo off
rem ============================================================
rem  build.bat - Cowboy op het eiland: compileren en starten
rem  Zet dit bestand in dezelfde map als main.c
rem ============================================================

rem --- Paden naar jouw installatie -----------------------------
set CC65_HOME=D:\dev\cc65
set CL65="%CC65_HOME%\cc65\bin\cl65.exe"
set VICE="D:\dev\cc65\vice\bin\x64sc.exe"
rem -------------------------------------------------------------

rem --- Backup stap: bij iedere build de laatste wijzigingen wegschrijven naar BACKUP\YYYY-MM-DD_HH-MM-SS.zip
if not exist BACKUP mkdir BACKUP
for /f "usebackq tokens=*" %%a in (`powershell -NoProfile -Command "Get-Date -Format 'yyyy-MM-dd_HH-mm-ss'"`) do set TS=%%a
set ZIPFILE=BACKUP\%TS%.zip
powershell -NoProfile -Command "Compress-Archive -Path '*.c','*.h','*.bat','*.js','*.json','*.md','*.s','*.sym','*.map','*.lbl' -DestinationPath '%ZIPFILE%' -Force"
echo [BACKUP] Broncode opgeslagen in %ZIPFILE%

echo [1/2] Compileren...
%CL65% -O -t c64 main.c cowboy_frames.c island_gfx.c intro_gfx.c anims.c -o cowboy.prg
if errorlevel 1 (
    echo.
    echo *** Compileren MISLUKT - emulator wordt niet gestart. ***
    pause
    exit /b 1
)

echo [2/2] Starten in VICE...
rem -autostart laadt en start de prg meteen
rem -pal      zorgt voor 50 Hz, waar de animatiesnelheid op is afgestemd
%VICE% -autostart cowboy.prg -pal

exit /b 0
