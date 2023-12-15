
#include "MyPlayerController.h"
#include "GameDemoGameModeBase.h"
#include <iostream>


void AMyPlayerController::BeginPlay()
{

}


void AMyPlayerController::CheckEosPlayer() {

	if (GIsServer && GetNetConnection())
	{
		AGameDemoGameModeBase* pGameMode = Cast<AGameDemoGameModeBase>(GetWorld()->GetAuthGameMode());

		FString PlayerName;

		if (pGameMode && GetNetConnection())
		{
			this->Client_Eos_InitEosAntiCheat();
		}
	}
}

void AMyPlayerController::OnClientToServerHandler(const TArray<uint8>& EosPacket, uint32 nLength, UNetConnection* Connection)
{
	this->Server_Eos_AddPacket(EosPacket, nLength, GetNetConnection());
}

void AMyPlayerController::OnNetCleanup(class UNetConnection* Connection)
{
	Super::OnNetCleanup(Connection);

	if (!EOSACSystem)
	{
		EOSACSystem = FEOSPluginModule::Get()->GetEOSACSystem();
	}

	if (GIsClient)
	{
		EOSACSystem->Client_EndSession();
	}
	else if (GIsServer)
	{
		EOSACSystem->Server_UnregisterClient(Connection);
	}
}

void AMyPlayerController::Server_Eos_RegisterClient_Implementation(const FString& ProductUserID) {
	if (!EOSACSystem)
	{
		EOSACSystem = FEOSPluginModule::Get()->GetEOSACSystem();
	}

	if (GIsServer)
	{
		if(EOSACSystem->Server_RegisterClient(GetNetConnection(), ProductUserID))
		{
			UE_LOG(LogTemp, Warning, TEXT("Server_RegisterClient Success"));
		}
	}
}

void AMyPlayerController::Server_Eos_AddPacket_Implementation(const TArray<uint8>& EosPacket, const uint32 nLength, UNetConnection* Connection)
{
	if (GIsServer)
	{
		if (!EOSACSystem)
		{
			EOSACSystem = FEOSPluginModule::Get()->GetEOSACSystem();
		}
		
		UNetConnection* ClientHandle = GetNetConnection();
		EOSACSystem->Server_ReceiveMessageFromClient(EosPacket, nLength, ClientHandle);
	}
}

void AMyPlayerController::Client_Eos_AddPacket_Implementation(const TArray<uint8>& EosPacket, const uint32 nLength)
{
	if (!EOSACSystem)
	{
		EOSACSystem = FEOSPluginModule::Get()->GetEOSACSystem();
	}
	
	EOSACSystem->Client_ReceiveMessageFromServer(EosPacket, nLength);
}


void AMyPlayerController::Client_Eos_InitEosAntiCheat_Implementation()
{
	if (!EOSACSystem)
	{
		EOSACSystem = FEOSPluginModule::Get()->GetEOSACSystem();
	}

	if(!EOSACSystem->Check_Start_Protected_Game())
	{
		return;
	}

	if (!EOSACSystem->Client_IsServiceInstalled())
	{
		return;
	}

	FString ProductUserID = EOSACSystem->Client_GetProductUserID();

	FCTSEosAddPacketDelegate m_delegate = FCTSEosAddPacketDelegate::CreateUObject(this, &AMyPlayerController::OnClientToServerHandler);

	if (GetNetConnection() && !EOSACSystem->Client_BeginSession(m_delegate, ProductUserID, GetNetConnection()))
	{
		UE_LOG(LogTemp, Error, TEXT("Client_Eos_InitEosAntiCheat() Client beginSession Failed Init failed"));
	}

	this->Server_Eos_RegisterClient(ProductUserID);
}