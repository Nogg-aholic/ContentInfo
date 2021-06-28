


#include "BP_SubsystemLoader.h"
#include "ContentInfoSubsystem.h"


void UBP_SubsystemLoader::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UClass* Class = LoadClass<UContentInfoSubsystem>(nullptr, TEXT("/ContentInfo/BP_ContentInfoSubsystem.BP_ContentInfoSubsystem_C"));
	Collection.InitializeDependency(Class);
}
