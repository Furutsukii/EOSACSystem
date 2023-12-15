// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "../interface/EOSACSystemAPI.h"
#include "../service/EOSACSystem.h"
#include "Modules/ModuleManager.h"

class FEOSPluginModule : public IModuleInterface
{
public:
	//static FEOSPluginModule& Get();
	static inline FEOSPluginModule* Get()
	{
		return FModuleManager::LoadModulePtr<FEOSPluginModule>("EOSPlugin");
	}

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
#if USE_EOSAC_SYSTEM
	virtual IEOSACSystem* GetEOSACSystem();
#endif
private:
	static FEOSPluginModule* Singleton;
};
