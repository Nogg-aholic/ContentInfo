

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
struct  CONTENTINFO_API  FNogs_ProductBuildingCost
{
	GENERATED_BODY()
	FNogs_ProductBuildingCost();
	FNogs_ProductBuildingCost(TSubclassOf<UFGRecipe> InRecipe,TSubclassOf<UObject> InBuilding);

	float GetMjCost() const;

	float GetMjCostForPotential(float Potential) const;
	
	UPROPERTY(BlueprintReadOnly)
	TSubclassOf<UFGRecipe> Recipe;
	UPROPERTY(BlueprintReadOnly)
	TSubclassOf<UObject> Building;
};

USTRUCT(BlueprintType)
struct  CONTENTINFO_API  FNogs_RecipeMJ
{
	
	GENERATED_BODY()
	friend struct FNogs_Recipe;
	friend class UContentInfoSubsystem;
	FNogs_RecipeMJ();
	FNogs_RecipeMJ(TSubclassOf<UFGRecipe>  Outer);


	bool HasAssignedMJ() const;
	bool TryAssignMJ(UContentInfoSubsystem * System);

	int32 GetItemAmount(TSubclassOf<UFGItemDescriptor> Item, bool Ingredient);
	private:
	bool CanCalculateMj(UContentInfoSubsystem * System) const;
	float GetAverageBuildingCost(TArray<TSubclassOf<UObject>> Exclude)const;
	void AddValue(const float Value);
	public:
	float GetProductMjValue(TSubclassOf<UFGItemDescriptor> Item,bool PerItem = true , TSubclassOf<UObject> Buildable = nullptr, bool ExcludeManual = true , float Potential = 1.f);

	UPROPERTY(BlueprintReadOnly)
	float MJValueOverride= -1.f;

	UPROPERTY(BlueprintReadOnly)
	float MJ_Average = 0.f;

	UPROPERTY(BlueprintReadOnly)
	TSubclassOf<UFGRecipe> nRecipe;
};




USTRUCT(BlueprintType)
struct  CONTENTINFO_API  FNogs_Descriptor
{
	GENERATED_BODY()
	public:
	friend class UContentInfoSubsystem;
	friend class UFGItemDescriptor;
	FNogs_Descriptor();
	FNogs_Descriptor(TSubclassOf< class UFGItemDescriptor > InClass);
	FNogs_Descriptor(TSubclassOf< class UFGItemDescriptor > InClass,TSubclassOf< class UFGRecipe > Recipe);
	float GetMj(FNogs_Recipe Recipe,TSubclassOf<UObject> Buildable = nullptr ) const;

	static void RecurseIngredients(TSubclassOf<class UFGItemDescriptor> Item , TArray<TSubclassOf<class UFGItemDescriptor>> & AllItems , TArray<TSubclassOf<class UFGRecipe>> & AllRecipes ,UContentInfoSubsystem * System, bool SkipAlternate, TArray<TSubclassOf<class UFGRecipe>> Excluded, bool UseFirst = false);
	static int32 CalculateDepth(UContentInfoSubsystem* System, TSubclassOf<UFGItemDescriptor> Item);

	bool HasMj() const;
	float AssignAverageMj(UContentInfoSubsystem * System,TArray<TSubclassOf<UFGRecipe>> Exclude = TArray<TSubclassOf<UFGRecipe>>(),TArray<TSubclassOf<UObject>> ExcludeBuilding = TArray<TSubclassOf<UObject>>());
	
	void AssignResourceValue();

	UPROPERTY(BlueprintReadOnly)
	TArray<TSubclassOf<class UFGRecipe>> ProductInRecipe;
	UPROPERTY(BlueprintReadOnly)
	TArray<TSubclassOf<class UFGRecipe>> IngredientInRecipe;
	UPROPERTY(BlueprintReadOnly)
	TSubclassOf< class UFGItemDescriptor > ItemClass;

	void SetMj(float Value,bool Override = false);

	UPROPERTY(BlueprintReadOnly)
	float MJValue = -1.f;

	~FNogs_Descriptor() = default;

};

USTRUCT(BlueprintType)
struct  CONTENTINFO_API  FNogs_Schematic
{
	GENERATED_BODY()
	public:
	FNogs_Schematic();
	FNogs_Schematic(TSubclassOf< class UFGSchematic > inClass, UContentInfoSubsystem* System);

	void DiscoverUnlocks(UContentInfoSubsystem* System);
	
	private:
	void DiscoverSchematics(UFGUnlockSchematic* Unlock,UContentInfoSubsystem* System) const;
	void DiscoverRecipes(UFGUnlockRecipe* Unlock, UContentInfoSubsystem* System) const;
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


	~FNogs_Schematic() = default;
};

USTRUCT(BlueprintType)
struct  CONTENTINFO_API  FNogs_Recipe
{
	GENERATED_BODY()

	FNogs_Recipe();
	FNogs_Recipe(const TSubclassOf<UFGRecipe> Class, FNogs_Schematic Schematic);

	void DiscoverMachines(UContentInfoSubsystem* System ) const;
	void DiscoverItem(UContentInfoSubsystem* System ) const;
	bool IsManualOnly() const;
	bool IsManual() const;

	TArray<float> GetIngredientsForProductRatio(TSubclassOf<UFGItemDescriptor> Item) const;
	float GetItemToTotalProductRatio(TSubclassOf<UFGItemDescriptor> Item,UContentInfoSubsystem* System ) const;

	bool UnlockedFromAlternate();
	bool IsBuildGunRecipe() const;

	UPROPERTY(BlueprintReadWrite)
	TArray<TSubclassOf<class UFGSchematic>> nUnlockedBy;
	UPROPERTY(BlueprintReadWrite)
	TSubclassOf< class UFGRecipe > nRecipeClass;

	TArray<TSubclassOf<class UFGItemDescriptor>> Products() const;

	TArray<TSubclassOf<class UFGItemDescriptor>> Ingredients() const;

	TArray<TSubclassOf<class UFGItemCategory>> ProductCats() const;

	TArray<TSubclassOf<class UFGItemCategory>> IngredientCats() const;

	UPROPERTY(BlueprintReadOnly)
	FNogs_RecipeMJ MJ;

	UPROPERTY(BlueprintReadOnly)
	FNogs_RecipeMJ MJ_Manual;

	~FNogs_Recipe() = default;

};



/**
 * 
 */
UCLASS(Abstract, Blueprintable)
class CONTENTINFO_API UContentInfoSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

	void CalculateCost(TArray<TSubclassOf<UFGRecipe>> RecipesToCalc);
	TMap<TSubclassOf<UFGItemDescriptor>,FNogs_Descriptor> CalculateRecipesRecursively(TSubclassOf<UFGItemDescriptor> Item, TArray<TSubclassOf<UFGRecipe>> Exclude,bool UseAlternates,UContentInfoSubsystem * System);
	static void SortPairs(TArray<TSubclassOf<UObject>> & Array_To_Sort_Keys,TArray<float> & Array_To_Sort_Values,bool Descending = true);


	void PrintSortedRecipes();
	void PrintSortedItems();

	void FullRecipeCalculation();
	public:

	UFUNCTION(BlueprintImplementableEvent)
    FNogs_Schematic HandleResearchTreeSchematic(TSubclassOf<UFGSchematic> Schematic, TSubclassOf<UFGResearchTree> Array );

	// Called when Content Registration has Finished
	UFUNCTION(BlueprintCallable)
    void ClientInit();
	
	UFUNCTION(BlueprintCallable)
	FNogs_Schematic HandleSchematic(TSubclassOf<UFGSchematic> Schematic);

	UFUNCTION(BlueprintCallable)
	static void AddToSchematicArrayProp(UPARAM(ref)FNogs_Schematic& Obj, TSubclassOf<UFGSchematic> Schematic, int32 Index);


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
