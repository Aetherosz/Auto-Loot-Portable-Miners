#pragma once

#include "CoreMinimal.h"
#include "Configuration/ModConfiguration.h"
#include "PortableMinerAutoLootConfig.generated.h"

/**
 * Shows up in the game's Mods -> Mod Configuration menu (main menu and pause menu).
 * SML saves it to FactoryGame/Configs/PortableMinerAutoLoot.cfg.
 */
UCLASS()
class UPortableMinerAutoLootConfig : public UModConfiguration
{
	GENERATED_BODY()

public:
	UPortableMinerAutoLootConfig();

	/** Read-only line: whether auto-looting is active. */
	static const FString StatusKey;
	static const FString EnabledKey;
	/** Range in meters. */
	static const FString RangeKey;

	static constexpr int32 MinRange = 1;
	static constexpr int32 MaxRange = 10;
	static constexpr int32 DefaultRange = 5;

	static FConfigId GetId();

	/**
	 * SML's C++ property classes have no menu widget (only its Blueprint subclasses do), so a
	 * config built in C++ shows up empty. Swaps the default object's properties for SML's
	 * BP_ConfigProperty* classes. Call before registering, once SML's content is loaded.
	 */
	static void UseMenuWidgets();
};
