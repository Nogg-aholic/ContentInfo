

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "BP_SubsystemLoader.generated.h"

/**
 * Only for Spawning our BP Subsystem
 */
UCLASS()
class CONTENTINFO_API UBP_SubsystemLoader : public UWorldSubsystem
{
	GENERATED_BODY()

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
};
