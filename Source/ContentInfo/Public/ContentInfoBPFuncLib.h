

#pragma once

#include "CoreMinimal.h"

#include "Buildables/FGBuildable.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ContentInfoBPFuncLib.generated.h"

DECLARE_DYNAMIC_DELEGATE_OneParam(FObjectFunctionBind, UObject* , Object);

template<typename Tag, typename Tag::type M>
struct Grab {
	friend typename Tag::type get(Tag) {
		return M;
	}
};

struct grab_FuncTable {
	typedef TMap<FName, UFunction*> UClass::* type;
	friend type get(grab_FuncTable);
};

template struct Grab<grab_FuncTable, &UClass::FuncMap>;

UCLASS()
class CONTENTINFO_API UContentInfoBPFuncLib : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	
    UFUNCTION(BlueprintPure, Category = "Class", meta = (DisplayName = "To CDO", CompactNodeTitle = "CDO", BlueprintAutocast))
	static UObject* Conv_ClassToObject(UClass* Class);

	UFUNCTION(BlueprintCallable)
	static void BindOnBPFunction(TSubclassOf<UObject> Class, FObjectFunctionBind Binding, FString FunctionName);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"))
	static UTexture2D* GetIconForBuilding(TSubclassOf<AFGBuildable> Buildable, bool Big, const UObject* WorldContext);

	UFUNCTION(BlueprintPure)
	static TArray<FString> GetBlueprintFunctionNames(UClass * BlueprintClass);


	static FObjectFunctionBind WidgetBind;

};
