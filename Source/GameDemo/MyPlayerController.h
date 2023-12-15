// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "EOSPlugin.h"
#include "MyPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class GAMEDEMO_API AMyPlayerController : public APlayerController {
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;

	void CheckEosPlayer();

	UFUNCTION()
	void OnClientToServerHandler(const TArray<uint8>& EosPacket, uint32 nLength, UNetConnection* Connection);

	virtual void OnNetCleanup(class UNetConnection* Connection) override;

	UFUNCTION(Reliable, Client)
	void Client_Eos_InitEosAntiCheat();

	UFUNCTION(Reliable, Server)
	void Server_Eos_AddPacket(const TArray<uint8>& EosPacket, const uint32 nLength, UNetConnection* Connection);
	
	UFUNCTION(Reliable, Server)
	void Server_Eos_RegisterClient(const FString& ProductUserID);

	UFUNCTION(Reliable, Client)
	void Client_Eos_AddPacket(const TArray<uint8>& EosPacket, const uint32 nLength);

private:
	IEOSACSystem* EOSACSystem;
};
