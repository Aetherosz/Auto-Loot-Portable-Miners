#pragma once

#include "CoreMinimal.h"

class AFGCharacterPlayer;
class AFGPortableMiner;
class UConfigManager;
class UConfigProperty;
class UFGInventoryComponent;
class UGameInstance;
class UWorld;

/**
 * Empties full Portable Miners near players into the player's inventory. Runs on the server
 * (single player, host or dedicated server), so in multiplayer only the host's settings should count (multiplayer is untested).
 */
class FAutoLootPortableMiners
{
public:
	static void RegisterHooks();

private:
	enum class EMenuMode { MainMenu, Client, InSave };

	/** How often miners are looked at. */
	static constexpr float ScanInterval = 0.5f;

	static void RegisterConfig(UGameInstance* GameInstance);
	static void OnWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);

	/** Timer on the server: handles the full miners in range of each player. */
	static void Scan(UWorld* World);

	/** True if every slot of the inventory holds a complete stack. */
	static bool IsFull(const UFGInventoryComponent* Inventory);
	/** Moves everything from the miner to the player, or nothing if it doesn't all fit. */
	static bool Collect(AFGPortableMiner* Miner, AFGCharacterPlayer* Player);

	static UConfigProperty* GetConfigProperty(const FString& Key);
	static int32 GetConfigInt(const FString& Key, int32 Fallback);
	static bool GetConfigBool(const FString& Key, bool bFallback);
	static int32 GetRangeMeters();
	static void SetStatusText(const FString& Text);
	static void UpdateStatus(EMenuMode Mode);

	static TWeakObjectPtr<UConfigManager> ConfigManager;
	/** The world the scan timer runs in. */
	static TWeakObjectPtr<UWorld> ScannedWorld;
};
