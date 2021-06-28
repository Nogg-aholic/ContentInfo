


#include "ContentInfoBPFuncLib.h"

#include "ContentInfoSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Patching/BlueprintHookHelper.h"
#include "Patching/BlueprintHookManager.h"
#include "Resources/FGBuildingDescriptor.h"

TArray<FString> UContentInfoBPFuncLib::GetBlueprintFunctionNames(UClass* BlueprintClass)
{
	TArray<FString> Names;

	if(BlueprintClass)
		for (auto i : BlueprintClass->*get(grab_FuncTable()))
			Names.Add(i.Key.ToString());
	return Names;
}


UObject* UContentInfoBPFuncLib::Conv_ClassToObject(UClass* Class)
{
	if(Class)
		return Class->GetDefaultObject();
	return nullptr;
}

void UContentInfoBPFuncLib::BindOnBPFunction(const TSubclassOf<UObject> Class, FObjectFunctionBind Binding, const FString FunctionName)
{
	if(!Class)
		return;
	UFunction* ConstructFunction = Class->FindFunctionByName(*FunctionName);
	if (!ConstructFunction || ConstructFunction->IsNative())
	{
		if (!ConstructFunction)
		{
			UE_LOG(LogTemp, Error, TEXT("Was not able to Bind on Function : %s Function was not Found. Function Dump:"), *FunctionName);
			for (auto i : Class->*get(grab_FuncTable()))
			{
				UE_LOG(LogTemp, Error, TEXT("FunctionName : %s"), *i.Key.ToString())
			}
		}
		else
			UE_LOG(LogTemp, Error, TEXT("Was not able to Bind on Function : %s Function was Native"), *FunctionName);

		return;
	}
	UBlueprintHookManager* HookManager = GEngine->GetEngineSubsystem<UBlueprintHookManager>();
	HookManager->HookBlueprintFunction(ConstructFunction, [Binding](FBlueprintHookHelper& HookHelper) {
        Binding.ExecuteIfBound(HookHelper.GetContext());
    }, EPredefinedHookOffset::Return);
}

UTexture2D* UContentInfoBPFuncLib::GetIconForBuilding(TSubclassOf<AFGBuildable> Buildable, bool Big, const UObject* WorldContext)
{
	UContentInfoSubsystem* System = UContentInfoSubsystem::CustomGet(WorldContext->GetWorld());
	TArray<TSubclassOf<AFGBuildable>> Arr;
	if(System->nBuildGunBuildings.Contains(Buildable))
	{
		const TSubclassOf<class UFGBuildingDescriptor> Desc = *System->nBuildGunBuildings.Find(Buildable);
		if(Big)
			return Desc.GetDefaultObject()->GetBigIcon(Desc);
		else
			return Desc.GetDefaultObject()->GetSmallIcon(Desc);
	}
	return nullptr;
}