#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FAutoLootPortableMinersModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
};
