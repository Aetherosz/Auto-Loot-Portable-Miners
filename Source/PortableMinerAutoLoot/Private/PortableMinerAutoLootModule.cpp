#include "PortableMinerAutoLootModule.h"

#include "PortableMinerAutoLoot.h"

void FPortableMinerAutoLootModule::StartupModule()
{
	FPortableMinerAutoLoot::RegisterHooks();
}

IMPLEMENT_MODULE(FPortableMinerAutoLootModule, PortableMinerAutoLoot)
