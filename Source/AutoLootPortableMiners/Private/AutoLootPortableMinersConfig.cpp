#include "AutoLootPortableMinersConfig.h"

#include "AutoLootPortableMinersLabel.h"
#include "Configuration/Properties/ConfigPropertyBool.h"
#include "Configuration/Properties/ConfigPropertyInteger.h"
#include "Configuration/Properties/ConfigPropertySection.h"
#include "Configuration/Properties/WidgetExtension/CP_Integer.h"
#include "Configuration/Properties/WidgetExtension/CP_Section.h"

#define LOCTEXT_NAMESPACE "AutoLootPortableMiners"

const FString UAutoLootPortableMinersConfig::StatusKey = TEXT("Status");
const FString UAutoLootPortableMinersConfig::EnabledKey = TEXT("Enabled");
const FString UAutoLootPortableMinersConfig::RangeKey = TEXT("Range");

FConfigId UAutoLootPortableMinersConfig::GetId()
{
	FConfigId Id;
	Id.ModReference = TEXT("AutoLootPortableMiners");
	Id.ConfigCategory = TEXT("");
	return Id;
}

UAutoLootPortableMinersConfig::UAutoLootPortableMinersConfig()
{
	ConfigId = GetId();
	DisplayName = LOCTEXT("ConfigName", "Auto-Loot Portable Miners");
	Description = LOCTEXT("ConfigDescription",
		"Automatically collects the ore from full Portable Miners near you. Changes apply right away. Multiplayer is untested: it should work, with the host's settings being the ones that count.");

	RootSection = CreateDefaultSubobject<UConfigPropertySection>(TEXT("RootSection"));

	UAutoLootPortableMinersLabel* Status = CreateDefaultSubobject<UAutoLootPortableMinersLabel>(TEXT("Status"));
	Status->DisplayName = LOCTEXT("StatusMenu", "Load a save to see the status.");
	Status->bAllowUserReset = false;
	RootSection->SectionProperties.Add(StatusKey, Status);

	UConfigPropertyBool* Enabled = CreateDefaultSubobject<UConfigPropertyBool>(TEXT("Enabled"));
	Enabled->DisplayName = LOCTEXT("EnabledName", "Enabled");
	Enabled->Tooltip = LOCTEXT("EnabledTooltip", "Turn auto-looting on or off without removing the mod.");
	Enabled->DefaultValue = true;
	Enabled->Value = true;
	Enabled->bRequiresWorldReload = false;
	RootSection->SectionProperties.Add(EnabledKey, Enabled);

	UConfigPropertyInteger* Range = CreateDefaultSubobject<UConfigPropertyInteger>(TEXT("Range"));
	Range->DisplayName = LOCTEXT("RangeName", "Range (meters)");
	Range->Tooltip = FText::Format(
		LOCTEXT("RangeTooltip",
			"Full Portable Miners within this distance of a player are emptied into that player's inventory ({0} - {1} m).\n"
			"A miner that isn't full is left alone, and so is a full one when its ore doesn't fit in your inventory."),
		FText::AsNumber(MinRange), FText::AsNumber(MaxRange));
	Range->DefaultValue = DefaultRange;
	Range->Value = DefaultRange;
	Range->bRequiresWorldReload = false;
	RootSection->SectionProperties.Add(RangeKey, Range);
}

namespace
{
	/** Copy of Native as SML's Blueprint version of its class, or Native itself if there is none. */
	UConfigProperty* ToMenuProperty(UConfigProperty* Native, UObject* Outer)
	{
		const UClass* NativeClass = Native->GetClass();
		const TCHAR* BlueprintName =
			NativeClass == UConfigPropertySection::StaticClass() ? TEXT("BP_ConfigPropertySection") :
			NativeClass == UConfigPropertyInteger::StaticClass() ? TEXT("BP_ConfigPropertyInteger") :
			NativeClass == UConfigPropertyBool::StaticClass() ? TEXT("BP_ConfigPropertyBool") : nullptr;
		if (!BlueprintName)
		{
			// Has its own widget (our label) or is already a Blueprint class: copy it as is.
			return DuplicateObject<UConfigProperty>(Native, Outer,
				MakeUniqueObjectName(Outer, NativeClass, FName(*(Native->GetName() + TEXT("_Menu")))));
		}
		UClass* BlueprintClass = LoadClass<UConfigProperty>(nullptr,
			*FString::Printf(TEXT("/SML/Interface/UI/Menu/Mods/ConfigProperties/%s.%s_C"), BlueprintName, BlueprintName));
		if (!BlueprintClass)
		{
			UE_LOG(LogTemp, Error, TEXT("AutoLootPortableMiners: %s not found, the config menu will be empty"), BlueprintName);
			return Native;
		}

		// Unique name: the native property may live in the same outer under its name (e.g. RootSection),
		// and UE refuses to replace an object with one of a different class. The name isn't used anyway;
		// sections look properties up by their map key.
		UConfigProperty* Menu = NewObject<UConfigProperty>(Outer, BlueprintClass,
			MakeUniqueObjectName(Outer, BlueprintClass, FName(*(Native->GetName() + TEXT("_Menu")))));
		Menu->DisplayName = Native->DisplayName;
		Menu->Tooltip = Native->Tooltip;
		Menu->bRequiresWorldReload = Native->bRequiresWorldReload;
		Menu->bHidden = Native->bHidden;
		Menu->bAllowUserReset = Native->bAllowUserReset;

		if (const UConfigPropertyInteger* Int = Cast<UConfigPropertyInteger>(Native))
		{
			UConfigPropertyInteger* MenuInt = CastChecked<UConfigPropertyInteger>(Menu);
			MenuInt->DefaultValue = Int->DefaultValue;
			MenuInt->Value = Int->Value;
			if (UCP_Integer* Widget = Cast<UCP_Integer>(MenuInt))
			{
				// SML's default is a text box with min = max = 0. Range is the only integer here.
				Widget->WidgetType = ECP_IntegerWidgetType::CPI_Spinbox;
				Widget->MinValue = UAutoLootPortableMinersConfig::MinRange;
				Widget->MaxValue = UAutoLootPortableMinersConfig::MaxRange;
			}
		}
		else if (const UConfigPropertyBool* Bool = Cast<UConfigPropertyBool>(Native))
		{
			UConfigPropertyBool* MenuBool = CastChecked<UConfigPropertyBool>(Menu);
			MenuBool->DefaultValue = Bool->DefaultValue;
			MenuBool->Value = Bool->Value;
		}
		else if (const UConfigPropertySection* Section = Cast<UConfigPropertySection>(Native))
		{
			UConfigPropertySection* MenuSection = CastChecked<UConfigPropertySection>(Menu);
			if (UCP_Section* Widget = Cast<UCP_Section>(MenuSection))
			{
				// SML's default is one horizontal row with a scroll bar.
				Widget->WidgetType = ECP_SectionWidgetType::CPS_Vertical;
			}
			for (const TPair<FString, TObjectPtr<UConfigProperty>>& Child : Section->SectionProperties)
			{
				MenuSection->SectionProperties.Add(Child.Key, Child.Value ? ToMenuProperty(Child.Value, MenuSection) : nullptr);
			}
		}
		return Menu;
	}
}

void UAutoLootPortableMinersConfig::UseMenuWidgets()
{
	UAutoLootPortableMinersConfig* Default = GetMutableDefault<UAutoLootPortableMinersConfig>();
	if (Default->RootSection)
	{
		Default->RootSection = CastChecked<UConfigPropertySection>(ToMenuProperty(Default->RootSection, Default));
	}
}

#undef LOCTEXT_NAMESPACE
