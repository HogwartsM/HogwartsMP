@echo off
setlocal

:: Définir les chemins
set BUILD_DIR=%~dp0build
set SERVER_EXE=%BUILD_DIR%\code\server\HogwartsMPServer.exe
set LAUNCHER_EXE=%BUILD_DIR%\code\launcher\HogwartsMPLauncher.exe

:: Vérifier si le build existe
if not exist "%BUILD_DIR%" (
    echo [ERREUR] Le dossier 'build' est introuvable. Veuillez lancer 'build.cmd' d'abord.
    exit /b 1
)

:: Traiter les arguments
if "%1"=="--server" goto :start_server
if "%1"=="--launcher" goto :start_launcher
if "%1"=="--help" goto :help
if "%1"=="" goto :help

:start_server
echo [INFO] Démarrage du serveur HogwartsMP...
if not exist "%SERVER_EXE%" (
    echo [ERREUR] L'exécutable du serveur est introuvable : %SERVER_EXE%
    exit /b 1
)
"%SERVER_EXE%"
goto :eof

:start_launcher
echo [INFO] Démarrage du launcher HogwartsMP...
if not exist "%LAUNCHER_EXE%" (
    echo [ERREUR] L'exécutable du launcher est introuvable : %LAUNCHER_EXE%
    exit /b 1
)
"%LAUNCHER_EXE%"
goto :eof

:help
echo Utilisation : start.cmd [OPTION]
echo.
echo Options :
echo   --server    Démarrer le serveur dédié
echo   --launcher  Démarrer le launcher client
echo   --help      Afficher cette aide
goto :eof
