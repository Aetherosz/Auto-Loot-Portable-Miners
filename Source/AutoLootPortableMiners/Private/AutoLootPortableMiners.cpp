#include "AutoLootPortableMiners.h"

#include "AutoLootPortableMinersConfig.h"

#include "Configuration/ConfigManager.h"
#include "Configuration/Properties/ConfigPropertyBool.h"
#include "Configuration/Properties/ConfigPropertyInteger.h"
#include "Configuration/Properties/ConfigPropertySection.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FGCharacterPlayer.h"
#include "FGInventoryComponent.h"
#include "FGPortableMiner.h"
#include "Patching/NativeHookManager.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogAutoLootPortableMiners, Log, All);

TWeakObjectPtr<UConfigManager> FAutoLootPortableMiners::ConfigManager;
TWeakObjectPtr<UWorld> FAutoLootPortableMiners::ScannedWorld;

// ---------------------------------------------------------------------------
// Hooks
// ---------------------------------------------------------------------------

void FAutoLootPortableMiners::RegisterHooks()
{
	// Register the mod config as soon as the game instance exists, so it is
	// listed in the main menu's Mods -> Mod Configuration before any save loads.
	FWorldDelegates::OnStartGameInstance.AddStatic(&FAutoLootPortableMiners::RegisterConfig);
	FWorldDelegates::OnWorldCleanup.AddStatic(&FAutoLootPortableMiners::OnWorldCleanup);

#if !WITH_EDITOR
	// Write config changes right away. SML otherwise saves on a timer that doesn't run while the
	// game is paused, and its flush at shutdown crashes, so "change a value in the pause menu,
	// then Exit to Desktop" would crash the game.
	SUBSCRIBE_METHOD_AFTER(UConfigManager::MarkConfigurationDirty,
		[](UConfigManager* Self, const FConfigId& Id)
		{
			if (Id == UAutoLootPortableMinersConfig::GetId())
			{
				Self->FlushPendingSaves();
				// The scan timer is paused with the game, so refresh the status line here too.
				if (ScannedWorld.IsValid())
				{
					UpdateStatus(EMenuMode::InSave);
				}
			}
		});

	// On the first player spawn in a world: start scanning.
	SUBSCRIBE_METHOD_VIRTUAL_AFTER(AFGCharacterPlayer::BeginPlay, GetMutableDefault<AFGCharacterPlayer>(),
		[](AFGCharacterPlayer* Self)
		{
			UWorld* World = Self->GetWorld();
			if (!World)
			{
				return;
			}
			if (!ConfigManager.IsValid())
			{
				RegisterConfig(Self->GetGameInstance()); // fallback if OnStartGameInstance fired before we loaded
			}
			if (World->GetNetMode() == NM_Client)
			{
				UpdateStatus(EMenuMode::Client);
				return;
			}
			if (ScannedWorld.Get() != World)
			{
				ScannedWorld = World;
				// World timers stop while single player is paused, so nothing is collected behind a menu.
				FTimerHandle Handle;
				World->GetTimerManager().SetTimer(Handle,
					FTimerDelegate::CreateWeakLambda(World, [World]() { Scan(World); }),
					ScanInterval, true);
				UpdateStatus(EMenuMode::InSave);
			}
		});
#endif
}

void FAutoLootPortableMiners::OnWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources)
{
	if (!World || !World->IsGameWorld())
	{
		return;
	}
	if (ScannedWorld.Get() == World)
	{
		ScannedWorld.Reset();
	}
	UpdateStatus(EMenuMode::MainMenu);
}

// ---------------------------------------------------------------------------
// Scanning
// ---------------------------------------------------------------------------

void FAutoLootPortableMiners::Scan(UWorld* World)
{
	if (!World || World->GetNetMode() == NM_Client)
	{
		return;
	}
	UpdateStatus(EMenuMode::InSave);

	if (!GetConfigBool(UAutoLootPortableMinersConfig::EnabledKey, true))
	{
		return;
	}
	const double RangeSquared = FMath::Square(100.0 * GetRangeMeters());

	// Players who are in the game right now. The bodies of offline players stay in the world
	// and are skipped; a player driving a vehicle has no controller on the body but counts.
	TArray<AFGCharacterPlayer*, TInlineAllocator<8>> Players;
	for (TActorIterator<AFGCharacterPlayer> It(World); It; ++It)
	{
		AFGCharacterPlayer* Player = *It;
		if (IsValid(Player) && Player->HasAuthority() && Player->IsAliveAndWell() && Player->GetInventory()
			&& (Player->GetController() || Player->IsDrivingVehicle()))
		{
			Players.Add(Player);
		}
	}
	if (Players.Num() == 0)
	{
		return;
	}

	for (TActorIterator<AFGPortableMiner> It(World); It; ++It)
	{
		AFGPortableMiner* Miner = *It;
		if (!IsValid(Miner) || !IsFull(Miner->GetOutputInventory()))
		{
			continue;
		}
		const FVector Location = Miner->GetActorLocation();
		for (AFGCharacterPlayer* Player : Players)
		{
			if (FVector::DistSquared(Location, Player->GetActorLocation()) <= RangeSquared && Collect(Miner, Player))
			{
				break; // otherwise (no room) the next player in range gets a turn
			}
		}
	}
}

bool FAutoLootPortableMiners::IsFull(const UFGInventoryComponent* Inventory)
{
	if (!Inventory || Inventory->GetSizeLinear() <= 0)
	{
		return false;
	}
	// Full = no slot can take another item. Capacity comes from the game (slot count of the
	// miner, stack size of the ore), nothing is assumed here.
	for (int32 Index = 0; Index < Inventory->GetSizeLinear(); ++Index)
	{
		FInventoryStack Stack;
		if (!Inventory->GetStackFromIndex(Index, Stack) || !Stack.HasItems()
			|| Stack.NumItems < Inventory->GetSlotSize(Index, Stack.Item.GetItemClass()))
		{
			return false;
		}
	}
	return true;
}

bool FAutoLootPortableMiners::Collect(AFGPortableMiner* Miner, AFGCharacterPlayer* Player)
{
	UFGInventoryComponent* From = Miner->GetOutputInventory();
	UFGInventoryComponent* To = Player->GetInventory();
	if (!From || !To)
	{
		return false;
	}
	TArray<FInventoryStack> Stacks;
	From->GetInventoryStacks(Stacks);
	if (Stacks.Num() == 0 || !To->HasEnoughSpaceForStacks(Stacks))
	{
		return false; // all or nothing: the miner stays full until there is room
	}
	To->AddStacks(Stacks);
	From->Empty();
	UE_LOG(LogAutoLootPortableMiners, Verbose, TEXT("Collected %d stack(s) from %s"), Stacks.Num(), *Miner->GetName());
	return true;
}

// ---------------------------------------------------------------------------
// Mod config
// ---------------------------------------------------------------------------

void FAutoLootPortableMiners::RegisterConfig(UGameInstance* GameInstance)
{
	if (!GameInstance || ConfigManager.IsValid())
	{
		return;
	}
	UConfigManager* Manager = GameInstance->GetSubsystem<UConfigManager>();
	if (!Manager)
	{
		UE_LOG(LogAutoLootPortableMiners, Error, TEXT("SML config manager not found; using default settings"));
		return;
	}
	UAutoLootPortableMinersConfig::UseMenuWidgets();
	Manager->RegisterModConfiguration(UAutoLootPortableMinersConfig::StaticClass());
	ConfigManager = Manager;
	UpdateStatus(EMenuMode::MainMenu);
}

UConfigProperty* FAutoLootPortableMiners::GetConfigProperty(const FString& Key)
{
	UConfigManager* Manager = ConfigManager.Get();
	UConfigPropertySection* Root = Manager ? Manager->GetConfigurationRootSection(UAutoLootPortableMinersConfig::GetId()) : nullptr;
	const TObjectPtr<UConfigProperty>* Property = Root ? Root->SectionProperties.Find(Key) : nullptr;
	return Property ? Property->Get() : nullptr;
}

int32 FAutoLootPortableMiners::GetConfigInt(const FString& Key, int32 Fallback)
{
	const UConfigPropertyInteger* Property = Cast<UConfigPropertyInteger>(GetConfigProperty(Key));
	return Property ? Property->Value : Fallback;
}

bool FAutoLootPortableMiners::GetConfigBool(const FString& Key, bool bFallback)
{
	const UConfigPropertyBool* Property = Cast<UConfigPropertyBool>(GetConfigProperty(Key));
	return Property ? Property->Value : bFallback;
}

int32 FAutoLootPortableMiners::GetRangeMeters()
{
	return FMath::Clamp(GetConfigInt(UAutoLootPortableMinersConfig::RangeKey, UAutoLootPortableMinersConfig::DefaultRange),
		UAutoLootPortableMinersConfig::MinRange, UAutoLootPortableMinersConfig::MaxRange);
}

void FAutoLootPortableMiners::SetStatusText(const FString& Text)
{
	// The label (UAutoLootPortableMinersLabel) redraws its DisplayName every frame.
	if (UConfigProperty* Status = GetConfigProperty(UAutoLootPortableMinersConfig::StatusKey))
	{
		Status->DisplayName = FText::FromString(Text);
	}
}

void FAutoLootPortableMiners::UpdateStatus(EMenuMode Mode)
{
	switch (Mode)
	{
	case EMenuMode::MainMenu:
		SetStatusText(TEXT("Load a save to see the status."));
		return;
	case EMenuMode::Client:
		SetStatusText(TEXT("Multiplayer: the host's settings are used."));
		return;
	case EMenuMode::InSave:
		break;
	}

	if (!GetConfigBool(UAutoLootPortableMinersConfig::EnabledKey, true))
	{
		SetStatusText(TEXT("Off."));
	}
	else
	{
		SetStatusText(FString::Printf(TEXT("Active. Full Portable Miners within %d m are emptied into your inventory."), GetRangeMeters()));
	}
}
