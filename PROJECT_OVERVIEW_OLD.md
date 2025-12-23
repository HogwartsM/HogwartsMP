# 🧙 HogwartsMP - Vue d'ensemble complète du projet

**Document technique destiné aux développeurs**
**Version : 0.0.3**
**Date : Décembre 2024**

---

## 📑 Table des matières

1. [Vue d'ensemble](#-vue-densemble)
2. [Architecture globale](#-architecture-globale)
3. [Composants principaux](#-composants-principaux)
4. [Framework FiveM-style](#-framework-fivem-style)
5. [Structure des dossiers](#-structure-des-dossiers)
6. [Flux de données](#-flux-de-données)
7. [Système de build](#-système-de-build)
8. [Offsets et Memory Management](#-offsets-et-memory-management)
9. [Networking](#-networking)
10. [État actuel et roadmap](#-état-actuel-et-roadmap)

---

## 🎯 Vue d'ensemble

### Qu'est-ce que HogwartsMP ?

**HogwartsMP** est un mod multijoueur pour **Hogwarts Legacy** (jeu solo basé sur Unreal Engine 4). Le projet transforme le jeu en expérience multijoueur en utilisant :

- **DLL Injection** pour modifier le comportement du jeu client
- **Serveur dédié** pour gérer les joueurs et la synchronisation
- **Framework style FiveM** pour simplifier le développement de fonctionnalités

### Objectifs du projet

✅ Créer un multijoueur stable pour Hogwarts Legacy
✅ Fournir une API simple façon FiveM pour les développeurs
✅ Synchroniser les joueurs, NPCs, et le monde
✅ Permettre l'extension via des scripts/mods

### Technologies utilisées

- **Langage** : C++17
- **Build System** : CMake 3.20+
- **Compilateur** : MinGW-w64 (GCC)
- **Networking** : ENet (UDP)
- **Entity System** : Flecs (ECS)
- **Math Library** : GLM
- **Hooking** : MinHook
- **UI** : ImGui (pour debug/overlay)

---

## 🏗 Architecture globale

### Schéma simplifié

```
┌─────────────────────────────────────────────────────────────┐
│                    Hogwarts Legacy.exe                       │
│                   (Unreal Engine 4 Game)                     │
│                                                               │
│  ┌────────────────────────────────────────────────────┐     │
│  │         HogwartsMPClient.dll (Injecté)             │     │
│  │                                                      │     │
│  │  ┌──────────────┐        ┌──────────────┐         │     │
│  │  │   Framework   │◄──────►│ Client Core  │         │     │
│  │  │   Natives     │        │   Instance   │         │     │
│  │  └──────────────┘        └──────┬───────┘         │     │
│  │                                  │                  │     │
│  │  ┌──────────────┐        ┌──────▼───────┐         │     │
│  │  │ Event System │◄──────►│   Network    │         │     │
│  │  └──────────────┘        │    Client    │         │     │
│  │                          └──────┬───────┘         │     │
│  └─────────────────────────────────┼─────────────────┘     │
└─────────────────────────────────────┼───────────────────────┘
                                      │
                              UDP (ENet)
                                      │
┌─────────────────────────────────────▼───────────────────────┐
│              HogwartsMPServer.exe                            │
│                (Serveur dédié)                               │
│                                                               │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │   Network    │  │    Flecs     │  │  Framework   │      │
│  │    Server    │  │     ECS      │  │   Natives    │      │
│  └──────┬───────┘  └──────┬───────┘  └──────────────┘      │
│         │                  │                                 │
│  ┌──────▼──────────────────▼───────┐                        │
│  │      Server Instance             │                        │
│  │  - Player Management             │                        │
│  │  - World State                   │                        │
│  │  - Module System                 │                        │
│  └──────────────────────────────────┘                        │
└───────────────────────────────────────────────────────────────┘

         ┌────────────────────────┐
         │ HogwartsMPLauncher.exe │
         │   (DLL Injector)       │
         └────────────────────────┘
```

### Flux d'exécution

1. **Launcher** lance Hogwarts Legacy et injecte `HogwartsMPClient.dll`
2. **Client DLL** s'initialise, hook le jeu, et se connecte au serveur
3. **Serveur** accepte la connexion et démarre la synchronisation
4. **Communication bidirectionnelle** via ENet (UDP)
5. **Framework** expose des API simples pour manipuler le jeu

---

## 🔧 Composants principaux

### 1. **HogwartsMPClient.dll** (Client)

**Rôle** : S'injecte dans le processus du jeu pour le modifier

**Fichiers principaux** :
- `code/client/src/main.cpp` - Point d'entrée DLL (DllMain)
- `code/client/src/Core/client_instance.h/cpp` - Gestion du cycle de vie
- `code/client/src/Services/network_client.h/cpp` - Connexion réseau
- `code/client/src/UI/*` - Interfaces utilisateur (chat, console, debug)
- `code/client/src/sdk/*` - **1265+ headers** du SDK Unreal Engine

**Fonctionnalités** :
- Hook des fonctions du jeu (D3D11, Input, GameLoop)
- Lecture/écriture de la mémoire du jeu
- Synchronisation avec le serveur
- Overlay UI (ImGui)
- Gestion des événements locaux

### 2. **HogwartsMPServer.exe** (Serveur)

**Rôle** : Serveur dédié gérant tous les clients

**Fichiers principaux** :
- `code/server/src/main.cpp` - Point d'entrée
- `code/server/src/core/server.h/cpp` - Classe serveur principale
- `code/server/src/core/server_instance.h/cpp` - Base du serveur
- `code/server/src/networking/network_server.h/cpp` - Gestion réseau
- `code/server/src/core/modules/human.h/cpp` - Module joueur/humain

**Fonctionnalités** :
- Gestion multi-clients (connexion/déconnexion)
- Synchronisation des entités (Flecs ECS)
- Système de commandes CLI
- Broadcast de messages
- Gestion de l'état du monde

### 3. **HogwartsMPLauncher.exe** (Launcher)

**Rôle** : Injecte la DLL dans le processus du jeu

**Fichiers principaux** :
- `code/launcher/src/main.cpp` - Point d'entrée
- `code/launcher/src/manual_map.h/cpp` - Injection manuelle (manual mapping)

**Fonctionnalités** :
- Détection du processus Hogwarts Legacy
- Injection de `HogwartsMPClient.dll`
- Logging de l'injection

### 4. **Code partagé** (Shared)

**Rôle** : Code commun entre client et serveur

**Fichiers principaux** :
- `code/shared/logging/logger.h/cpp` - Système de logs unifié
- `code/shared/networking/network_packet.h/cpp` - Sérialisation des paquets
- `code/shared/messages/*` - Définitions des messages réseau
- `code/shared/rpc/*` - RPC (Remote Procedure Calls)

**Fonctionnalités** :
- Logging avec rotation de fichiers
- Paquets réseau typés
- Messages de synchronisation (spawn, despawn, update)

---

## 🚀 Framework FiveM-style

### Pourquoi un framework ?

Le développement initial manipulait directement la mémoire du jeu, ce qui était :
- ❌ Complexe et verbeux
- ❌ Sujet aux erreurs
- ❌ Difficile à maintenir

**Solution** : Créer un framework avec des **Natives API** comme FiveM

### Structure du Framework

```
framework/
├── memory/                    # Gestion de la mémoire
│   ├── PatternScanner.h/cpp   # Recherche de patterns en mémoire
│   └── GameOffsets.h/cpp      # Gestionnaire d'offsets
│
├── events/                    # Système d'événements
│   └── EventManager.h/cpp     # RegisterEventHandler, TriggerEvent
│
├── natives/                   # API Natives
│   ├── ClientNatives.h/cpp    # GetPlayerCoords(), TeleportPlayer()...
│   └── ServerNatives.h/cpp    # GetPlayerCount(), BroadcastMessage()...
│
├── examples/                  # Exemples d'utilisation
│   ├── client_example.cpp
│   └── server_example.cpp
│
├── CMakeLists.txt            # Build du framework
├── README.md                 # Documentation complète
├── INTEGRATION.md            # Guide d'intégration
└── QUICKSTART.md             # Démarrage rapide
```

### Exemples d'API

#### Client Natives

```cpp
#include <framework/natives/include/ClientNatives.h>
using namespace HogwartsMP::Framework::Natives;

// Récupérer la position du joueur
FVector pos = GetPlayerCoords();
// Résultat: pos.X, pos.Y, pos.Z

// Téléporter le joueur
TeleportPlayer(1000.0f, 2000.0f, 100.0f);

// Changer le heading
SetPlayerHeading(180.0f);

// Vérifier si dans une zone
bool inZone = IsPlayerInArea(centerPos, radius);
```

#### Server Natives

```cpp
#include <framework/natives/include/ServerNatives.h>
using namespace HogwartsMP::Framework::Natives;

// Nombre de joueurs
int count = GetPlayerCount();

// Broadcast un message
BroadcastMessage("Bienvenue sur HogwartsMP!");

// Kick un joueur
KickPlayer(playerId, "AFK");

// Changer la météo
SetWeather("rainy");
```

#### Système d'événements

```cpp
#include <framework/events/include/EventManager.h>
using namespace HogwartsMP::Framework::Events;

// Enregistrer un handler
EventManager::Get().RegisterEventHandler("playerSpawned", [](const EventArgs& args) {
    TeleportPlayer(0, 0, 100);
});

// Déclencher un événement
EventManager::Get().TriggerEvent("playerSpawned");

// Client → Serveur
EventManager::Get().TriggerServerEvent("requestWeapon", {weaponName});

// Serveur → Client
EventManager::Get().TriggerClientEvent(playerId, "giveWeapon", {weaponName});
```

### Avantages du Framework

✅ **Simple** : `GetPlayerCoords()` au lieu de manipuler la mémoire
✅ **Type-safe** : Types Unreal Engine intégrés
✅ **Extensible** : Facile d'ajouter de nouvelles natives
✅ **Documenté** : README, guides, exemples complets
✅ **Testé** : Exemples fonctionnels fournis

---

## 📁 Structure des dossiers

### Vue complète

```
HogwartsMP/
│
├── code/                                # Code source principal
│   ├── client/                          # Client (DLL injectée)
│   │   ├── src/
│   │   │   ├── main.cpp                 # DllMain
│   │   │   ├── Core/
│   │   │   │   └── client_instance.cpp
│   │   │   ├── Services/
│   │   │   │   └── network_client.cpp
│   │   │   ├── UI/
│   │   │   │   ├── chat.cpp
│   │   │   │   ├── console.cpp
│   │   │   │   └── teleport_manager.cpp
│   │   │   └── sdk/                     # 1265+ fichiers SDK UE4
│   │   │       ├── containers/
│   │   │       │   ├── fvector.h
│   │   │       │   ├── frotator.h
│   │   │       │   └── tarray.h
│   │   │       ├── entities/
│   │   │       │   ├── aactor.h
│   │   │       │   ├── apawn.h
│   │   │       │   └── aplayercontroller.h
│   │   │       ├── components/
│   │   │       ├── game/
│   │   │       └── Runtime/
│   │   └── CMakeLists.txt
│   │
│   ├── server/                          # Serveur dédié
│   │   ├── src/
│   │   │   ├── main.cpp
│   │   │   ├── core/
│   │   │   │   ├── server.cpp
│   │   │   │   ├── server_instance.cpp
│   │   │   │   ├── cli.cpp
│   │   │   │   └── modules/
│   │   │   │       └── human.cpp
│   │   │   ├── networking/
│   │   │   │   └── network_server.cpp
│   │   │   └── logging/
│   │   │       └── network_logger.cpp
│   │   └── CMakeLists.txt
│   │
│   ├── launcher/                        # Launcher/Injector
│   │   ├── src/
│   │   │   ├── main.cpp
│   │   │   ├── manual_map.cpp
│   │   │   └── Logger.cpp
│   │   └── CMakeLists.txt
│   │
│   ├── shared/                          # Code partagé
│   │   ├── logging/
│   │   │   └── logger.cpp
│   │   ├── networking/
│   │   │   └── network_packet.cpp
│   │   ├── messages/
│   │   │   ├── messages.h
│   │   │   └── human/
│   │   │       ├── human_spawn.h
│   │   │       ├── human_despawn.h
│   │   │       └── human_update.h
│   │   └── rpc/
│   │       ├── chat_message.h
│   │       └── set_weather.h
│   │
│   └── CMakeLists.txt
│
├── framework/                           # Framework FiveM-style
│   ├── memory/
│   │   ├── include/
│   │   │   ├── PatternScanner.h
│   │   │   └── GameOffsets.h
│   │   └── src/
│   │       ├── PatternScanner.cpp
│   │       └── GameOffsets.cpp
│   ├── events/
│   │   ├── include/
│   │   │   └── EventManager.h
│   │   └── src/
│   │       └── EventManager.cpp
│   ├── natives/
│   │   ├── include/
│   │   │   ├── ClientNatives.h
│   │   │   └── ServerNatives.h
│   │   └── src/
│   │       ├── ClientNatives.cpp
│   │       └── ServerNatives.cpp
│   ├── examples/
│   │   ├── client_example.cpp
│   │   └── server_example.cpp
│   ├── CMakeLists.txt
│   ├── README.md
│   ├── INTEGRATION.md
│   └── QUICKSTART.md
│
├── vendor/                              # Dépendances tierces
│   ├── enet/                            # ENet (UDP networking)
│   ├── flecs/                           # Flecs (ECS)
│   ├── glm/                             # GLM (Math)
│   ├── minhook/                         # MinHook (API hooking)
│   └── imgui/                           # ImGui (UI)
│
├── build/                               # Artefacts de build (généré)
│   ├── code/
│   │   ├── client/
│   │   │   └── libHogwartsMPClient.dll
│   │   ├── server/
│   │   │   └── HogwartsMPServer.exe
│   │   └── launcher/
│   │       └── HogwartsMPLauncher.exe
│   └── ...
│
├── CMakeLists.txt                       # CMake racine
├── build.cmd                            # Script de build Windows
├── start.cmd                            # Script de lancement
├── README.md                            # Documentation du projet
└── LICENSE                              # Licence du projet
```

---

## 🔄 Flux de données

### 1. Démarrage du système

```
1. Utilisateur lance: start.cmd --launcher
   ↓
2. HogwartsMPLauncher.exe démarre
   ↓
3. Launcher injecte HogwartsMPClient.dll dans HogwartsLegacy.exe
   ↓
4. DllMain() s'exécute
   ↓
5. ClientThread() démarre:
   - Initialise GameOffsets (pattern scanning)
   - Initialise ClientInstance
   - Se connecte au serveur (NetworkClient)
   ↓
6. Serveur accepte la connexion
   ↓
7. Synchronisation commence
```

### 2. Communication Client-Serveur

```
CLIENT                                  SERVEUR
  │                                       │
  │─────── Connect Request ─────────────►│
  │                                       │
  │◄────── Connect Accept ───────────────│
  │                                       │
  │─────── Player Join ──────────────────►│
  │                                       │ (Crée l'entité joueur)
  │◄────── Spawn Confirmation ───────────│
  │                                       │
  │─────── Position Update ──────────────►│
  │                                       │ (Broadcast aux autres)
  │◄────── Other Players Update ─────────│
  │                                       │
  │─────── Chat Message ─────────────────►│
  │                                       │ (Broadcast)
  │◄────── Chat Broadcast ───────────────│
  │                                       │
  │─────── Disconnect ───────────────────►│
  │                                       │ (Cleanup)
```

### 3. Gestion de la mémoire (Pattern Scanning)

```
Au démarrage du client:

1. GameOffsets::Initialize()
   ↓
2. Pour chaque pattern défini:
   - ScanPattern("HogwartsLegacy.exe", pattern)
   ↓
3. Pattern trouvé → Stocke l'adresse
   ↓
4. Natives utilisent ces adresses:
   GetPlayerCoords() {
       uintptr_t gEngine = GetModuleBase() + Offsets::GEngine;
       uintptr_t gameViewport = Read<uintptr_t>(gEngine + 0x10);
       uintptr_t world = Read<uintptr_t>(gameViewport + 0x10);
       // ... etc
   }
```

### 4. Événements réseau

```
CLIENT                                          SERVEUR

1. TriggerServerEvent("requestWeapon", {name})
   ↓
2. EventManager sérialise
   ↓
3. NetworkClient::Send(packet)
   ────────────────────────────────────────────►
                                                4. NetworkServer reçoit
                                                ↓
                                                5. Désérialise le packet
                                                ↓
                                                6. EventManager::TriggerEvent("requestWeapon")
                                                ↓
                                                7. Handler s'exécute
                                                ↓
                                                8. TriggerClientEvent(playerId, "giveWeapon", {name})
   ◄────────────────────────────────────────────
9. NetworkClient reçoit
   ↓
10. EventManager::TriggerEvent("giveWeapon")
   ↓
11. Handler donne l'arme au joueur
```

---

## 🛠 Système de build

### Commandes de build

```bash
# Build complet (Debug par défaut)
build.cmd

# Build en Release
mkdir build
cd build
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ..
mingw32-make -j8

# Copier la DLL client vers le launcher
copy /Y code\client\libHogwartsMPClient.dll code\launcher\
```

### Structure CMake

```
CMakeLists.txt (racine)
├── vendor/enet/CMakeLists.txt
├── vendor/flecs/CMakeLists.txt
├── vendor/glm/CMakeLists.txt
├── vendor/minhook/CMakeLists.txt
├── framework/CMakeLists.txt
│   ├── HogwartsMPMemory (STATIC)
│   ├── HogwartsMPEvents (STATIC)
│   ├── HogwartsMPNativesClient (STATIC)
│   ├── HogwartsMPNativesServer (STATIC)
│   ├── HogwartsMPFrameworkClient (INTERFACE)
│   └── HogwartsMPFrameworkServer (INTERFACE)
└── code/CMakeLists.txt
    ├── code/client/CMakeLists.txt
    │   └── HogwartsMPClient (SHARED) → libHogwartsMPClient.dll
    ├── code/server/CMakeLists.txt
    │   └── HogwartsMPServer (EXECUTABLE) → HogwartsMPServer.exe
    └── code/launcher/CMakeLists.txt
        └── HogwartsMPLauncher (EXECUTABLE) → HogwartsMPLauncher.exe
```

### Dépendances de compilation

```
HogwartsMPClient.dll dépend de:
- enet (UDP networking)
- minhook (API hooking)
- ws2_32 (Windows sockets)
- winmm (Windows multimedia)
- HogwartsMPFrameworkClient

HogwartsMPServer.exe dépend de:
- flecs (ECS)
- enet (UDP networking)
- HogwartsMPFrameworkServer

HogwartsMPLauncher.exe dépend de:
- psapi (Process API)
```

### Outputs de build

```
build/code/client/libHogwartsMPClient.dll    (~188 KB en Release)
build/code/server/HogwartsMPServer.exe       (~1.4 MB en Release)
build/code/launcher/HogwartsMPLauncher.exe   (~188 KB en Release)
```

---

## 🧠 Offsets et Memory Management

### Offsets actuels (Hogwarts Legacy)

```cpp
// Dans GameOffsets.cpp et ClientNatives.cpp
GEngine              = 0x99B1298    // Pointeur vers l'instance du moteur
GameViewport         = 0x10         // Offset GameViewport dans GEngine
World                = 0x10         // Offset World dans GameViewport
GameInstance         = 0x250        // Offset GameInstance dans World
LocalPlayers         = 0x30         // Offset LocalPlayers dans GameInstance
PlayerController     = 0x30         // Offset PlayerController dans LocalPlayer
AcknowledgedPawn     = 0x230        // Offset AcknowledgedPawn dans PlayerController
RootComponent        = 0x158        // Offset RootComponent dans Actor
RelativeLocation     = 0x1F0        // Offset RelativeLocation dans SceneComponent
RelativeRotation     = 0x1FC        // Offset RelativeRotation dans SceneComponent
```

### Chaîne d'accès au joueur

```cpp
// Pour récupérer le PlayerController:
uintptr_t gEngine = GetModuleBase() + 0x99B1298;
uintptr_t gEnginePtr = Read<uintptr_t>(gEngine);
uintptr_t gameViewport = Read<uintptr_t>(gEnginePtr + 0x10);
uintptr_t world = Read<uintptr_t>(gameViewport + 0x10);
uintptr_t gameInstance = Read<uintptr_t>(world + 0x250);
uintptr_t localPlayers = Read<uintptr_t>(gameInstance + 0x30);
uintptr_t playerController = Read<uintptr_t>(localPlayers + 0x30);

// Pour récupérer le Pawn:
uintptr_t pawn = Read<uintptr_t>(playerController + 0x230);

// Pour récupérer la position:
uintptr_t rootComponent = Read<uintptr_t>(pawn + 0x158);
FVector position = Read<FVector>(rootComponent + 0x1F0);
```

### Pattern Scanning

Le framework cherche automatiquement les offsets au démarrage :

```cpp
// Dans GameOffsets::InitializePatterns()
RegisterPattern("UWorld", "48 8B 05 ?? ?? ?? ?? 48 8B 88 ?? ?? ?? ?? 48 85 C9", 3);
RegisterPattern("GNames", "48 8B 1D ?? ?? ?? ?? 48 85 DB 75 ?? E8", 3);
RegisterPattern("GObjects", "48 8B 0D ?? ?? ?? ?? 48 8D 14 40", 3);
```

**Note** : Ces patterns peuvent changer selon les mises à jour du jeu. Si le jeu est patché, il faut mettre à jour les patterns.

---

## 🌐 Networking

### Architecture réseau

**Protocole** : UDP via ENet
**Port par défaut** : 7777
**Max clients** : Configurable (32 par défaut)

### Types de paquets

```cpp
enum class PacketType : uint8_t {
    Connect = 0,
    Disconnect = 1,
    PlayerJoin = 2,
    PlayerLeave = 3,
    PlayerUpdate = 4,
    Event = 5,
    ResourceList = 6,
    ResourceRequest = 7,
    ResourceManifest = 8,
    ResourceFile = 9,
    ResourceComplete = 10,
    Chat = 11,
    RPC = 12,
    Custom = 255
};
```

### Messages de synchronisation

#### Human (Joueur/NPC)

```cpp
// human_spawn.h
struct HumanSpawnMessage {
    uint32_t entityId;
    FVector position;
    FRotator rotation;
    std::string modelName;
};

// human_update.h
struct HumanUpdateMessage {
    uint32_t entityId;
    FVector position;
    FRotator rotation;
    FVector velocity;
};
```

### RPC (Remote Procedure Calls)

```cpp
// chat_message.h
struct ChatMessageRPC {
    std::string sender;
    std::string message;
    uint32_t timestamp;
};

// set_weather.h
struct SetWeatherRPC {
    std::string weatherType;  // "sunny", "rainy", "cloudy", etc.
};
```

### Sérialisation

Les paquets utilisent une classe de base `NetworkPacket` :

```cpp
class NetworkPacket {
public:
    virtual void Serialize(std::vector<uint8_t>& buffer) const = 0;
    virtual void Deserialize(const uint8_t* data, size_t size) = 0;

protected:
    // Helpers
    void WriteUInt8(std::vector<uint8_t>& buffer, uint8_t value);
    void WriteUInt32(std::vector<uint8_t>& buffer, uint32_t value);
    void WriteFloat(std::vector<uint8_t>& buffer, float value);
    void WriteString(std::vector<uint8_t>& buffer, const std::string& value);
    // ... Read equivalents
};
```

---

## 📊 État actuel et roadmap

### ✅ Fonctionnalités implémentées

#### Client
- ✅ Injection DLL (manual mapping)
- ✅ Pattern scanning automatique
- ✅ Connexion au serveur (ENet)
- ✅ Overlay UI (ImGui) : chat, console, debug
- ✅ Système de téléportation
- ✅ Lecture/écriture mémoire
- ✅ Hooks de base (préparation pour D3D11, Input)

#### Serveur
- ✅ Serveur dédié multi-clients
- ✅ Gestion des connexions/déconnexions
- ✅ Système ECS (Flecs)
- ✅ Système de modules
- ✅ CLI (Command Line Interface)
- ✅ Broadcast de messages
- ✅ Logging réseau

#### Framework
- ✅ Pattern Scanner
- ✅ GameOffsets Manager
- ✅ Event System complet
- ✅ 30+ Client Natives implémentées
- ✅ 15+ Server Natives implémentées
- ✅ Documentation complète (README, guides, exemples)

#### Networking
- ✅ ENet intégré (UDP)
- ✅ Sérialisation de paquets
- ✅ Messages typés (Human, Chat, RPC)
- ✅ Communication bidirectionnelle

### 🚧 En cours de développement

- 🚧 Synchronisation complète des joueurs
- 🚧 Synchronisation des rotations/animations
- 🚧 Système de NPCs partagés
- 🚧 Hooks D3D11 pour le rendu
- 🚧 Intégration complète du framework dans le code existant

### 📋 Roadmap future

#### Phase 1 : Stabilisation (Q1 2025)
- [ ] Synchronisation joueur complète (position, rotation, animation)
- [ ] Optimisation de la bande passante
- [ ] Anti-cheat de base
- [ ] Système de permissions
- [ ] Gestion des zones (safe zones, PvP zones)

#### Phase 2 : Gameplay (Q2 2025)
- [ ] Système de sorts synchronisé
- [ ] Combat multijoueur
- [ ] Inventaire synchronisé
- [ ] Quêtes multijoueur
- [ ] NPCs synchronisés

#### Phase 3 : Contenu (Q3 2025)
- [ ] Système de maisons (Gryffondor, Serpentard, etc.)
- [ ] Événements serveur
- [ ] Économie (boutiques, monnaie)
- [ ] Classements (leaderboards)
- [ ] Mini-jeux multijoueur

#### Phase 4 : Extensibilité (Q4 2025)
- [ ] Support Lua pour les scripts
- [ ] API de modding
- [ ] Hot-reload des scripts
- [ ] Éditeur de ressources
- [ ] Système de plugins

---

## 🔍 Points techniques importants

### 1. DLL Injection (Manual Mapping)

Le projet utilise **manual mapping** au lieu d'une injection classique :

**Avantages** :
- ✅ Contourne certaines détections anti-cheat
- ✅ Plus discret dans la liste des modules
- ✅ Contrôle total sur le processus d'injection

**Code** : `code/launcher/src/manual_map.cpp`

### 2. Unreal Engine 4 SDK

Le projet contient un **SDK complet d'Unreal Engine 4** :
- **1265+ fichiers headers**
- Structures exactes du moteur
- Classes du jeu (Character, Controller, World, etc.)

**Important** : Ces structures doivent correspondre **exactement** à la version du jeu.

### 3. ECS (Entity Component System) avec Flecs

Le serveur utilise **Flecs** pour gérer les entités :

```cpp
// Création d'une entité joueur
flecs::entity player = world.entity()
    .set<Position>({x, y, z})
    .set<Rotation>({pitch, yaw, roll})
    .set<PlayerComponent>({playerId, name});

// Système de mise à jour
world.system<Position, Velocity>()
    .each([](Position& pos, const Velocity& vel) {
        pos.x += vel.x;
        pos.y += vel.y;
        pos.z += vel.z;
    });
```

### 4. Logging System

Système de logs centralisé avec rotation :

```cpp
LOG_INFO("Player {} connected", playerId);
LOG_ERROR("Failed to initialize: {}", error);
LOG_DEBUG("Position: {}, {}, {}", x, y, z);
```

**Features** :
- Rotation automatique (5 MB max)
- Niveaux de log (Trace, Debug, Info, Warning, Error, Critical)
- Formatage printf-style
- Fichiers datés (`logs/client_2024-12-23.log`)

---

## 🐛 Debugging et développement

### Outils recommandés

1. **Cheat Engine** : Pour trouver les offsets/patterns
2. **x64dbg** : Debugger pour reverse engineering
3. **IDA Pro / Ghidra** : Désassembleur pour analyse statique
4. **Process Hacker** : Monitorer les processus/DLL
5. **Wireshark** : Analyser le trafic réseau

### Logs utiles

```
logs/client_YYYY-MM-DD.log         # Logs du client
logs/server_YYYY-MM-DD.log         # Logs du serveur
logs/launcher_YYYY-MM-DD.log       # Logs du launcher
```

### Debugging du client

Le client alloue une console pour afficher les logs en temps réel :

```cpp
// Dans main.cpp
AllocConsole();
freopen_s(&f, "CONOUT$", "w", stdout);
LOG_INFO("Client initialized");
```

### Points de breakpoint utiles

- `ClientThread()` - Point d'entrée du thread client
- `NetworkClient::OnPacketReceived()` - Réception de paquets
- `GetPlayerCoords()` - Test de lecture mémoire
- `TriggerEvent()` - Système d'événements

---

## 📚 Ressources et références

### Documentation du projet

- [README.md](README.md) - Vue d'ensemble du projet
- [framework/README.md](framework/README.md) - Documentation du framework
- [framework/INTEGRATION.md](framework/INTEGRATION.md) - Guide d'intégration
- [framework/QUICKSTART.md](framework/QUICKSTART.md) - Démarrage rapide

### Dépôts GitHub

- **Projet principal** : https://github.com/Akitium/HogwartsM
- **Organisation** : https://github.com/HogwartsM/
- **Framework** : https://github.com/HogwartsM/framework

### Inspirations et références

- **FiveM** : https://fivem.net/ (Mod GTA V)
- **ENet Documentation** : http://enet.bespin.org/
- **Flecs Documentation** : https://www.flecs.dev/
- **Unreal Engine Documentation** : https://docs.unrealengine.com/

---

## 🤝 Contribution

### Pour contribuer au projet

1. **Fork** le repository
2. **Clone** votre fork localement
3. **Créer une branche** pour votre feature (`git checkout -b feature/amazing-feature`)
4. **Commit** vos changements (`git commit -m 'Add amazing feature'`)
5. **Push** vers la branche (`git push origin feature/amazing-feature`)
6. **Ouvrir une Pull Request**

### Guidelines

- Suivre le style de code existant (C++17)
- Commenter les sections complexes
- Tester avant de push
- Documenter les nouvelles natives/API
- Utiliser des messages de commit clairs

---

## 📞 Contact et support

### Équipe de développement

- **Lead Developer** : [À compléter]
- **Contributors** : Voir GitHub

### Communication

- **Issues** : https://github.com/Akitium/HogwartsM/issues
- **Discussions** : [À définir - Discord ?]

---

## 📜 Licence

Ce projet est sous licence **MIT**. Voir le fichier [LICENSE](LICENSE) pour plus de détails.

---

## 🎓 Glossaire

- **DLL** : Dynamic Link Library (bibliothèque dynamique Windows)
- **Injection** : Processus d'insertion de code dans un autre processus
- **Pattern Scanning** : Recherche de séquences d'octets en mémoire
- **Offset** : Décalage en mémoire par rapport à une adresse de base
- **ECS** : Entity Component System (architecture de données)
- **RPC** : Remote Procedure Call (appel de procédure distant)
- **Native** : Fonction exposée par l'API du framework
- **Hook** : Interception d'un appel de fonction
- **Manual Mapping** : Technique d'injection avancée

---

**Document créé le** : 23 Décembre 2024
**Version du projet** : 0.0.3
**Dernière mise à jour** : 23 Décembre 2024

---

## 🚀 Commandes rapides

```bash
# Build complet
build.cmd

# Lancer le serveur
start.cmd --server

# Lancer le launcher/client
start.cmd --launcher

# Nettoyer et rebuild
rmdir /s /q build
build.cmd

# Voir les logs
type logs\client_*.log
type logs\server_*.log
```

---

**Bonne chance dans le développement de HogwartsMP ! 🧙‍♂️✨**
