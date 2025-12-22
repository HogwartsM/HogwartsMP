@echo off
mkdir build
cd build
cmake -G "MinGW Makefiles" ..
mingw32-make -j8
copy /Y code\client\libHogwartsMPClient.dll code\launcher\
cd ..