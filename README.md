# HogwartsMP

This repository contains the code for the HogwartsMP project:

**HogwartsMP**, a multiplayer modification framework for the PC version of Hogwarts Legacy as released by WB Games.

**HogwartsMPServer**, the dedicated server component for multiplayer services on the HogwartsMP network.

HogwartsMP differs from other similar multiplayer modifications by implementing a comprehensive client-server architecture with entity synchronization, building a modification framework around the game's core systems, and providing extensive modding capabilities through native game integration and scripting support.

## Features

The HogwartsMP framework includes:

- **Server/Client Architecture**: ENet-based networking for reliable and efficient multiplayer communication
- **Advanced Injection System**: DLL injection launcher with Denuvo bypass for seamless game integration
- **Entity Component System**: Flecs-based ECS for scalable entity management and synchronization
- **Game SDK Integration**: Comprehensive SDK with access to game entities (Player, Character, Controller), UI components, and world management
- **In-Game Systems**: Chat, console, teleport manager, and season/weather control
- **Scripting Support**: Lua scripting engine (in development) for server-side resources
- **RPC Framework**: Remote Procedure Call system for synchronized gameplay actions
- **Resource System**: FiveM-style resource loading and management (in development)

## Getting started

To play HogwartsMP, simply download the launcher binaries from the [releases page](https://github.com/Akitium/HogwartsM/releases).

To develop HogwartsMP, please follow the compilation instructions below.

## License

HogwartsMP is licensed under the GNU Affero General Public License v3.0 (AGPL-3.0), details of which are in the [LICENSE](LICENSE) file in the repository. This project is for educational purposes only.

## Links

- [GitHub Repository](https://github.com/Akitium/HogwartsM)
- [Issues](https://github.com/Akitium/HogwartsM/issues)
- [Releases](https://github.com/Akitium/HogwartsM/releases)

**Disclaimer**: This is an unofficial multiplayer mod for educational purposes. Use at your own risk.