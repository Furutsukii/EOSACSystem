// Copyright Epic Games, Inc. All Rights Reserved.


#include "GameDemoGameModeBase.h"

void AGameDemoGameModeBase::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

}

void AGameDemoGameModeBase::BeginPlay()
{
	Super::BeginPlay();
	PlayerControllerClass = AMyPlayerController::StaticClass();
}


void AGameDemoGameModeBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

void AGameDemoGameModeBase::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	if (GIsServer)
	{
		AMyPlayerController* NewPC = Cast<AMyPlayerController>(NewPlayer);
		NewPC->CheckEosPlayer();
	}
}