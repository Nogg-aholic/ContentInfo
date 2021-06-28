

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Unlocks/FGUnlockRecipe.h"
#include "Unlocks/FGUnlockSchematic.h"
#include "FGItemCategory.h"
#include "FGResearchTreeNode.h"
#include "FGSchematic.h"

#include "ContentInfoSubsystem.generated.h"


class UFGResearchTree;
class UFGItemCategory;

USTRUCT(BlueprintType)
struct  CONTENTINFO_API  FNogs_Descriptor
{
	GENERATED_BODY()
	public:
	friend class UContentInfoSubsystem;
	friend class UFGItemDescriptor;
	FNogs_Descriptor();
	FNogs_Descriptor(TSubclassOf< class UFGItemDescriptor > InClass);
	FNogs_Descriptor(TSubclassOf< class UFGRecipe > Recipe, TSubclassOf< class UFGItemDescriptor > InClass);
	FNogs_Descriptor(TSubclassOf< class UFGItemDescriptor > InClass,TSubclassOf< class UFGRecipe > Recipe);

	UPROPERTY(BlueprintReadOnly)
	TArray<TSubclassOf<class UFGRecipe>> ProductInRecipe;
	UPROPERTY(BlueprintReadOnly)
	TArray<TSubclassOf<class UFGRecipe>> IngredientInRecipe;
	UPROPERTY(BlueprintReadOnly)
	TSubclassOf< class UFGItemDescriptor > ItemClass;

	int32 DepthToResourceClass;

	TMap<int32,TArray<float>> DepthToResource;
	UPROPERTY(BlueprintReadOnly)
	float MJValue;
	
	// Only ResourceDesciptors have this 
	UPROPERTY(BlueprintReadOnly)
	float MJProduced;

	~FNogs_Descriptor() = default;

};

USTRUCT(BlueprintType)
struct  CONTENTINFO_API  FNogs_Schematic
{
	GENERATED_BODY()
	public:
	void DiscoverDependendies();
	FNogs_Schematic();
	FNogs_Schematic(TSubclassOf< class UFGSchematic > inClass, UObject * Context);

	void DiscoverUnlocks();
	
	private:
	void DiscoverSchematics(UFGUnlockSchematic* Unlock) const;
	void DiscoverRecipes(UFGUnlockRecipe* Unlock) const;
	void GatherDependencies();
	public:
	UPROPERTY(BlueprintReadWrite)
	TSubclassOf< class UFGSchematic > nClass;

	
	UPROPERTY(BlueprintReadWrite)
	TArray<TSubclassOf<class UFGSchematic>> nUnlockedBy;

	UPROPERTY(BlueprintReadWrite)
	TArray<TSubclassOf<class UFGSchematic>> nDependsOn;

	UPROPERTY(BlueprintReadWrite)
	TArray<TSubclassOf<class UFGSchematic>> nDependingOnThis;

	UPROPERTY(BlueprintReadWrite)
	TArray<TSubclassOf<class UFGSchematic>> nVisibilityDepOn;

	UPROPERTY(BlueprintReadWrite)
	TArray<TSubclassOf<class UFGSchematic>> nVisibilityDepOnThis; 
	UPROPERTY(BlueprintReadWrite)
	int32 GuessedTier = 0;

	
	UObject * ContextObj;

	~FNogs_Schematic() = default;
};

USTRUCT(BlueprintType)
struct  CONTENTINFO_API  FNogs_Recipe
{
	GENERATED_BODY()

	FNogs_Recipe();
	FNogs_Recipe(const TSubclassOf<UFGRecipe> Class, FNogs_Schematic Schematic);

	void CalculateInfo();


	void DiscoverMachines() const;
	void DiscoverItem() const;

	UPROPERTY(BlueprintReadWrite)
	TArray<TSubclassOf<class UFGSchematic>> nUnlockedBy;
	UPROPERTY(BlueprintReadWrite)
	TSubclassOf< class UFGRecipe > nRecipeClass;

	TArray<TSubclassOf<class UFGItemDescriptor>> Products() const;

	TArray<TSubclassOf<class UFGItemDescriptor>> Ingredients() const;

	TArray<TSubclassOf<class UFGItemCategory>> ProductCats() const;

	TArray<TSubclassOf<class UFGItemCategory>> IngredientCats() const;

	TMap<int32, TArray<float>> IngredientToProductRatio;

	UObject * ContextObj;

	~FNogs_Recipe() = default;

};



/**
 * 
 */
UCLASS(Abstract, Blueprintable)
class CONTENTINFO_API UContentInfoSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

	void CalculateCost();

	public:

	UFUNCTION(BlueprintImplementableEvent)
    FNogs_Schematic HandleResearchTreeSchematic(TSubclassOf<UFGSchematic> Schematic, TSubclassOf<UFGResearchTree> Array );
	
	// Called when Content Registration has Finished
	UFUNCTION(BlueprintCallable)
    void ClientInit();
	
	void CalculateDepth(FNogs_Descriptor& ItemObject);

	UFUNCTION(BlueprintCallable)
	FNogs_Schematic HandleSchematic(TSubclassOf<UFGSchematic> Schematic);

	UFUNCTION(BlueprintCallable)
		void AddToSchematicArrayProp(UPARAM(ref)FNogs_Schematic& Obj, TSubclassOf<UFGSchematic> Schematic, int32 Index);


	UFUNCTION(BlueprintPure, Category="Info", meta=(WorldContext="WorldContextObject"))
	static UContentInfoSubsystem* CustomGet(const UObject* WorldContextObject);

	UPROPERTY(BlueprintReadOnly, Category = "Info")
	TMap<TSubclassOf<class AFGBuildable>,TSubclassOf<class UFGBuildingDescriptor>> nBuildGunBuildings;

	UPROPERTY(BlueprintReadWrite, Category = "Info")
	TMap<TSubclassOf<class UFGItemDescriptor>, FNogs_Descriptor > nItems;

	UPROPERTY(BlueprintReadWrite, Category = "Info")
	TMap<TSubclassOf<class UFGRecipe>, FNogs_Recipe > nRecipes;
	
	UPROPERTY(BlueprintReadWrite, Category = "Info")
    TMap<TSubclassOf<class UFGSchematic>, FNogs_Schematic> nSchematics;

	UPROPERTY(Transient, BlueprintReadWrite)
	TMap< TSubclassOf<class UFGSchematic>, TSubclassOf<class UFGResearchTree>> SchematicResearchTreeParents;

};
