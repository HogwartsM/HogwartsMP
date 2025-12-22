# HogwartsMP - Multiplayer for Hogwarts Legacy

FiveM-style multiplayer framework for Hogwarts Legacy.

## 🚀 Features

- ✅ Server/Client architecture with ENet networking
- ✅ DLL injection launcher with Denuvo bypass
- ✅ ECS-based entity system (Flecs)
- 🚧 Lua scripting support (coming soon)
- 🚧 Resource system (coming soon)
- 🚧 Natives API (coming soon)

## 📋 Requirements

### To Use (Pre-compiled)
- Windows 10/11 (64-bit)
- Hogwarts Legacy (Steam version)
- [Latest release](https://github.com/Akitium/HogwartsM/releases)

### To Compile from Source
- Windows 10/11 (64-bit)
- [MinGW-w64](https://www.mingw-w64.org/) (GCC 15.2.0 or later)
- [CMake](https://cmake.org/download/) (3.10 or later)
- Git

## 🔧 Installation (Pre-compiled)

1. Download the latest release ZIP from [Releases](https://github.com/Akitium/HogwartsM/releases)
2. Extract to a folder of your choice
3. You're ready to use!

## 🎮 Usage

### Running the Server

```cmd
cd server
HogwartsMPServer.exe
```

The server will start on port `27015` by default.

### Playing (Client)

```cmd
cd launcher
HogwartsMPLauncher.exe
```

The launcher will:
1. Ask for your Hogwarts Legacy installation path (if not auto-detected)
2. Launch the game with HogwartsMP injected
3. A MessageBox will appear when the mod is loaded
4. A console window will show client logs

**Note**: Make sure the server is running before launching the client!

## 🛠️ Compiling from Source

### 1. Install Dependencies

**MinGW-w64**:
```cmd
# Download from: https://github.com/niXman/mingw-builds-binaries/releases
# Extract to C:\mingw64
# Add C:\mingw64\bin to PATH
```

**CMake**:
```cmd
# Download from: https://cmake.org/download/
# Install and add to PATH
```

### 2. Clone Repository

```cmd
git clone --recursive https://github.com/Akitium/HogwartsM.git
cd HogwartsM
```

**Important**: Use `--recursive` to automatically download vendor dependencies (glm, minhook).

If you already cloned without `--recursive`, initialize submodules manually:
```cmd
git submodule update --init --recursive
```

### 3. Compile

```cmd
mkdir build
cd build
cmake -G "MinGW Makefiles" ..
mingw32-make -j8
```

### 4. Copy Dependencies

```cmd
copy C:\mingw64\bin\libgcc_s_seh-1.dll code\client\
copy "C:\mingw64\bin\libstdc++-6.dll" code\client\
copy C:\mingw64\bin\libwinpthread-1.dll code\client\
copy code\client\libHogwartsMPClient.dll code\launcher\
copy code\client\libgcc_s_seh-1.dll code\launcher\
copy "code\client\libstdc++-6.dll" code\launcher\
copy code\client\libwinpthread-1.dll code\launcher\
```

### 5. Run

**Server**:
```cmd
cd build\code\server
HogwartsMPServer.exe
```

**Client**:
```cmd
cd build\code\launcher
HogwartsMPLauncher.exe
```

## 📁 Project Structure

```
HogwartsMP/
├── code/
│   ├── client/          # Client DLL (injected into game)
│   ├── server/          # Standalone server
│   ├── launcher/        # Launcher (injects client DLL)
│   └── shared/          # Shared code (networking, logging)
├── vendor/              # Third-party libraries
│   ├── enet/           # Networking
│   ├── flecs/          # ECS
│   ├── glm/            # Math
│   └── minhook/        # Hooking
└── build/              # Compiled binaries (not in repo)
```

## 🐛 Troubleshooting

### "libstdc++-6.dll not found"
Make sure you copied all MinGW DLLs to the launcher folder.

### "Could not find Hogwarts Legacy installation"
The launcher will prompt you to enter the path manually. Example:
```
C:\Program Files (x86)\Steam\steamapps\common\Hogwarts Legacy\Phoenix\Binaries\Win64\HogwartsLegacy.exe
```

### "DLL injection failed"
- Make sure you're running the launcher as Administrator
- Antivirus might block the injection - add an exception
- Steam overlay might interfere - try disabling it

### Compilation errors
Make sure MinGW bin folder is in PATH:
```cmd
where gcc
# Should output: C:\mingw64\bin\gcc.exe
```

### "vendor/glm or vendor/minhook is empty"
You forgot to initialize Git submodules. Run:
```cmd
git submodule update --init --recursive
```

## 🗺️ Roadmap

- [x] Server/Client networking
- [x] Launcher with Denuvo bypass
- [ ] Lua scripting engine (Phase 2)
- [ ] Natives API with game offsets
- [ ] Resource system (fxmanifest.lua)
- [ ] Event system (AddEventHandler, TriggerEvent)
- [ ] Sync system (player position, actions)

## 📄 License

This project is for educational purposes only.

## 👥 Contributors

- [Akitium](https://github.com/Akitium) - Project Lead
- Claude Sonnet 4.5 - AI Assistant

## 🔗 Links

- [GitHub Repository](https://github.com/Akitium/HogwartsM)
- [Issues](https://github.com/Akitium/HogwartsM/issues)
- [Releases](https://github.com/Akitium/HogwartsM/releases)

---

**Disclaimer**: This is an unofficial multiplayer mod. Use at your own risk.
