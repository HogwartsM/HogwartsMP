@echo off
REM Script de nettoyage du client HogwartsMP v2.0
REM Supprime les fichiers obsolètes

echo ========================================
echo HogwartsMP v2.0 - Client Cleanup
echo ========================================
echo.

echo [INFO] Suppression des fichiers obsoletes...
echo.

REM Core (obsolète - remplacé par client_main.cpp)
if exist "code\client\src\core\client_instance.cpp" (
    del /Q "code\client\src\core\client_instance.cpp"
    echo [OK] Supprime: core/client_instance.cpp
)
if exist "code\client\src\core\client_instance.h" (
    del /Q "code\client\src\core\client_instance.h"
    echo [OK] Supprime: core/client_instance.h
)

REM Services (obsolète - remplacé par NetLibrary)
if exist "code\client\src\Services\network_client.cpp" (
    del /Q "code\client\src\Services\network_client.cpp"
    echo [OK] Supprime: Services/network_client.cpp
)
if exist "code\client\src\Services\network_client.h" (
    del /Q "code\client\src\Services\network_client.h"
    echo [OK] Supprime: Services/network_client.h
)

REM UI (supprimé - debug commands à la place)
if exist "code\client\src\UI\chat.cpp" del /Q "code\client\src\UI\chat.cpp"
if exist "code\client\src\UI\chat.h" del /Q "code\client\src\UI\chat.h"
if exist "code\client\src\UI\console.cpp" del /Q "code\client\src\UI\console.cpp"
if exist "code\client\src\UI\console.h" del /Q "code\client\src\UI\console.h"
if exist "code\client\src\UI\season_manager.cpp" del /Q "code\client\src\UI\season_manager.cpp"
if exist "code\client\src\UI\season_manager.h" del /Q "code\client\src\UI\season_manager.h"
if exist "code\client\src\UI\teleport_manager.cpp" del /Q "code\client\src\UI\teleport_manager.cpp"
if exist "code\client\src\UI\teleport_manager.h" del /Q "code\client\src\UI\teleport_manager.h"
if exist "code\client\src\UI\ui_base.cpp" del /Q "code\client\src\UI\ui_base.cpp"
if exist "code\client\src\UI\ui_base.h" del /Q "code\client\src\UI\ui_base.h"
echo [OK] Supprime: UI/* (10 fichiers)

REM Main.cpp obsolète (remplacé par client_main.cpp)
if exist "code\client\src\main.cpp" (
    del /Q "code\client\src\main.cpp"
    echo [OK] Supprime: main.cpp
)

echo.
echo [INFO] Suppression des dossiers vides...

REM Suppression des dossiers vides
if exist "code\client\src\Core" rmdir /Q "code\client\src\Core" 2>nul
if exist "code\client\src\Services" rmdir /Q "code\client\src\Services" 2>nul
if exist "code\client\src\UI" rmdir /Q "code\client\src\UI" 2>nul
echo [OK] Dossiers vides supprimes

echo.
echo ========================================
echo Nettoyage termine!
echo ========================================
echo.
echo Fichiers restants dans code/client/src:
dir /B code\client\src
echo.
echo Doit afficher: client_main.cpp et sdk/
echo.
pause
