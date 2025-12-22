@echo off
echo === HogwartsMP Release Packager ===
echo.

set VERSION=v0.0.1

echo Creating release directory...
mkdir release 2>nul
cd release
mkdir HogwartsMP-%VERSION% 2>nul
cd HogwartsMP-%VERSION%

echo Copying server...
mkdir server
copy ..\..\build\code\server\HogwartsMPServer.exe server\

echo Copying launcher...
mkdir launcher
copy ..\..\build\code\launcher\HogwartsMPLauncher.exe launcher\
copy ..\..\build\code\launcher\libHogwartsMPClient.dll launcher\
copy ..\..\build\code\launcher\libgcc_s_seh-1.dll launcher\
copy ..\..\build\code\launcher\libstdc++-6.dll launcher\
copy ..\..\build\code\launcher\libwinpthread-1.dll launcher\

echo Creating README...
echo HogwartsMP v%VERSION% > README.txt
echo. >> README.txt
echo Installation: >> README.txt
echo 1. Extract this archive >> README.txt
echo 2. Run server/HogwartsMPServer.exe to start the server >> README.txt
echo 3. Run launcher/HogwartsMPLauncher.exe to play >> README.txt
echo. >> README.txt
echo The launcher will ask for your Hogwarts Legacy installation path. >> README.txt

cd ..

echo Creating ZIP archive...
powershell Compress-Archive -Path "HogwartsMP-%VERSION%" -DestinationPath "HogwartsMP-%VERSION%.zip" -Force

echo.
echo === Package created: release\HogwartsMP-%VERSION%.zip ===
echo.
echo Upload this ZIP to GitHub Releases!
pause
