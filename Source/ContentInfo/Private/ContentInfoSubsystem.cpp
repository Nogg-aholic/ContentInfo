#include "ContentInfoSubsystem.h"


#include "AssetRegistryModule.h"
#include "FGWorkBench.h"
#include "AvailabilityDependencies/FGSchematicPurchasedDependency.h"
#include "Buildables/FGBuildableFactory.h"
#include "Equipment/FGBuildGun.h"
#include "Reflection/ReflectionHelper.h"
#include "Registry/ModContentRegistry.h"
#include "Resources/FGBuildDescriptor.h"
#include "Resources/FGConsumableDescriptor.h"
#include "Resources/FGItemDescriptorBiomass.h"
#include "Resources/FGItemDescriptorNuclearFuel.h"
#include "Resources/FGResourceDescriptor.h"
#include "Unlocks/FGUnlockRecipe.h"
#include "Unlocks/FGUnlockSchematic.h"


FNogs_ProductBuildingCost::FNogs_ProductBuildingCost()
{
}

FNogs_ProductBuildingCost::FNogs_ProductBuildingCost(TSubclassOf<UFGRecipe> InRecipe, TSubclassOf<UObject> InBuilding)
{
	Recipe = InRecipe;
	Building = InBuilding;
}

float FNogs_ProductBuildingCost::GetMjCost() const
{
	return GetMjCostForPotential(1.f);
}

float FNogs_ProductBuildingCost::GetMjCostForPotential(const float Potential) const
{
	if (Building)
	{
		if (Building->IsChildOf(AFGBuildableFactory::StaticClass()) && Recipe)
		{
			const float Power = Cast<AFGBuildableFactory>(Building.GetDefaultObject())->
				CalcProducingPowerConsumptionForPotential(Potential);
			const float TimeMod = FMath::Clamp(
				Potential, Cast<AFGBuildableFactory>(Building.GetDefaultObject())->GetMinPotential(),
				Cast<AFGBuildableFactory>(Building.GetDefaultObject())->GetMaxPossiblePotential());
			const float Duration = Recipe.GetDefaultObject()->mManufactoringDuration / TimeMod;
			return (Duration * Power);
		}
		else
			return 0.f;
	}
	else
		return 0.f;
}

FNogs_RecipeMJ::FNogs_RecipeMJ(): nRecipe()
{
}

FNogs_RecipeMJ::FNogs_RecipeMJ(TSubclassOf<UFGRecipe> Outer): nRecipe(Outer)
{
}

int32 FNogs_RecipeMJ::GetItemAmount(const TSubclassOf<UFGItemDescriptor> Item, bool Ingredient)
{
	TArray<TSubclassOf<class UFGItemDescriptor>> Out;
	TArray<FItemAmount> Arr = Ingredient ? nRecipe.GetDefaultObject()->GetIngredients(): nRecipe.GetDefaultObject()->GetProducts();
	for (auto i : Arr)
	{
		Out.Add(i.ItemClass);
	}
	if (!Out.Contains(Item))
	{
		UE_LOG(LogTemp, Error, TEXT("Item not part of this Recipe ! "));
		return 0.f;
	}
	return Arr[Out.Find(Item)].Amount;
}

bool FNogs_RecipeMJ::CanCalculateMj(UContentInfoSubsystem* System) const
{
	if (!System || !nRecipe)
		return false; 
	for (auto i : nRecipe.GetDefaultObject()->mIngredients)
	{
		if (!System->nItems.Find(i.ItemClass)->HasMj())
			return false;
	}
	return true;
}

bool FNogs_RecipeMJ::HasAssignedMJ() const
{
	if (MJ_Average != 0)
		return true;
	else
		return false;
}

void FNogs_RecipeMJ::AddValue(const float Value)
{
	MJ_Average += Value;
	if (Value != MJ_Average)
		MJ_Average /= 2;
}


bool FNogs_RecipeMJ::TryAssignMJ(UContentInfoSubsystem* System)
{
	if(!CanCalculateMj(System))
		return false;
	FNogs_Recipe & Recipe = *System->nRecipes.Find(nRecipe);
	float Sum = 0.f;
	bool Finished = true;
	for (auto n : nRecipe.GetDefaultObject()->mIngredients)
	{
		System->nItems.Find(n.ItemClass)->AssignAverageMj(System);
		if (System->nItems.Find(n.ItemClass)->HasMj())
		{
			Sum += System->nItems.Find(n.ItemClass)->MJValue * n.Amount;
		}
		else
			return false;
	}
	Recipe.MJ.AddValue(Sum);
	for (auto n : Recipe.Products())
	{
		System->nItems.Find(n)->AssignAverageMj(System);
	}
	return true;
	
}

float FNogs_RecipeMJ::GetAverageBuildingCost(const TArray<TSubclassOf<UObject>> Exclude) const
{
	float Sum2 = 0.f;
	int32 Count2 = 0;
	TArray<TSubclassOf<UObject>> Producers;
	nRecipe.GetDefaultObject()->GetProducedIn(Producers);
	for(auto & O: Producers)
	{
		if(Exclude.Contains(O))
			continue;
		FNogs_ProductBuildingCost e = FNogs_ProductBuildingCost(nRecipe,O);
		const float TempCost = e.GetMjCost();
		if(TempCost > 0)
		{
			Sum2 += TempCost;
			Count2++;
		}
	}
	if(Sum2 > 0 && Count2 > 0)
	{
		return (Sum2 / Count2);
	}
	return Sum2;
}


// TODO :: Product TO MJ Ratio 
float FNogs_RecipeMJ::GetProductMjValue(TSubclassOf<UFGItemDescriptor> Item,bool PerItem  , TSubclassOf<UObject> Buildable, bool ExcludeManual, float Potential)
{
	const int32 ItemAmount = GetItemAmount(Item,false);
	if (ItemAmount == 0 || Potential < 0 )
		return 0.f;

	TArray<TSubclassOf<UObject>> Producers;
	nRecipe.GetDefaultObject()->GetProducedIn(Producers);
	float BuildingCost = 0.f;
	if(!Buildable)
	{
		TArray<TSubclassOf<UObject>> Excludes;
		if(ExcludeManual)
			for(auto i : Producers)
				if (i->IsChildOf(UFGWorkBench::StaticClass()) || i->IsChildOf(AFGBuildGun::StaticClass()))
					Excludes.Add(i);
	
		BuildingCost = GetAverageBuildingCost(Excludes);
	}
	else
	{
		if (!Producers.Contains(Buildable))
		{
			UE_LOG(LogTemp, Error, TEXT("Building cannot Craft this !"));
		}
		else
		{
			const FNogs_ProductBuildingCost Prod = FNogs_ProductBuildingCost(nRecipe,Buildable);
			BuildingCost = Prod.GetMjCostForPotential(Potential);
		}
	}
	if(PerItem)
		return (MJ_Average + BuildingCost) / ItemAmount;
	else
		return (MJ_Average + BuildingCost);
}

FNogs_Descriptor::FNogs_Descriptor()
{
}

FNogs_Descriptor::FNogs_Descriptor(TSubclassOf<UFGItemDescriptor> InClass)
{
	ItemClass = InClass;
	AssignResourceValue();
}

FNogs_Descriptor::FNogs_Descriptor(TSubclassOf<UFGItemDescriptor> InClass, TSubclassOf<UFGRecipe> Recipe)
{
	IngredientInRecipe.Add(Recipe);
	ItemClass = InClass;
	AssignResourceValue();
}

float FNogs_Descriptor::GetMj(FNogs_Recipe Recipe,TSubclassOf<UObject> Buildable) const
{
	if (!ItemClass)
		return 0.f;
	if(Recipe.nRecipeClass)
	{
		return Recipe.MJ.GetProductMjValue(ItemClass,true,Buildable);
	}
	return MJValue;
}

void FNogs_Descriptor::AssignResourceValue()
{
	const auto i = ItemClass;
	auto& ProcessedItemStruct = *this;
	if (ItemClass->IsChildOf(UFGResourceDescriptor::StaticClass()))
	{
		if (i.GetDefaultObject()->GetName().Contains("Desc_OreIron"))
		{
			ProcessedItemStruct.SetMj(10.76f);
		}
		else if (i.GetDefaultObject()->GetName().Contains("Desc_OreCopper"))
		{
			ProcessedItemStruct.SetMj(11.62f);
		}
		else if (i.GetDefaultObject()->GetName().Contains("Desc_OreBauxite"))
		{
			ProcessedItemStruct.SetMj(11.17f);
		}
		else if (i.GetDefaultObject()->GetName().Contains("Desc_OreGold"))
		{
			ProcessedItemStruct.SetMj(8.49f);
		}
		else if (i.GetDefaultObject()->GetName().Contains("Desc_LiquidOil"))
		{
			ProcessedItemStruct.SetMj(.03332f);
		}
		else if (i.GetDefaultObject()->GetName().Contains("Desc_Stone"))
		{
			ProcessedItemStruct.SetMj(10.7f);
		}
		else if (i.GetDefaultObject()->GetName().Contains("Desc_NitrogenGas"))
		{
			ProcessedItemStruct.SetMj(.01949f);
		}
		else if (i.GetDefaultObject()->GetName().Contains("Desc_RawQuartz"))
		{
			ProcessedItemStruct.SetMj(10.03f);
		}
		else if (i.GetDefaultObject()->GetName().Contains("Desc_Sulfur"))
		{
			ProcessedItemStruct.SetMj(10.84f);
		}
		else if (i.GetDefaultObject()->GetName().Contains("Desc_OreUranium"))
		{
			ProcessedItemStruct.SetMj(14.85f);
		}
		else if (i.GetDefaultObject()->mForm == EResourceForm::RF_SOLID && !i.GetDefaultObject()->GetName().Contains(
            "Desc_CrystalShard"))
            	ProcessedItemStruct.SetMj(10.f);
		else if (i.GetDefaultObject()->mForm == EResourceForm::RF_LIQUID || i.GetDefaultObject()->mForm ==
            EResourceForm::RF_GAS)
            	ProcessedItemStruct.SetMj(.01f);

	}
	else if (ItemClass->IsChildOf(UFGItemDescriptorBiomass::StaticClass()))
	{
		ProcessedItemStruct.SetMj(ItemClass.GetDefaultObject()->mEnergyValue);
	}
	else if(ItemClass->IsChildOf(UFGConsumableDescriptor::StaticClass()))
	{
		ProcessedItemStruct.SetMj(10.f);
	}
	else if(ItemClass->IsChildOf(UFGItemDescriptorNuclearFuel::StaticClass()))
	{
		//Cast<UFGItemDescriptorNuclearFuel>(ItemClass.GetDefaultObject())->GetSpentFuelClass()
	}
}


void FNogs_Descriptor::SetMj(const float Value, bool Override)
{
	MJValue = Value;
}

bool FNogs_Descriptor::HasMj() const
{
	if (!ItemClass)
		return false;

	return MJValue != -1;
}

float FNogs_Descriptor::AssignAverageMj(UContentInfoSubsystem* System, const TArray<TSubclassOf<UFGRecipe>> Exclude, const TArray<TSubclassOf<UObject>> ExcludeBuilding)
{

	if (ItemClass->IsChildOf(UFGResourceDescriptor::StaticClass()))
	{
		AssignResourceValue();
		return MJValue;
	}
	else
	{
		float IngredientCost = 0.f;
		int32 Count = 0;
		for (auto i : ProductInRecipe)
		{
			if(Exclude.Contains(i))
				continue;
			
			auto& Recipe = *System->nRecipes.Find(i);
			if (Recipe.MJ.HasAssignedMJ())
			{
				IngredientCost += Recipe.MJ.GetProductMjValue(ItemClass)* Recipe.GetItemToTotalProductRatio(ItemClass,System);
				Count++;
				//UE_LOG(LogTemp, Error, TEXT("%s in Recipe %s IngredientCost-> %f"), *ItemClass->GetName(),*i->GetName(),IngredientCost /Count);
			}
		}
		if (Count > 0 && IngredientCost > 0)
		{
			MJValue = (IngredientCost / Count);
		}
		return MJValue;
	}
	
};


void FNogs_Descriptor::RecurseIngredients(TSubclassOf<class UFGItemDescriptor> Item, TArray<TSubclassOf<class UFGItemDescriptor>> & AllItems , TArray<TSubclassOf<class UFGRecipe>> & AllRecipes ,UContentInfoSubsystem * System, bool SkipAlternate, TArray<TSubclassOf<class UFGRecipe>> Excluded, bool UseFirst)
{
	if(!System)
		return;
	
	const int32 Len = AllRecipes.Num(); 
	for(auto i : System->nItems.Find(Item)->ProductInRecipe)
	{
		if(!Excluded.Contains(i))
			continue;
		if(!AllRecipes.Contains(i))
			AllRecipes.Add(i);
		FNogs_Recipe Recipe = *System->nRecipes.Find(i); 
		if(Recipe.UnlockedFromAlternate() && SkipAlternate)
			continue;
		
		for(auto e: Recipe.Ingredients())
		{
			if(!AllRecipes.Contains(i))
				System->nItems.Find(e)->RecurseIngredients(e, AllItems,AllRecipes,System, SkipAlternate,Excluded);
			if(!AllItems.Contains(e))
				AllItems.Add(e);
		}
		if(UseFirst && Len != AllRecipes.Num())
			return;
	}
};

void UContentInfoSubsystem::CalculateCost(TArray<TSubclassOf<UFGRecipe>> RecipesToCalc)
{
	UE_LOG(LogTemp, Display, TEXT("******************** Content Info MJ Calculation %i Recipes to Calculate ********************"), RecipesToCalc.Num());
	int32 CounterInside = 0;
	for (auto Recipe : RecipesToCalc)
	{
		FNogs_Recipe & ItemItr = *nRecipes.Find(Recipe);
		if(ItemItr.MJ.TryAssignMJ(this))
			CounterInside++;
	}
}

TMap<TSubclassOf<UFGItemDescriptor>, FNogs_Descriptor>  UContentInfoSubsystem::CalculateRecipesRecursively(const TSubclassOf<UFGItemDescriptor> Item,
                                                                                                           const TArray<TSubclassOf<UFGRecipe>> Exclude, const bool UseAlternates,UContentInfoSubsystem * System)
{
	TMap<TSubclassOf<UFGItemDescriptor>,FNogs_Descriptor>  Map;
	TArray<TSubclassOf<UFGRecipe>> Recipes;
	TArray<TSubclassOf<UFGItemDescriptor>> RecipesItem;
	FNogs_Descriptor::RecurseIngredients(Item,RecipesItem,Recipes,System,UseAlternates,Exclude);
	
	for(auto i : RecipesItem)
	{
		System->nItems.Find(i)->MJValue = -1;
	}
	for(auto i : Recipes)
	{
		System->nRecipes.Find(i)->MJ.MJ_Average = 0;
	}
	CalculateCost(Recipes);
	for(auto i : RecipesItem)
	{
		FNogs_Descriptor & InValue = *System->nItems.Find(i);
		Map.Add(i,InValue);
	}
	return Map;
};


void UContentInfoSubsystem::SortPairs(TArray<TSubclassOf<UObject>> & Array_To_Sort_Keys,TArray<float> & Array_To_Sort_Values, const bool Descending)
{
	const int32 m = Array_To_Sort_Keys.Num();      // Return the array size
	
	for (int32 a = 0; a < (m - 1); a++)
	{
		bool bDidSwap = false;

		if (Descending == true)         // If element 1 is less than element 2 move it back in the array (sorts high to low)
		{
			for (int32 k = 0; k < m - a - 1; k++)
			{
				if(Array_To_Sort_Values[k] < Array_To_Sort_Values[k + 1])
				{
					const float z = Array_To_Sort_Values[k];
					const auto Ob = Array_To_Sort_Keys[k];
					Array_To_Sort_Values[k] = Array_To_Sort_Values[k + 1];
					Array_To_Sort_Keys[k] = Array_To_Sort_Keys[k + 1];
					Array_To_Sort_Values[k + 1] = z;
					Array_To_Sort_Keys[k + 1] = Ob;
					bDidSwap = true;
				}
			}

			if (bDidSwap == false)      // If no swaps occured array is sorted do not go through last loop
			{
				break;
			}
		}
		else
		{
			for (int32 k = 0; k < m - a - 1; k++)
			{
				if(Array_To_Sort_Values[k] > Array_To_Sort_Values[k + 1])
				{
					const float z = Array_To_Sort_Values[k];
					const auto Ob = Array_To_Sort_Keys[k];
					Array_To_Sort_Values[k] = Array_To_Sort_Values[k + 1];
					Array_To_Sort_Keys[k] = Array_To_Sort_Keys[k + 1];
					Array_To_Sort_Values[k + 1] = z;
					Array_To_Sort_Keys[k + 1] = Ob;
					bDidSwap = true;
				}
			}

			if (bDidSwap == false)      // If no swaps occured array is sorted do not go through last loop
			{
				break;
			}
		}
	}
}

void UContentInfoSubsystem::PrintSortedRecipes()
{
	TArray<TSubclassOf<UObject>> Array_To_Sort_Keys;
	TArray<float> Array_To_Sort_Values;
	for(auto i: nRecipes)
	{
		if(i.Value.MJ.HasAssignedMJ())
		{
			Array_To_Sort_Keys.Add(i.Key);
			Array_To_Sort_Values.Add(i.Value.MJ.MJ_Average);
		}
	}
	SortPairs(Array_To_Sort_Keys,Array_To_Sort_Values);
	for(int32 i = 0; i< Array_To_Sort_Keys.Num(); i++)
	{
		UE_LOG(LogTemp, Display,TEXT("Recipe: %s MJ: %f"), *Array_To_Sort_Keys[i]->GetName(),Array_To_Sort_Values[i]);
	}
}

void UContentInfoSubsystem::PrintSortedItems()
{
	TArray<TSubclassOf<UObject>> Array_To_Sort_Keys;
	TArray<float> Array_To_Sort_Values;
	for(auto i: nItems)
	{
		if(i.Value.HasMj())
		{
			Array_To_Sort_Keys.Add(i.Key);
			Array_To_Sort_Values.Add(i.Value.MJValue);
		}
	}
	SortPairs(Array_To_Sort_Keys,Array_To_Sort_Values);
	for(int32 i = 0; i< Array_To_Sort_Keys.Num(); i++)
	{
		UE_LOG(LogTemp,Display,TEXT("Item: %s MJ: %f"), *Array_To_Sort_Keys[i]->GetName(),Array_To_Sort_Values[i]);
	}
};

void UContentInfoSubsystem::FullRecipeCalculation()
{
	TArray<TSubclassOf<UFGRecipe>> NormalRecipes;
	TArray<TSubclassOf<UFGRecipe>> AlternateRecipes;
	TArray<TSubclassOf<UFGRecipe>> BuildingRecipes;
	TArray<TSubclassOf<UFGRecipe>> ManualOnly;

	for(auto i : nRecipes)
	{
		if(i.Value.IsBuildGunRecipe())
			BuildingRecipes.Add(i.Key);
		else if(i.Value.IsManualOnly())
			ManualOnly.Add(i.Key);
		else if(i.Value.UnlockedFromAlternate())
			AlternateRecipes.Add(i.Key);
		else
		{
			NormalRecipes.Add(i.Key);
		}
	}

	// do a bunch of times to average them out
	for(int32 i = 0 ; i < 10; i++)
		CalculateCost(NormalRecipes);
	// once for generating , another time to average
	for(int32 i = 0 ; i < 2; i++)
	CalculateCost(AlternateRecipes);
	CalculateCost(ManualOnly);
	CalculateCost(BuildingRecipes);
	
	UE_LOG(LogTemp, Display,TEXT("-----------------------------------------------------"));
	UE_LOG(LogTemp, Display,TEXT("___________________Results Recipes___________________"));

	PrintSortedRecipes();
	UE_LOG(LogTemp, Display,TEXT("_____________________________________________________"));
	UE_LOG(LogTemp, Display,TEXT("-----------------------------------------------------"));
	UE_LOG(LogTemp, Display,TEXT("____________________Results Items____________________"));

	PrintSortedItems();
	UE_LOG(LogTemp, Display,TEXT("_____________________________________________________"));
	UE_LOG(LogTemp, Display,TEXT("-----------------------------------------------------"));
}

void UContentInfoSubsystem::ClientInit()
{
	TArray<TSubclassOf<class UFGSchematic>> toProcess;
	TArray<UClass*> Arr;
	GetDerivedClasses(UFGSchematic::StaticClass(), Arr, true);
	for(auto i : Arr)
	{
		HandleSchematic(i);
	}

	static FStructProperty* NodeDataStructProperty = nullptr;
	static FClassProperty* SchematicStructProperty = nullptr;
	static UClass* ResearchTreeNodeClass = nullptr;

	ResearchTreeNodeClass = LoadClass<UFGResearchTreeNode>(
        nullptr, TEXT("/Game/FactoryGame/Schematics/Research/BPD_ResearchTreeNode.BPD_ResearchTreeNode_C"));
	if (ResearchTreeNodeClass)
	{
		TArray<UClass*> Trees;
		GetDerivedClasses(UFGResearchTree::StaticClass(), Arr, true);
		for(auto i : Trees)
		{
			NodeDataStructProperty = FReflectionHelper::FindPropertyChecked<FStructProperty>(
            ResearchTreeNodeClass, TEXT("mNodeDataStruct"));
			SchematicStructProperty = FReflectionHelper::FindPropertyByShortNameChecked<FClassProperty>(
                NodeDataStructProperty->Struct, TEXT("Schematic"));
			const TArray<UFGResearchTreeNode*> Nodes = UFGResearchTree::GetNodes(i);
			for (UFGResearchTreeNode* Node : Nodes)
			{
				if (!Node)
					continue;
				TSubclassOf<UFGSchematic> Schematic = Cast<UClass>(
                    SchematicStructProperty->GetPropertyValue_InContainer(
                        NodeDataStructProperty->ContainerPtrToValuePtr<void>(Node)));
				if (!Schematic)
					continue;
				nSchematics.Add(Schematic, HandleResearchTreeSchematic(Schematic, i));
			}
		}	
	}

	FullRecipeCalculation();
}

int32 FNogs_Descriptor::CalculateDepth(UContentInfoSubsystem * System, const TSubclassOf<UFGItemDescriptor> Item)
{
	if (Item)
	{
		const TArray<TSubclassOf<UFGRecipe>> Exc;
		TArray<TSubclassOf<UFGRecipe>> Recipes;
		TArray<TSubclassOf<UFGItemDescriptor>> RecipesItem;
		RecurseIngredients(Item,RecipesItem,Recipes,System,false,Exc,true);
		return Recipes.Num();
	}
	return 0 ;
}

FNogs_Schematic UContentInfoSubsystem::HandleSchematic(const TSubclassOf<class UFGSchematic> Schematic)
{
	return nSchematics.Add(Schematic, FNogs_Schematic(Schematic, this));
}

void UContentInfoSubsystem::AddToSchematicArrayProp(UPARAM(ref)FNogs_Schematic& Obj,
                                                    const TSubclassOf<UFGSchematic> Schematic, const int32 Index)
{
	if (Index == 0)
	{
		Obj.nUnlockedBy.Add(Schematic);
	}
	else if (Index == 1)
	{
		Obj.nDependsOn.Add(Schematic);
	}
	else if (Index == 2)
	{
		Obj.nDependingOnThis.Add(Schematic);
	}
	else if (Index == 3)
	{
		Obj.nVisibilityDepOn.Add(Schematic);
	}
	else if (Index == 4)
	{
		Obj.nVisibilityDepOnThis.Add(Schematic);
	}
}

UContentInfoSubsystem* UContentInfoSubsystem::CustomGet(const UObject* WorldContextObject)
{
	UWorld* const World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (World)
	{
		UClass* Class = LoadClass<UContentInfoSubsystem>(
			nullptr,TEXT("/ContentInfo/BP_ContentInfoSubsystem.BP_ContentInfoSubsystem_C"));
		if (Class)
			return Cast<UContentInfoSubsystem>(World->GetSubsystemBase(Class));
		else
			return nullptr;
	}
	return nullptr;
}

FNogs_Recipe::FNogs_Recipe()
{
	nRecipeClass = nullptr;
	nUnlockedBy = {};
};

FNogs_Recipe::FNogs_Recipe(const TSubclassOf<UFGRecipe> Class, const FNogs_Schematic Schematic)
{
	nRecipeClass = Class;
	nUnlockedBy.Add(Schematic.nClass);
	MJ = FNogs_RecipeMJ(Class);
}

TArray<float> FNogs_Recipe::GetIngredientsForProductRatio(const TSubclassOf<UFGItemDescriptor> Item) const
{
	const auto CDO = nRecipeClass.GetDefaultObject();
	TArray<float> Array;
	for (const auto i : CDO->mIngredients)
	{
		if(i.ItemClass != Item)
			continue;
		for (const auto e : CDO->mProduct)
		{
			Array.Add(e.Amount / i.Amount);
		}
	}
	return Array;
}

float FNogs_Recipe::GetItemToTotalProductRatio(TSubclassOf<UFGItemDescriptor> Item, UContentInfoSubsystem* System ) const
{
	
	const auto CDO = nRecipeClass.GetDefaultObject();
	if(CDO->mProduct.Num() > 1)
	{
		if(MJ.HasAssignedMJ())
		{
			for (const auto i : CDO->mProduct)
			{
				if(i.ItemClass != Item)
					continue;
				const FNogs_Descriptor & product = *System->nItems.Find(i.ItemClass);
				if(product.HasMj())
					return FMath::Clamp((product.MJValue * i.Amount) / MJ.MJ_Average,0.f,1.f);
				else
					return 1.f;
			}
			// 10000 Cost -> product is 300 * 10 -> 3000/10000 -> 0.3
		}
	}
	return 1.f;
}

void FNogs_Recipe::DiscoverMachines(UContentInfoSubsystem* System ) const
{
	TArray<TSubclassOf<UObject>> BuildClasses;
	nRecipeClass.GetDefaultObject()->GetProducedIn(BuildClasses);
	BuildClasses.Remove(nullptr);
	if (Products().IsValidIndex(0))
	{
		for (auto j : BuildClasses)
		{
			TArray<UClass*> Arr;
			GetDerivedClasses(j, Arr,true);
			if(!Arr.Contains(j) && !j->IsNative())
			{
				Arr.Add(j);
			}
			for(auto h : Arr)
			{
				if (h->IsChildOf(AFGBuildGun::StaticClass()) && Products()[0]->IsChildOf(UFGBuildDescriptor::StaticClass()))
				{
					TSubclassOf<UFGBuildDescriptor> Desc = *Products()[0];
					TSubclassOf<AFGBuildable> Buildable;
					Buildable = Desc.GetDefaultObject()->GetBuildClass(Desc);
					if (!System->nBuildGunBuildings.Contains(*Desc))
						System->nBuildGunBuildings.Add(Buildable, *Desc);
				}
			}
		}
	}
}

void FNogs_Recipe::DiscoverItem(UContentInfoSubsystem* System ) const
{
	for (auto k : Ingredients())
	{
		FNogs_Descriptor Item = FNogs_Descriptor(k, nRecipeClass);
		if (System->nItems.Contains(k))
			System->nItems.Find(k)->IngredientInRecipe.Add(nRecipeClass);
		else
			System->nItems.Add(k, Item);
	}

	for (auto k : Products())
	{
		FNogs_Descriptor Item = FNogs_Descriptor(k);
		if (System->nItems.Contains(k))
			System->nItems.Find(k)->ProductInRecipe.Add(nRecipeClass);
		else
		{
			Item.ProductInRecipe.Add(nRecipeClass);
			System->nItems.Add(k, Item);
		}
	}
}

bool FNogs_Recipe::IsManualOnly() const
{
	if(!IsManual())
		return false;
	
	TArray<TSubclassOf<UObject>> arr;
	nRecipeClass.GetDefaultObject()->GetProducedIn(arr);
	for(auto e : arr)
		if(e && !e->IsChildOf(UFGWorkBench::StaticClass()))
			return false;
	return true;
}

bool FNogs_Recipe::IsManual() const
{
	TArray<TSubclassOf<UObject>> arr;
	nRecipeClass.GetDefaultObject()->GetProducedIn(arr);
	for(auto e : arr)
		if(e->IsChildOf(UFGWorkBench::StaticClass()))
			return true;

	return false;
}
bool FNogs_Recipe::UnlockedFromAlternate()
{
	for(auto e : nUnlockedBy)
		if(e.GetDefaultObject()->mType == ESchematicType::EST_Alternate)
			return true;

	return false;
}
bool FNogs_Recipe::IsBuildGunRecipe() const
{
	TArray<TSubclassOf<UObject>> arr;
	nRecipeClass.GetDefaultObject()->GetProducedIn(arr);
	for(auto e : arr)
		if(e->IsChildOf(AFGBuildGun::StaticClass()))
			return true;

	return false;
}
TArray<TSubclassOf<UFGItemDescriptor>> FNogs_Recipe::Products() const
{
	TArray<TSubclassOf<class UFGItemDescriptor>> out;
	TArray<FItemAmount> Arr = nRecipeClass.GetDefaultObject()->GetProducts();
	for (auto i : Arr)
	{
		out.Add(i.ItemClass);
	}
	return out;
}

TArray<TSubclassOf<UFGItemDescriptor>> FNogs_Recipe::Ingredients() const
{
	TArray<TSubclassOf<class UFGItemDescriptor>> out;
	TArray<FItemAmount> Arr = nRecipeClass.GetDefaultObject()->GetIngredients();
	for (auto i : Arr)
	{
		out.Add(i.ItemClass);
	}
	return out;
}

TArray<TSubclassOf<UFGItemCategory>> FNogs_Recipe::ProductCats() const
{
	TArray<TSubclassOf<class UFGItemCategory>> Out;
	TArray<FItemAmount> Arr = nRecipeClass.GetDefaultObject()->GetProducts();
	for (auto i : Arr)
	{
		if (!Out.Contains(i.ItemClass.GetDefaultObject()->GetItemCategory(i.ItemClass)))
			Out.Add(i.ItemClass.GetDefaultObject()->GetItemCategory(i.ItemClass));
	}
	return Out;
}

TArray<TSubclassOf<UFGItemCategory>> FNogs_Recipe::IngredientCats() const
{
	TArray<TSubclassOf<class UFGItemCategory>> Out;
	TArray<FItemAmount> Arr = nRecipeClass.GetDefaultObject()->GetIngredients();
	for (auto i : Arr)
	{
		if (!Out.Contains(i.ItemClass.GetDefaultObject()->GetItemCategory(i.ItemClass)))
			Out.Add(i.ItemClass.GetDefaultObject()->GetItemCategory(i.ItemClass));
	}
	return Out;
}

FNogs_Schematic::FNogs_Schematic()
{
}

FNogs_Schematic::FNogs_Schematic(TSubclassOf<UFGSchematic> inClass, UContentInfoSubsystem* System)
{
	nClass = inClass;
	DiscoverUnlocks(System);
}

void FNogs_Schematic::DiscoverUnlocks(UContentInfoSubsystem* System)
{
	GatherDependencies();

	TArray<UFGUnlock*> Unlocks = nClass.GetDefaultObject()->GetUnlocks(nClass);
	for (auto i : Unlocks)
	{
		// Recipe unlocks make struct for it and save Buildings found
		if (Cast<UFGUnlockRecipe>(i))
		{
			TArray<TSubclassOf<class UFGRecipe>> UnlockRecipes = Cast<UFGUnlockRecipe>(i)->GetRecipesToUnlock();

			for (auto j : UnlockRecipes)
			{
				if (!j)
					continue;

				if (!System->nRecipes.Contains(j))
				{
					FNogs_Recipe rep = FNogs_Recipe(j, *this);
					rep.DiscoverItem(System);
					rep.DiscoverMachines(System);
					System->nRecipes.Add(j, rep);
				}
				else
				{
					System->nRecipes.Find(j)->nUnlockedBy.Add(nClass);
				}
			}
		} // schematics unlocks cause recursion
		else if (Cast<UFGUnlockSchematic>(i))
		{
			TArray<TSubclassOf<class UFGSchematic>> UnlockSchematics = Cast<UFGUnlockSchematic>(i)->GetSchematicsToUnlock();

			for (auto j : UnlockSchematics)
			{
				if (!System->nSchematics.Contains(j))
				{
					System->HandleSchematic(j);
				}
			}
		}
	}
}

void FNogs_Schematic::GatherDependencies()
{
	TArray<UFGAvailabilityDependency*> Out_SchematicDependencies;
	nClass.GetDefaultObject()->GetSchematicDependencies(nClass, Out_SchematicDependencies);
	for (auto i : Out_SchematicDependencies)
	{
		if (!i)
			continue;
		TArray<TSubclassOf<class UFGSchematic>> Out_Schematics;
		const UFGSchematicPurchasedDependency* Dep = Cast<UFGSchematicPurchasedDependency>(i);
		if (Dep)
		{
			Dep->GetSchematics(Out_Schematics);
			for (auto j : Out_Schematics)
			{
				if (!j)
					continue;
				if (!nDependsOn.Contains(j))
					nDependsOn.Add(j);
			}
		}
	}
}
