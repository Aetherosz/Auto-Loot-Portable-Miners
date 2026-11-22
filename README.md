# Auto-Loot Portable Miners

A Satisfactory mod (mod ID `AutoLootPortableMiners`) that empties full Portable Miners into your inventory when you get close to them.

A miner counts as full when it can't hold any more ore, which is 100 ore. Miners that are still filling up are left alone.

## Settings

Open Mods -> Mod Configuration -> Auto-Loot Portable Miners, from the main menu or the pause menu. Changes apply right away.

- **Enabled** turns auto-looting on or off without removing the mod.
- **Range (meters)** is how close you have to be to a miner. 1 - 10, default 5.

The line at the top of the settings shows whether the mod is active or off.

## Good to know
- If the ore doesn't fit in your inventory, the miner keeps it until you have room.
- Nothing is collected while the game is paused.
- Multiplayer is not tested. It should run on the host and use the host's settings.
- Windows client only for now. No dedicated server builds.

## Requirements
- Satisfactory 1.2.6 (build 520140) or newer
- SML 3.12.1 or newer

## Building
Put this repo at `<project>\Mods\AutoLootPortableMiners` in the SatisfactoryModLoader starter project (SML 3.12.1) and package it with Alpakit, or with `RunUAT.bat ... PackagePlugin -DLCName=AutoLootPortableMiners`.
