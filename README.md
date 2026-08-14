# YaGs

**Yet Another Game Server**

YaGs is a small multiplayer text-game server written in C. Its purpose is to give
people interested in MUD development a clear, approachable codebase they can explore,
change, and learn from.

The project deliberately keeps things simple: the server has no external library
dependencies, and all of the game code lives in a single source file. YaGs is not
intended to be the foundation of a large, production MUD. It is a place to learn how
one works, experiment with ideas, and decide where you want to go next.

## What is implemented

YaGs currently includes:

- Multiple simultaneous player connections over TCP
- Persistent player records, inventory, and equipment
- Rooms, exits, movement, room flags, and objects on the ground
- Mobiles that spawn, wander, fight, respawn, and provide loot and experience
- Player combat, fleeing, death, recovery, experience, and leveling
- Objects, wear positions, weapons, food, and drink
- Shops with listing, buying, and selling
- Hunger, thirst, sitting, sleeping, and hit-point recovery
- Player communication, groups, socials, and online-player listings
- In-game help and administrator commands
- Text-file world definitions that are easy to inspect and modify

## Design philosophy

YaGs favors readable, direct code over abstraction. Most behavior is visible in
`YaGs.c`, while rooms, mobiles, objects, shops, spawns, socials, help, and other game
content are stored in plain-text files.

The code is intentionally modest. You may find techniques worth borrowing, parts you
would design differently, or features you want to extend. All three are useful results.

## Requirements

- Linux, including Linux under Windows Subsystem for Linux (WSL)
- GCC or another compatible C compiler
- A terminal client capable of connecting to a TCP server

YaGs has no third-party library dependencies. It is NOT currently designed to compile
as a native Windows application.

## Build and run

Before building, review the configuration macros near the top of `YaGs.c`. At minimum,
set `YAGS_DIR` to the absolute path of the YaGs folder. You may also want to change
`PORT`, which defaults to `3737`.

Build from a Linux terminal:

```sh
gcc YaGs.c -o YaGs.out -lm
```

Then start the server:

```sh
./YaGs.out
```

YaGs also includes a Visual Studio project configured for Linux development under WSL.

Connect with a MUD client, Telnet client, or a similar TCP terminal client using the
server's address and configured port. For a local server using the default port, connect
to `localhost:3737`.

The first player created in an empty `World/Player.yags` file becomes an administrator.

## Repository layout

- `YaGs.c` - the complete game-server source
- `Library/` - greeting, help, message of the day, socials, and valid player names
- `World/` - rooms, mobiles, objects, shops, spawns, and player data
- `Logs/` - runtime logs
- `Doc/Admin.txt` - setup, operation, and administrator commands
- `Doc/Building.txt` - world-building file formats and guidance
- `Doc/Coding.txt` - code organization, conventions, and extension guidance
- `Doc/ExpCalc.md` - experience and level calculations

Generated player records, inventories, equipment, logs, and build output are excluded
from version control.

## A good place to start

1. Read `Doc/Admin.txt` and configure `YAGS_DIR`.
2. Build and run the server.
3. Connect, create a player, and explore the included world.
4. Read `Doc/Building.txt` and change some world content.
5. Read `Doc/Coding.txt`, then add or modify a small command.

## License

See `LICENSE` for the project license.