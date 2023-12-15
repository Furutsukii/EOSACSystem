#include "MyGameInstance.h"
#include "EOSPlugin.h"

UMyGameInstance::UMyGameInstance(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	
}

UMyGameInstance::~UMyGameInstance()
{
	
}

void UMyGameInstance::Init()
{
	Super::Init();
}

void UMyGameInstance::BeginDestroy()
{
	Super::BeginDestroy();
}

void UMyGameInstance::Shutdown()
{
	if (!EOSACSystem)
	{
		EOSACSystem = FEOSPluginModule::Get()->GetEOSACSystem();
	}
	if (GIsServer)
	{
		EOSACSystem->Server_EndSession();
	}
	else
	{
		EOSACSystem->Client_EndSession();
	}

	Super::Shutdown();
}
















































