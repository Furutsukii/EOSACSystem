// Copyright Epic Games, Inc. All Rights Reserved.

#include "EOSPlugin.h"
#include "../service/EOSACSystem.h"

#define LOCTEXT_NAMESPACE "FEOSPluginModule"

void FEOSPluginModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FEOSPluginModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#if USE_EOSAC_SYSTEM 
IEOSACSystem* FEOSPluginModule::GetEOSACSystem()
{
	return FEOSACSystem::GetInstance();
}
#endif

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FEOSPluginModule, EOSPlugin)