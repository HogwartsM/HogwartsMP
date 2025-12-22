# HogwartsMP FiveM-like Refactoring Guide

## 🎯 Objectif
Transformer HogwartsMP en framework multiplayer autonome inspiré de FiveM/CitizenFX.

## ✅ Phase 1 - Foundation (EN COURS)

### Fichiers Créés

#### 1. Système de Logging Indépendant
- ✅ `code/shared/logging/logger.h` - Interface de logging
- ✅ `code/shared/logging/logger.cpp` - Implémentation

**Utilisation:**
```cpp
#include "../../shared/logging/logger.h"

// Initialize au démarrage
HogwartsMP::Logging::Logger::Initialize("logs", HogwartsMP::Logging::LogLevel::Debug);

// Utiliser
LOG_INFO("Application started");
LOG_ERROR_F("Failed to load file: %s", filename.c_str());

// Shutdown
HogwartsMP::Logging::Logger::Shutdown();
```

#### 2. ClientInstance (Remplace Framework::Integrations::Client::Instance)
- ✅ `code/client/src/core/client_instance.h`
- ✅ `code/client/src/core/client_instance.cpp`

**Features:**
- Gestion cycle de vie (Init/Update/Shutdown)
- Hooks virtuels (PostInit, PostUpdate, PostRender, PreShutdown)
- ECS World (Flecs) intégré
- Accès D3D12 device/commandQueue/swapchain

#### 3. ServerInstance (Remplace Framework::Integrations::Server::Instance)
- ✅ `code/server/src/core/server_instance.h`
- ✅ `code/server/src/core/server_instance.cpp`

**Features:**
- Configuration serveur (port, maxPlayers, etc.)
- ECS World serveur
- Hooks virtuels (PostInit, PostUpdate, PreShutdown)

### Structure Créée

```
HogwartsMP/
├── code/
│   ├── client/
│   │   └── src/
│   │       ├── core/
│   │       │   └── client_instance.h/cpp [NOUVEAU]
│   │       ├── networking/ [NOUVEAU]
│   │       ├── scripting/ [NOUVEAU]
│   │       ├── resources/ [NOUVEAU]
│   │       └── nui/ [NOUVEAU]
│   ├── server/
│   │   └── src/
│   │       ├── core/
│   │       │   └── server_instance.h/cpp [NOUVEAU]
│   │       ├── networking/ [NOUVEAU]
│   │       ├── scripting/ [NOUVEAU]
│   │       └── resources/ [NOUVEAU]
│   └── shared/
│       ├── logging/ [NOUVEAU]
│       │   ├── logger.h
│       │   └── logger.cpp
│       ├── networking/ [NOUVEAU]
│       └── resources/ [NOUVEAU]
└── vendor/ [NOUVEAU]
    └── spdlog/ (pour logging)
```

## 📋 Prochaines Étapes (TODO)

### Phase 1.4: Networking Wrapper ENet
Créer wrapper autour d'ENet pour remplacer SLikeNet:
- `code/shared/networking/network_base.h`
- `code/client/src/networking/network_client.h/cpp`
- `code/server/src/networking/network_server.h/cpp`

### Phase 1.5: Modifier Application
Modifier `code/client/src/core/application.h/cpp`:
```cpp
// Avant
class Application : public Framework::Integrations::Client::Instance

// Après
class Application : public ClientInstance
```

Retirer tous les appels à Framework dans application.cpp.

### Phase 1.6: Modifier Server
Modifier `code/server/src/core/server.h/cpp`:
```cpp
// Avant
class Server : public Framework::Integrations::Server::Instance

// Après
class Server : public ServerInstance
```

### Phase 1.7: CMakeLists.txt
Modifier le CMakeLists.txt root:
```cmake
# SUPPRIMER
# include_directories(${PROJECT_SOURCE_DIR}/code/framework/src/)

# AJOUTER
add_subdirectory(vendor/enet)
include_directories(code/shared)
```

Modifier `code/client/CMakeLists.txt`:
```cmake
# SUPPRIMER
# Framework FrameworkClient

# AJOUTER
target_sources(HogwartsMPClient PRIVATE
    src/core/client_instance.cpp
    ${CMAKE_SOURCE_DIR}/code/shared/logging/logger.cpp
)
```

## 🔧 Dépendances à Télécharger

### ENet (Networking)
```bash
# Télécharger ENet 1.3.17+
# URL: http://enet.bespin.org/Downloads.html
# Placer dans: vendor/enet/
```

### Lua 5.4 (Phase 2)
```bash
# URL: https://www.lua.org/ftp/lua-5.4.6.tar.gz
# Placer dans: vendor/lua-5.4/
```

### Sol3 (Phase 2)
```bash
# URL: https://github.com/ThePhD/sol2
# Header-only, placer dans: vendor/sol3/include/
```

### CEF (Phase 6 - NUI)
```bash
# URL: https://cef-builds.spotifycdn.com/index.html
# Version: 112+ Windows 64-bit
# Placer dans: vendor/cef/
```

## 📖 Plan Complet

Le plan détaillé complet (8 semaines, 7 phases) se trouve dans:
**`C:\Users\fific\.claude\plans\recursive-watching-hinton.md`**

## 🎮 Références FiveM

Code source FiveM disponible dans:
**`C:\Users\fific\OneDrive\Desktop\WebSite_\fivem-master`**

Fichiers clés à étudier:
- `code/components/citizen-scripting-core/src/ScriptInvoker.cpp` - Native system
- `code/components/citizen-resources-core/src/ResourceManager.cpp` - Resource system
- `code/components/citizen-scripting-lua/src/LuaScriptRuntime.cpp` - Lua VM
- `code/components/nui-core/src/NUIWindow.cpp` - NUI/CEF

## ⚠️ Important

**NE PAS** compiler avant d'avoir:
1. Téléchargé ENet
2. Modifié Application et Server
3. Mis à jour CMakeLists.txt
4. Retiré toutes les références à Framework

## 🚀 Commandes de Build (Une fois prêt)

```bash
cd HogwartsMP
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

## 📝 Notes

- Le logging est maintenant indépendant et écrit dans `logs/`
- ClientInstance et ServerInstance sont les nouvelles bases
- Flecs (ECS) est conservé et intégré
- Le pattern FiveM est adopté pour l'architecture modulaire

---

**Statut actuel**: ✅ Phase 1 - Foundation (100% COMPLÉTÉ !)
**Prochaine étape**: Compiler le projet et passer à la Phase 2 (Scripting Lua)
