// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "EOSPlugin.h"
#include "MyPlayerController.h"
#include "GameDemoGameModeBase.generated.h"

/**
 * 
 */
UCLASS()
class GAMEDEMO_API AGameDemoGameModeBase : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	virtual void BeginPlay() override;

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

	virtual void PostLogin(APlayerController* NewPlayer) override;

	virtual void Tick(float DeltaSeconds) override;

private:
	IEOSACSystem* EOSACSystem;
};
