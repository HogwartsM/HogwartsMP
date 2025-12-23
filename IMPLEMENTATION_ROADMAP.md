# 🛠 Implementation Roadmap - HogwartsMP v2.0

**Date**: Décembre 2024
**Status**: 🚧 En cours
**Objectif**: Rendre le client et le serveur 100% opérationnels

---

## 📋 Table des matières

1. [État actuel](#état-actuel)
2. [Nettoyage nécessaire](#nettoyage-nécessaire)
3. [Implémentations manquantes](#implémentations-manquantes)
4. [Plan d'action](#plan-daction)
5. [Timeline](#timeline)

---

## 🎯 État actuel

### ✅ Ce qui est fait

**Framework v2.0** (Architecture complète):
- ✅ Headers complets pour tous les systèmes
- ✅ DebugSystem (implémenté, fonctionnel)
- ✅ NetBuffer (implémenté, fonctionnel)
- ✅ EntityManager (headers)
- ✅ BaseEntity (headers)
- ✅ SpellSystem (headers)
- ✅ EventManager (implémenté, fonctionnel)
- ✅ GameOffsets (implémenté, fonctionnel)
- ✅ PatternScanner (implémenté, fonctionnel)
- ✅ ClientNatives (headers)
- ✅ ServerNatives (headers)

**Client v2.0**:
- ✅ client_main.cpp (nouveau, complet)
- ✅ CMakeLists.txt (simplifié)
- ✅ SDK UE4 (référencé par le framework)

**Documentation**:
- ✅ 7 guides complets (~3000 lignes)
- ✅ Exemples d'intégration

### ⚠️ Ce qui manque (Implémentations .cpp)

**Framework - Sources à implémenter**:
1. `NetLibrary.cpp` (Client + Server) - **CRITIQUE**
2. `EntityManager.cpp` - **IMPORTANT**
3. `BaseEntity.cpp` - **IMPORTANT**
4. `SpellSystem.cpp` - **IMPORTANT**
5. `ClientNatives.cpp` - **CRITIQUE**
6. `ServerNatives.cpp` - **CRITIQUE**

**Client - Fichiers obsolètes**:
- ❌ `core/client_instance.cpp/h` (remplacé par client_main.cpp)
- ❌ `Services/network_client.cpp/h` (remplacé par NetLibrary)
- ❌ `UI/*.cpp/h` (supprimé, debug commands à la place)
- ❌ `main.cpp` (remplacé par client_main.cpp)

---

## 🧹 Nettoyage nécessaire

### Fichiers à supprimer du client

```bash
# Core (obsolète)
code/client/src/core/client_instance.cpp
code/client/src/core/client_instance.h

# Services (obsolète)
code/client/src/Services/network_client.cpp
code/client/src/Services/network_client.h

# UI (pas nécessaire pour v2.0)
code/client/src/UI/chat.cpp
code/client/src/UI/chat.h
code/client/src/UI/console.cpp
code/client/src/UI/console.h
code/client/src/UI/season_manager.cpp
code/client/src/UI/season_manager.h
code/client/src/UI/teleport_manager.cpp
code/client/src/UI/teleport_manager.h
code/client/src/UI/ui_base.cpp
code/client/src/UI/ui_base.h

# Main obsolète
code/client/src/main.cpp

# Dossiers vides après nettoyage
code/client/src/Core/
code/client/src/Services/
code/client/src/UI/
```

### Fichiers à garder

```bash
# Client
code/client/src/client_main.cpp          ✅ Nouveau client
code/client/src/sdk/                     ✅ SDK UE4 (utilisé par framework)
code/client/CMakeLists.txt               ✅ Build simplifié

# Framework (tout garder)
code/framework/                          ✅ Tout le framework
```

---

## 🔧 Implémentations manquantes

### 1. NetLibrary (CRITIQUE - Priorité 1)

**Fichiers à créer**:
- `code/framework/components/networking/src/NetLibraryClient.cpp`
- `code/framework/components/networking/src/NetLibraryServer.cpp`

**Dépendance**: ENet (déjà dans vendor/)

**Travail estimé**: ~800 lignes de code

**Fonctionnalités à implémenter**:

```cpp
// NetLibraryClient::Impl
class NetLibraryClient::Impl {
    ENetHost* client;
    ENetPeer* serverPeer;
    std::unordered_map<PacketType, PacketHandler> handlers;

    // À implémenter:
    - bool Connect(host, port)
    - void Disconnect()
    - void Update() // Poll events
    - void SendPacket(type, buffer, reliable)
    - void ProcessPacket(packet)
};

// NetLibraryServer::Impl
class NetLibraryServer::Impl {
    ENetHost* server;
    std::vector<ENetPeer*> clients;
    std::unordered_map<PacketType, PacketHandler> handlers;

    // À implémenter:
    - bool Start(port, maxClients)
    - void Stop()
    - void Update() // Poll events
    - void SendPacket(peerId, type, buffer, reliable)
    - void BroadcastPacket(type, buffer, reliable)
    - void KickClient(peerId, reason)
};
```

**Détails techniques**:
```cpp
// Utilisation d'ENet
#include <enet/enet.h>

// Format de paquet:
struct Packet {
    uint8_t type;      // PacketType
    uint32_t size;     // Taille des données
    uint8_t data[];    // NetBuffer sérialisé
};

// Initialisation ENet
if (enet_initialize() != 0) {
    return false;
}

// Création client
ENetAddress address;
enet_address_set_host(&address, "127.0.0.1");
address.port = 7777;

ENetHost* client = enet_host_create(nullptr, 1, 2, 0, 0);
ENetPeer* peer = enet_host_connect(client, &address, 2, 0);

// Polling events
ENetEvent event;
while (enet_host_service(client, &event, 0) > 0) {
    switch (event.type) {
        case ENET_EVENT_TYPE_CONNECT:
            // Connection établie
            break;
        case ENET_EVENT_TYPE_RECEIVE:
            // Paquet reçu
            ProcessPacket(event.packet);
            enet_packet_destroy(event.packet);
            break;
        case ENET_EVENT_TYPE_DISCONNECT:
            // Déconnexion
            break;
    }
}
```

---

### 2. ClientNatives (CRITIQUE - Priorité 1)

**Fichier**: `code/framework/natives/src/ClientNatives.cpp`

**Travail estimé**: ~400 lignes

**Fonctions à implémenter**:

```cpp
#include "../include/ClientNatives.h"
#include "../../memory/include/GameOffsets.h"
#include "client/src/sdk/entities/acharacter.h"
#include "client/src/sdk/containers/fvector.h"

namespace HogwartsMP::Framework::Natives {

// Récupère le joueur local
APlayerController* GetPlayerController() {
    auto& offsets = Memory::GameOffsets::Get();

    // Via GEngine -> GameViewport -> World -> GameInstance -> LocalPlayers[0] -> PlayerController
    uintptr_t gEngine = offsets.offsets.UWorld; // À adapter
    if (!gEngine) return nullptr;

    // Lire les pointeurs
    uintptr_t gameViewport = Read<uintptr_t>(gEngine + 0x10);
    uintptr_t world = Read<uintptr_t>(gameViewport + 0x10);
    uintptr_t gameInstance = Read<uintptr_t>(world + 0x250);
    uintptr_t localPlayers = Read<uintptr_t>(gameInstance + 0x30);
    uintptr_t player = Read<uintptr_t>(localPlayers);
    uintptr_t playerController = Read<uintptr_t>(player + 0x30);

    return reinterpret_cast<APlayerController*>(playerController);
}

// Récupère les coordonnées du joueur
FVector GetPlayerCoords() {
    APlayerController* controller = GetPlayerController();
    if (!controller) return FVector();

    // PlayerController -> AcknowledgedPawn -> RootComponent -> RelativeLocation
    uintptr_t pawn = Read<uintptr_t>(reinterpret_cast<uintptr_t>(controller) + 0x230);
    uintptr_t rootComponent = Read<uintptr_t>(pawn + 0x158);
    FVector location = Read<FVector>(rootComponent + 0x1F0);

    return location;
}

// Téléporte le joueur
void TeleportPlayer(float x, float y, float z) {
    APlayerController* controller = GetPlayerController();
    if (!controller) return;

    uintptr_t pawn = Read<uintptr_t>(reinterpret_cast<uintptr_t>(controller) + 0x230);
    uintptr_t rootComponent = Read<uintptr_t>(pawn + 0x158);

    FVector newLocation(x, y, z);
    Write<FVector>(rootComponent + 0x1F0, newLocation);
}

// Template helper pour lire/écrire
template<typename T>
T Read(uintptr_t address) {
    return *reinterpret_cast<T*>(address);
}

template<typename T>
void Write(uintptr_t address, const T& value) {
    *reinterpret_cast<T*>(address) = value;
}

// Implémenter toutes les autres fonctions...
// (30+ fonctions au total)

} // namespace
```

---

### 3. EntityManager (IMPORTANT - Priorité 2)

**Fichier**: `code/framework/components/entities/src/EntityManager.cpp`

**Travail estimé**: ~600 lignes

**Méthodes à implémenter**:

```cpp
#include "../include/EntityManager.h"
#include "../include/BaseEntity.h"
#include "../../utilities/include/DebugSystem.h"

namespace HogwartsMP::Framework::Entities {

uint32_t EntityManager::CreateEntity(EntityType type, const std::string& model) {
    DEBUG_PROFILE_FUNCTION();

    uint32_t id = GenerateEntityId();

    std::unique_ptr<BaseEntity> entity;

    switch (type) {
        case EntityType::Player:
            entity = std::make_unique<PlayerEntity>(id);
            break;
        case EntityType::NPC:
            entity = std::make_unique<NPCEntity>(id);
            break;
        default:
            entity = std::make_unique<BaseEntity>(id, type);
            break;
    }

    entity->SetModel(model);
    entity->OnCreated();

    _entities[id] = std::move(entity);

    // Callbacks
    for (auto& callback : _createdCallbacks) {
        callback(id, type);
    }

    DEBUG_LOGF(DebugCategory::Entities, DebugLevel::Detailed,
              "Entity %d created (type: %d)", id, static_cast<int>(type));

    return id;
}

bool EntityManager::DeleteEntity(uint32_t entityId) {
    auto it = _entities.find(entityId);
    if (it == _entities.end()) return false;

    auto type = it->second->GetType();
    it->second->OnDestroyed();

    _entities.erase(it);

    // Callbacks
    for (auto& callback : _deletedCallbacks) {
        callback(entityId, type);
    }

    return true;
}

std::vector<BaseEntity*> EntityManager::GetEntitiesInRange(
    const FVector& position, float radius) const {

    DEBUG_PROFILE_SCOPE("GetEntitiesInRange");

    std::vector<BaseEntity*> result;
    float radiusSq = radius * radius;

    for (const auto& [id, entity] : _entities) {
        FVector entityPos = entity->GetPosition();

        float dx = entityPos.X - position.X;
        float dy = entityPos.Y - position.Y;
        float dz = entityPos.Z - position.Z;
        float distSq = dx*dx + dy*dy + dz*dz;

        if (distSq <= radiusSq) {
            result.push_back(entity.get());
        }
    }

    return result;
}

void EntityManager::Update(float deltaTime) {
    DEBUG_PROFILE_SCOPE("EntityManagerUpdate");

    for (auto& [id, entity] : _entities) {
        entity->Update(deltaTime);
    }
}

// Implémenter toutes les autres méthodes...

} // namespace
```

---

### 4. BaseEntity (IMPORTANT - Priorité 2)

**Fichier**: `code/framework/components/entities/src/BaseEntity.cpp`

**Travail estimé**: ~300 lignes

```cpp
#include "../include/BaseEntity.h"
#include "../../components/networking/include/NetBuffer.h"

namespace HogwartsMP::Framework::Entities {

void BaseEntity::SetPosition(const FVector& position) {
    if (_position.X != position.X ||
        _position.Y != position.Y ||
        _position.Z != position.Z) {

        _position = position;
        MarkDirty();
        OnPositionChanged();
    }
}

void BaseEntity::Update(float deltaTime) {
    // Physique basique si velocity != 0
    if (_velocity.X != 0 || _velocity.Y != 0 || _velocity.Z != 0) {
        if (!IsFrozen()) {
            _position.X += _velocity.X * deltaTime;
            _position.Y += _velocity.Y * deltaTime;
            _position.Z += _velocity.Z * deltaTime;

            MarkDirty();
        }
    }
}

void BaseEntity::Serialize(Networking::NetBuffer& buffer) const {
    buffer.WriteUInt32(_id);
    buffer.WriteByte(static_cast<uint8_t>(_type));
    buffer.WriteString(_model);
    buffer.WriteVector3(_position.X, _position.Y, _position.Z);
    buffer.WriteRotator(_rotation.Pitch, _rotation.Yaw, _rotation.Roll);
    buffer.WriteUInt32(static_cast<uint32_t>(_flags));
    buffer.WriteUInt32(_ownerId);
}

void BaseEntity::Deserialize(Networking::NetBuffer& buffer) {
    _id = buffer.ReadUInt32();
    _type = static_cast<EntityType>(buffer.ReadByte());
    _model = buffer.ReadString();
    buffer.ReadVector3(_position.X, _position.Y, _position.Z);
    buffer.ReadRotator(_rotation.Pitch, _rotation.Yaw, _rotation.Roll);
    _flags = static_cast<EntityFlags>(buffer.ReadUInt32());
    _ownerId = buffer.ReadUInt32();
}

// PlayerEntity
void PlayerEntity::Serialize(Networking::NetBuffer& buffer) const {
    BaseEntity::Serialize(buffer);
    buffer.WriteString(_playerName);
    buffer.WriteUInt32(_health);
    buffer.WriteUInt32(_maxHealth);
}

void PlayerEntity::Deserialize(Networking::NetBuffer& buffer) {
    BaseEntity::Deserialize(buffer);
    _playerName = buffer.ReadString();
    _health = buffer.ReadUInt32();
    _maxHealth = buffer.ReadUInt32();
}

} // namespace
```

---

### 5. SpellSystem (IMPORTANT - Priorité 3)

**Fichier**: `code/framework/components/game/src/SpellSystem.cpp`

**Travail estimé**: ~500 lignes

```cpp
#include "../include/SpellSystem.h"
#include "../../utilities/include/DebugSystem.h"
#include "../../components/entities/include/EntityManager.h"

namespace HogwartsMP::Framework::Game {

void SpellSystem::Initialize() {
    InitializeDefaultSpells();

    DEBUG_LOG(DebugCategory::Game, DebugLevel::Basic,
             "SpellSystem initialized");
}

void SpellSystem::InitializeDefaultSpells() {
    // Stupefy
    SpellDefinition stupefy;
    stupefy.name = "Stupefy";
    stupefy.displayName = "Stupefy";
    stupefy.type = SpellType::Offensive;
    stupefy.element = SpellElement::None;
    stupefy.damage = 25.0f;
    stupefy.range = 300.0f;
    stupefy.castTime = 0.3f;
    stupefy.cooldown = 1.5f;
    stupefy.stun = true;
    stupefy.stunDuration = 2.0f;
    RegisterSpell(stupefy);

    // ... autres sorts par défaut
}

uint32_t SpellSystem::CastSpell(uint32_t caster, const std::string& spellName,
                                uint32_t target, const FVector* targetPosition) {

    if (!CanCastSpell(caster, spellName)) {
        return 0;
    }

    auto* spell = GetSpell(spellName);
    if (!spell) return 0;

    // Créer le sort actif
    uint32_t spellId = GenerateSpellId();
    auto activeSpell = std::make_unique<ActiveSpell>();
    activeSpell->id = spellId;
    activeSpell->spellName = spellName;
    activeSpell->caster = caster;
    activeSpell->target = target;
    activeSpell->timeAlive = 0.0f;
    activeSpell->hit = false;

    // Position de départ = position du caster
    auto& entityMgr = Entities::EntityManager::Get();
    auto* casterEntity = entityMgr.GetEntity(caster);
    if (casterEntity) {
        activeSpell->startPosition = new FVector(casterEntity->GetPosition());
        activeSpell->currentPosition = new FVector(*activeSpell->startPosition);
    }

    _activeSpells[spellId] = std::move(activeSpell);

    // Cooldown
    _cooldowns[caster][spellName] = spell->cooldown;

    // Callbacks
    for (auto& callback : _castCallbacks) {
        callback(caster, spellName);
    }

    return spellId;
}

void SpellSystem::Update(float deltaTime) {
    DEBUG_PROFILE_SCOPE("SpellSystemUpdate");

    // Update cooldowns
    for (auto& [casterId, spells] : _cooldowns) {
        for (auto& [spellName, cooldown] : spells) {
            cooldown -= deltaTime;
            if (cooldown < 0) cooldown = 0;
        }
    }

    // Update active spells
    std::vector<uint32_t> toRemove;
    for (auto& [id, spell] : _activeSpells) {
        UpdateActiveSpell(*spell, deltaTime);

        if (spell->hit || spell->timeAlive > 10.0f) {
            toRemove.push_back(id);
        }
    }

    // Cleanup
    for (auto id : toRemove) {
        _activeSpells.erase(id);
    }
}

void SpellSystem::UpdateActiveSpell(ActiveSpell& spell, float deltaTime) {
    spell.timeAlive += deltaTime;

    auto* spellDef = GetSpell(spell.spellName);
    if (!spellDef) return;

    // Déplacer le projectile
    if (spell.currentPosition && spell.direction) {
        float distance = spellDef->projectileSpeed * deltaTime;
        spell.currentPosition->X += spell.direction->X * distance;
        spell.currentPosition->Y += spell.direction->Y * distance;
        spell.currentPosition->Z += spell.direction->Z * distance;
    }

    // Check collision
    CheckSpellCollision(spell);
}

} // namespace
```

---

### 6. ServerNatives (IMPORTANT - Priorité 3)

**Fichier**: `code/framework/natives/src/ServerNatives.cpp`

**Travail estimé**: ~200 lignes

```cpp
#include "../include/ServerNatives.h"

namespace HogwartsMP::Framework::Natives {

// Variables globales serveur
static std::string g_serverName = "HogwartsMP Server";
static int g_maxPlayers = 32;
static std::unordered_map<std::string, std::string> g_convars;

int GetPlayerCount() {
    // À implémenter avec la vraie liste de joueurs
    return 0;
}

std::vector<int> GetPlayers() {
    // À implémenter
    return {};
}

std::string GetServerName() {
    return g_serverName;
}

void SetServerName(const std::string& name) {
    g_serverName = name;
}

// ... autres fonctions

} // namespace
```

---

## 📅 Plan d'action

### Phase 1: Nettoyage (30 minutes)

```bash
# Supprimer les fichiers obsolètes
rm -rf code/client/src/Core/
rm -rf code/client/src/Services/
rm -rf code/client/src/UI/
rm code/client/src/main.cpp

# Vérifier que client_main.cpp et sdk/ restent
ls code/client/src/
# Doit afficher: client_main.cpp  sdk/
```

### Phase 2: Implémentation NetLibrary (2-3 jours)

**Priorité CRITIQUE**:
1. Créer `NetLibraryClient.cpp` (400 lignes)
2. Créer `NetLibraryServer.cpp` (400 lignes)
3. Tester la connexion Client ↔ Server
4. Tester l'envoi/réception de paquets

**Fichiers**:
```
code/framework/components/networking/src/
├── NetBuffer.cpp               (✅ Déjà fait)
├── NetLibraryClient.cpp        (❌ À faire)
└── NetLibraryServer.cpp        (❌ À faire)
```

### Phase 3: Implémentation ClientNatives (1-2 jours)

**Priorité CRITIQUE**:
1. Implémenter les fonctions Player (GetPlayerCoords, TeleportPlayer, etc.)
2. Implémenter les fonctions World
3. Implémenter les fonctions Entity
4. Tester in-game

**Fichiers**:
```
code/framework/natives/src/
└── ClientNatives.cpp           (❌ À faire - 400 lignes)
```

### Phase 4: Implémentation Entity System (2-3 jours)

**Priorité IMPORTANTE**:
1. `EntityManager.cpp` (600 lignes)
2. `BaseEntity.cpp` (300 lignes)
3. Tester création/suppression d'entités
4. Tester queries spatiales

**Fichiers**:
```
code/framework/components/entities/src/
├── EntityManager.cpp           (❌ À faire)
└── BaseEntity.cpp              (❌ À faire)
```

### Phase 5: Implémentation SpellSystem (1-2 jours)

**Priorité IMPORTANTE**:
1. `SpellSystem.cpp` (500 lignes)
2. Implémenter sorts par défaut
3. Tester lancement de sorts
4. Tester cooldowns

**Fichiers**:
```
code/framework/components/game/src/
└── SpellSystem.cpp             (❌ À faire)
```

### Phase 6: Implémentation ServerNatives (1 jour)

**Priorité MOYENNE**:
1. `ServerNatives.cpp` (200 lignes)
2. Tester fonctions serveur

**Fichiers**:
```
code/framework/natives/src/
└── ServerNatives.cpp           (❌ À faire)
```

### Phase 7: Tests d'intégration (2-3 jours)

1. Test Client standalone
2. Test Server standalone
3. Test Client ↔ Server
4. Test synchronisation entités
5. Test sorts réseau
6. Fix bugs

---

## ⏱ Timeline

### Semaine 1
- Jour 1-2: NetLibrary (Client + Server)
- Jour 3: ClientNatives
- Jour 4-5: EntityManager + BaseEntity

### Semaine 2
- Jour 1-2: SpellSystem
- Jour 3: ServerNatives
- Jour 4-5: Tests et debugging

**Total estimé**: 10-12 jours de développement

---

## 📊 Résumé des tâches

| Tâche | Priorité | Lignes | Temps | Status |
|-------|----------|--------|-------|--------|
| Nettoyage client | 🔴 URGENT | - | 30 min | ⏳ À faire |
| NetLibraryClient.cpp | 🔴 CRITIQUE | 400 | 1-2 jours | ⏳ À faire |
| NetLibraryServer.cpp | 🔴 CRITIQUE | 400 | 1-2 jours | ⏳ À faire |
| ClientNatives.cpp | 🔴 CRITIQUE | 400 | 1-2 jours | ⏳ À faire |
| EntityManager.cpp | 🟡 IMPORTANT | 600 | 2 jours | ⏳ À faire |
| BaseEntity.cpp | 🟡 IMPORTANT | 300 | 1 jour | ⏳ À faire |
| SpellSystem.cpp | 🟡 IMPORTANT | 500 | 1-2 jours | ⏳ À faire |
| ServerNatives.cpp | 🟢 MOYEN | 200 | 1 jour | ⏳ À faire |
| Tests | 🔴 CRITIQUE | - | 2-3 jours | ⏳ À faire |

**Total**: ~2800 lignes de code, 10-12 jours

---

## ✅ Checklist de validation

### Client opérationnel
- [ ] Client compile sans erreurs
- [ ] DLL s'injecte correctement
- [ ] Console de debug s'ouvre
- [ ] Commandes debug fonctionnent (`debug.help`, `pos`, `tp`)
- [ ] Position du joueur récupérée correctement
- [ ] Téléportation fonctionne

### Serveur opérationnel
- [ ] Serveur compile sans erreurs
- [ ] Serveur démarre sur le port 7777
- [ ] Serveur accepte les connexions

### Networking
- [ ] Client se connecte au serveur
- [ ] Paquets Client → Server
- [ ] Paquets Server → Client
- [ ] Broadcast fonctionne
- [ ] Déconnexion propre

### Entity System
- [ ] Création d'entités
- [ ] Suppression d'entités
- [ ] Queries spatiales
- [ ] Synchronisation réseau

### Spell System
- [ ] Sorts par défaut chargés
- [ ] Lancement de sorts
- [ ] Cooldowns fonctionnels
- [ ] Projectiles

---

## 🎯 Objectif final

**Client + Server 100% fonctionnels avec**:
- ✅ Framework v2.0 complet
- ✅ Networking opérationnel
- ✅ Synchronisation entités
- ✅ Système de sorts
- ✅ Debug avancé
- ✅ Performance optimale

**Status**: 🚧 En cours de développement
**ETA**: 10-12 jours

---

**Next step**: Commencer par le nettoyage du client, puis NetLibrary!
