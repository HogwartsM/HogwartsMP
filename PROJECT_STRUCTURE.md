# Documentation Technique du Projet HogwartsMP

Ce document fournit une analyse détaillée de la structure du projet HogwartsMP, un framework multijoueur pour Hogwarts Legacy. Il est destiné aux développeurs et aux LLM pour comprendre l'architecture du système.

## Vue d'ensemble

HogwartsMP est une modification qui ajoute des fonctionnalités multijoueurs à Hogwarts Legacy. Il utilise une architecture Client-Serveur classique :
- **Serveur** : Une application console autonome qui gère l'état du monde et la synchronisation.
- **Client** : Une DLL injectée dans le processus du jeu qui intercepte les appels du moteur et communique avec le serveur.
- **Launcher** : Un outil pour lancer le jeu et injecter la DLL client, avec des techniques de contournement (bypass) pour les protections (Denuvo).

Le projet utilise **ENet** pour le réseau, **Flecs** pour le système d'entités (ECS), et **MinHook** pour le hooking des fonctions.

## Structure des Dossiers

```
HogwartsM/
├── code/                   # Code source principal
│   ├── client/             # Logique côté client (DLL)
│   ├── server/             # Logique côté serveur (Console)
│   ├── launcher/           # Outil d'injection et de lancement
│   └── shared/             # Code partagé (Protocoles, Utils)
├── vendor/                 # Bibliothèques tierces (ENet, Flecs, GLM, etc.)
└── CMakeLists.txt          # Configuration de build globale
```

## Analyse Détaillée par Module

### 1. Client (`code/client/`)

Le client est compilé sous forme de bibliothèque dynamique (`libHogwartsMPClient.dll`).

**Point d'entrée** : `src/main.cpp` -> `DllMain`
- Initialise la console de débogage.
- Lance le thread principal `ClientThread`.
- Crée une instance de `ClientInstance`.

**Composants Clés** :
- **Core (`src/Core/`)** :
  - `ClientInstance` : Gère le cycle de vie du client, la connexion au serveur et la boucle de mise à jour.
- **SDK (`src/SDK/`)** :
  - Contient les définitions des structures de l'Unreal Engine 4/5 (UWorld, ULevel, UObject, etc.).
  - Permet d'interagir avec la mémoire du jeu.
- **UI (`src/UI/`)** :
  - Gestion de l'interface utilisateur superposée (Chat, Console) via ImGui (supposé via les dépendances).

### 2. Serveur (`code/server/`)

Le serveur est une application console autonome (`HogwartsMPServer.exe`).

**Point d'entrée** : `src/main.cpp` -> `main`
- Initialise le système de logs.
- Configure les options du serveur (port 27015 par défaut).
- Lance la boucle principale `server.Update()`.

**Composants Clés** :
- **Core (`src/core/`)** :
  - `Server` : Classe principale gérant les connexions clients et la logique de jeu.
- **CLI (`src/cli/`)** :
  - Interface en ligne de commande pour l'administration du serveur.

### 3. Launcher (`code/launcher/`)

Le launcher est un exécutable (`HogwartsMPLauncher.exe`) responsable du démarrage du jeu.

**Point d'entrée** : `src/main.cpp`
- Détecte l'installation de Hogwarts Legacy (Steam).
- Lance le jeu.
- Injecte la DLL client (`libHogwartsMPClient.dll`).

**Fonctionnalités Spéciales** :
- **Manual Map Injection** : Utilise une technique d'injection manuelle pour contourner les protections anti-triche/DRM (Denuvo).
- Évite l'utilisation de `LoadLibrary` standard qui pourrait être surveillé.

### 4. Shared (`code/shared/`)

Contient le code commun utilisé par le client et le serveur pour assurer la compatibilité.

**Sous-modules** :
- **messages/** : Définit le protocole de communication.
  - `messages.h` : Énumération des types de messages (Spawn, Despawn, Update).
- **rpc/** : Définitions pour les appels de procédure à distance (Remote Procedure Calls).
- **logging/** : Système de journalisation unifié.
- **utils/** :
  - `hooking/` : Wrappers pour MinHook.
  - `states/` : Machines à états pour la logique de jeu.

## Protocoles et Données

- **Réseau** : Utilise ENet pour une communication UDP fiable/non fiable.
- **Sérialisation** : Les paquets sont probablement structurés en binaire (à confirmer via l'analyse du code de sérialisation).
- **Synchronisation** :
  - `MOD_HUMAN_SPAWN/DESPAWN` : Gestion des entités joueurs/NPC.
  - `MOD_HUMAN_UPDATE` : Synchronisation des positions et états.

## Dépendances (Vendor)

- **ENet** : Bibliothèque réseau UDP.
- **Flecs** : Entity Component System (ECS) pour une gestion performante des entités de jeu.
- **GLM** : Bibliothèque mathématique (vecteurs, matrices) compatible GLSL.
- **ImGui** : Interface graphique immédiate (pour l'UI client).
- **MinHook** : Bibliothèque de hooking x86/x64 pour Windows.

## Instructions de Build

Le projet utilise CMake.

1.  **Prérequis** :
    - Windows 10/11 x64
    - MinGW-w64 (GCC 15.2.0+)
    - CMake 3.10+
    - Git

2.  **Compilation** :
    ```bash
    mkdir build && cd build
    cmake -G "MinGW Makefiles" ..
    mingw32-make -j8
    ```

3.  **Résultat** :
    - `HogwartsMPServer.exe`
    - `HogwartsMPLauncher.exe`
    - `libHogwartsMPClient.dll`

## Notes pour les Développeurs

- **SDK Unreal** : Le dossier `client/src/SDK` est critique. Toute mise à jour du jeu peut nécessiter une mise à jour des offsets ou des structures dans ce dossier.
- **Hooking** : Les hooks sont essentiels pour intercepter les événements du jeu. Regarder `shared/utils/hooking` et comment ils sont utilisés dans le client.
- **Injection** : Si le launcher échoue, vérifier les antivirus ou les mises à jour de Denuvo qui pourraient bloquer l'injection manuelle.
