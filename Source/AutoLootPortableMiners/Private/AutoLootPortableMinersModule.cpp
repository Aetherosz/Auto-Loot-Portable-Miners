#include "AutoLootPortableMinersModule.h"

#include "AutoLootPortableMiners.h"

void FAutoLootPortableMinersModule::StartupModule()
{
	FAutoLootPortableMiners::RegisterHooks();
}

IMPLEMENT_MODULE(FAutoLootPortableMinersModule, AutoLootPortableMiners)
