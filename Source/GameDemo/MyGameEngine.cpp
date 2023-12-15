#include "MyGameEngine.h"
#include "MyPlayerController.h"

void UMyGameEngine::Init(IEngineLoop* InEngineLoop)
{
	Super::Init(InEngineLoop);

	FEOSPluginModule* pPlugin = FEOSPluginModule::Get();
	if (pPlugin)
	{
		EOSACSystem = FEOSPluginModule::Get()->GetEOSACSystem();
	}

	if (GIsServer)
	{
		bool InitResult = EOSACSystem->PlatformInit();
		if(InitResult)
		{
			FServerToClientEosAddPacketDelegate m_delegate = FServerToClientEosAddPacketDelegate::CreateUObject(this, &UMyGameEngine::OnServerToClientHandler);
			FEACServerKickerDelegate kickerDelegate = FEACServerKickerDelegate::CreateUObject(this, &UMyGameEngine::OnServerKickerPlayerHandler);

			int serverID = 1;
			FString serverIDStr = FString::FromInt(serverID);
			EOSACSystem->Server_BeginSession(serverIDStr, m_delegate, kickerDelegate);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("EOSAC Server init failed"));
		}
	}
	else
	{
		bool InitResult = EOSACSystem->PlatformInit();
		if(InitResult)
		{
			FClientViolationDelegate ViolationDelegate = FClientViolationDelegate::CreateUObject(this, &UMyGameEngine::OnEACViolationHandler);
			EOSACSystem->Add_ClientViolation_Notification(ViolationDelegate);

			// EAC Login 
			FEACLoginDelegate loginDelegate = FEACLoginDelegate::CreateUObject(this, &UMyGameEngine::OnEACLoginHandler);
			EOSACSystem->Client_CallLogin(loginDelegate);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("EOSAC Client init failed"));
		}
	}
}

void UMyGameEngine::Tick(float DeltaSeconds, bool bIdleMode)
{
	Super::Tick(DeltaSeconds, bIdleMode);

	FEOSPluginModule* pPlugin = FEOSPluginModule::Get();
	if (pPlugin)
	{
		EOSACSystem = FEOSPluginModule::Get()->GetEOSACSystem();
		EOSACSystem->Tick(DeltaSeconds);
	}
}

void UMyGameEngine::OnEACLoginHandler(int32 resultCode, const FString& ProductUserID, const FString& ErrorMsg)
{
	UE_LOG(LogTemp, Warning, TEXT("OnEACLoginHandler() ProductUserID: %s  ErrorMsg: %s"), *ProductUserID,*ErrorMsg);

	if(ErrorMsg.IsEmpty()){
		return;
	}
}

void UMyGameEngine::OnServerToClientHandler(const TArray<uint8>& EosPacket, uint32 nLength, UNetConnection* ClientConnection)
{
	if (GIsServer)
	{
		AMyPlayerController* NewPC = Cast<AMyPlayerController>(ClientConnection->OwningActor);

		if(!NewPC)
		{
			UE_LOG(LogTemp, Warning, TEXT("OnServerToClientHandler cannot Get AMyPlayerController,  PlayerController = null"));
		}
		NewPC->Client_Eos_AddPacket(EosPacket, nLength);
	}
}

void UMyGameEngine::OnServerKickerPlayerHandler(int32 resultCode, const FString& reason, UNetConnection* ClientConnection)
{
	UE_LOG(LogTemp, Warning, TEXT("OnServerKickerPlayerHandler, resultCode:%d , reason:%s"), resultCode, *reason);
}


void UMyGameEngine::OnEACViolationHandler(int32 resultCode, const FString& reason)
{
	UE_LOG(LogTemp, Warning, TEXT("OnEACViolationHandler, resultCode:%d , reason:%s"), resultCode, *reason);
}
