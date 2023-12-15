#pragma once

#include "CoreMinimal.h"
#include "Engine/GameEngine.h"
#include "EOSPlugin.h"
#include "MyGameEngine.generated.h"

/**
 * 
 */
UCLASS()
class GAMEDEMO_API UMyGameEngine : public UGameEngine
{
	GENERATED_BODY()
	
public:
	virtual void Init(class IEngineLoop* InEngineLoop) override;
	
	virtual void Tick( float DeltaSeconds, bool bIdleMode) override;

	UFUNCTION()
	void OnEACLoginHandler(int32 resultCode, const FString& ProductUserID, const FString& ErrorMsg);

	UFUNCTION()
	void OnEACViolationHandler(int32 resultCode, const FString& reason);

	UFUNCTION()
	void OnServerToClientHandler(const TArray<uint8>& EosPacket, uint32 nLength, UNetConnection* ClientConnection);
	
	UFUNCTION()
	void OnServerKickerPlayerHandler(int32 resultCode, const FString& reason, UNetConnection* ClientConnection);

	
private:
	IEOSACSystem* EOSACSystem;
};
