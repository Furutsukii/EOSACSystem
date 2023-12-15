#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "EOSPlugin.h"
#include "MyGameInstance.generated.h"

#if !WITH_EDITOR
extern bool GEnableParallelTickFunction;
extern bool GAllowCapsuleCollision;
extern bool GAllowSkeletalMeshCollision;
#endif

/**
 * 
 */
UCLASS(config=Game)
class GAMEDEMO_API UMyGameInstance : public UGameInstance
{
public:
	GENERATED_UCLASS_BODY()

	~UMyGameInstance();

	virtual void Init() override;
	virtual void Shutdown() override;
	virtual void BeginDestroy() override;

public:
	IEOSACSystem* EOSACSystem;
};
