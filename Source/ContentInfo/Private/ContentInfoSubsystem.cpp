#include "ContentInfoSubsystem.h"
#include "AvailabilityDependencies/FGSchematicPurchasedDependency.h"
#include "Buildables/FGBuildableFactory.h"
#include "Buildables/FGBuildableManufacturer.h"
#include "Buildables/FGBuildableResourceExtractor.h"
#include "Equipment/FGBuildGun.h"
#include "Reflection/ReflectionHelper.h"
#include "Registry/ModContentRegistry.h"
#include "Resources/FGBuildDescriptor.h"
#include "Resources/FGResourceDescriptor.h"
#include "Unlocks/FGUnlockRecipe.h"
#include "Unlocks/FGUnlockSchematic.h"


FNogs_Descriptor::FNogs_Descriptor(): DepthToResourceClass(0), MJValue(0)
{
}

FNogs_Descriptor::FNogs_Descriptor(const TSubclassOf<UFGItemDescriptor> InClass): ItemClass(InClass),
	DepthToResourceClass(0), MJValue(0)
{
	
}

FNogs_Descriptor::FNogs_Descriptor(const TSubclassOf<UFGRecipe> Recipe,
                                   const TSubclassOf<UFGItemDescriptor> InClass): ItemClass(InClass),
	DepthToResourceClass(0), MJValue(0)
{
	IngredientInRecipe.Add(Recipe);
}

FNogs_Descriptor::FNogs_Descriptor(const TSubclassOf<UFGItemDescriptor> InClass,
                                   const TSubclassOf<UFGRecipe> Recipe): ItemClass(InClass), DepthToResourceClass(0),
                                                                         MJValue(0)
{
	ProductInRecipe.Add(Recipe);
}


void UContentInfoSubsystem::CalculateCost()
{
	TArray<TSubclassOf<class UFGItemDescriptor>> StartWith;
	for (auto i : nItems)
		if (i.Value.ItemClass->IsChildOf(UFGResourceDescriptor::StaticClass()))
			StartWith.Add(i.Value.ItemClass);

	for (auto i : StartWith)
	{
		if (i.GetDefaultObject()->GetName().Contains("Desc_OreIron"))
		{
			nItems.Find(i)->MJValue = 10.76f;
		}
		else if (i.GetDefaultObject()->GetName().Contains("Desc_OreCopper"))
		{
			nItems.Find(i)->MJValue = 11.62f;
		}
		else if (i.GetDefaultObject()->GetName().Contains("Desc_OreBauxite"))
		{
			nItems.Find(i)->MJValue = 11.17f;
		}
		else if (i.GetDefaultObject()->GetName().Contains("Desc_OreGold"))
		{
			nItems.Find(i)->MJValue = 8.49f;
		}
		else if (i.GetDefaultObject()->GetName().Contains("Desc_LiquidOil"))
		{
			nItems.Find(i)->MJValue = .03332f;
		}
		else if (i.GetDefaultObject()->GetName().Contains("Desc_Stone"))
		{
			nItems.Find(i)->MJValue = 10.7f;
		}
		else if (i.GetDefaultObject()->GetName().Contains("Desc_NitrogenGas"))
		{
			nItems.Find(i)->MJValue = .01949f;
		}
		else if (i.GetDefaultObject()->GetName().Contains("Desc_RawQuartz"))
		{
			nItems.Find(i)->MJValue = 10.03f;
		}
		else if (i.GetDefaultObject()->GetName().Contains("Desc_Sulfur"))
		{
			nItems.Find(i)->MJValue = 10.84f;
		}
		else if (i.GetDefaultObject()->GetName().Contains("Desc_OreUranium"))
		{
			nItems.Find(i)->MJValue = 14.85f;
		}
		else if (i.GetDefaultObject()->mForm == EResourceForm::RF_SOLID)
			nItems.Find(i)->MJValue = 10.f;
		else if (i.GetDefaultObject()->mForm == EResourceForm::RF_LIQUID || i.GetDefaultObject()->mForm ==
			EResourceForm::RF_GAS)
			nItems.Find(i)->MJValue = .01f;
	}
	TArray<TSubclassOf<class UFGItemDescriptor>> ProcessNext;
	TArray<TSubclassOf<class UFGItemDescriptor>> AlreadyProcessed;

	while (StartWith.Num() > 0)
	{
		ProcessNext.Append(StartWith);
		for (auto ItemBeingProcessed : StartWith)
		{
			FNogs_Descriptor& ProcessedItemStruct = *nItems.Find(ItemBeingProcessed);
			for (TSubclassOf<class UFGRecipe> RecipeUsingThis : ProcessedItemStruct.IngredientInRecipe)
			{
				TArray<FItemAmount> Ingredients = RecipeUsingThis.GetDefaultObject()->GetIngredients();
				TArray<FItemAmount> Products = RecipeUsingThis.GetDefaultObject()->GetProducts();
				TArray<TSubclassOf<UObject>> Producers;
				FNogs_Recipe & RecipeUsingThisStruct = *nRecipes.Find(RecipeUsingThis);
				RecipeUsingThis.GetDefaultObject()->GetProducedIn(Producers);

				// Only add newly discovered things
				for (auto k : Products)
					if (!ProcessNext.Contains(k.ItemClass) && !AlreadyProcessed.Contains(k.ItemClass))
					{
						ProcessNext.Add(k.ItemClass);
						AlreadyProcessed.Add(k.ItemClass);
					}
				
				for (auto Buildable : Producers)
				{

					if (!Buildable)
						return;
					if (!Buildable->IsChildOf(AFGBuildableFactory::StaticClass()))
					{
						UE_LOG(LogTemp, Error, TEXT("WHAT IS THIS ? %s "), *Buildable->GetPathName());
						continue;
					}
					TSubclassOf<AFGBuildableFactory> Building = Buildable;
					float Speed = 1.f;
					if (Buildable->IsChildOf(AFGBuildableManufacturer::StaticClass()))
					{
						TSubclassOf<AFGBuildableManufacturer> Manufacturer = Buildable;
						Speed = Manufacturer.GetDefaultObject()->GetManufacturingSpeed();
					}
					float CycleTime = (RecipeUsingThis.GetDefaultObject()->mManufactoringDuration/Speed);

					float Power = 1.f;
					if (Buildable->IsChildOf(AFGBuildableFactory::StaticClass()))
						Power = Building.GetDefaultObject()->mPowerConsumption;

					Power = Power * CycleTime;
					for (auto k : Ingredients)
					{
						FNogs_Descriptor& RecipeIngredient = *nItems.Find(k.ItemClass);
						Power += RecipeIngredient.MJValue;
					}
					
					for (auto k : Products)
					{
						const float CalculatedMJ = FMath::Clamp(Power, 0.0001f, 99999999999.f) / k.Amount;

						if (k.ItemClass->IsChildOf(UFGResourceDescriptor::StaticClass()))
						{
							float& OldMj = nItems.Find(k.ItemClass)->MJProduced;
							if (OldMj == 0)
							{
								OldMj += CalculatedMJ;
							}
							else
							{
								OldMj += CalculatedMJ;
								OldMj = OldMj / 2;
							}
						}
						else
						{
							float& OldMj = nItems.Find(k.ItemClass)->MJValue;
							if (OldMj == 0)
							{
								OldMj += CalculatedMJ;
							}
							else
							{
								OldMj += CalculatedMJ;
								OldMj = OldMj / 2;
							}
						}
						
					}
				}
			}
		}
		for (auto i : StartWith)
		{
			ProcessNext.Remove(i);
		}
		StartWith.Empty();
		for (auto i : ProcessNext)
		{
			StartWith.Add(i);
		}
		ProcessNext.Empty();
	}
}

void UContentInfoSubsystem::ClientInit()
{
	TArray<TSubclassOf<class UFGSchematic>> toProcess;
	TArray<FSchematicRegistrationInfo> Schematics;
	TArray<FResearchTreeRegistrationInfo> Research;
	AModContentRegistry* ContentManager = AModContentRegistry::Get(GetWorld());

	if (ContentManager)
	{
		Schematics = ContentManager->GetRegisteredSchematics();
		Research = ContentManager->GetRegisteredResearchTrees();
	}
	else
	{
		UE_LOG(LogTemp, Fatal, TEXT("No AModContentRegistry Found. this was Called too Early !"));
		return;
	}


	for (auto i : Schematics)
	{
		HandleSchematic(i.RegisteredObject);
	}
	for (auto i : Research)
	{
		static FStructProperty* NodeDataStructProperty = nullptr;
		static FClassProperty* SchematicStructProperty = nullptr;
		static UClass* ResearchTreeNodeClass = nullptr;

		ResearchTreeNodeClass = LoadClass<UFGResearchTreeNode>(
			nullptr, TEXT("/Game/FactoryGame/Schematics/Research/BPD_ResearchTreeNode.BPD_ResearchTreeNode_C"));
		if (ResearchTreeNodeClass)
		{
			NodeDataStructProperty = FReflectionHelper::FindPropertyChecked<FStructProperty>(
				ResearchTreeNodeClass, TEXT("mNodeDataStruct"));
			SchematicStructProperty = FReflectionHelper::FindPropertyByShortNameChecked<FClassProperty>(
				NodeDataStructProperty->Struct, TEXT("Schematic"));
			const TArray<UFGResearchTreeNode*> Nodes = UFGResearchTree::GetNodes(i.RegisteredObject);
			for (UFGResearchTreeNode* Node : Nodes)
			{
				if (!Node)
					continue;
				TSubclassOf<UFGSchematic> Schematic = Cast<UClass>(
					SchematicStructProperty->GetPropertyValue_InContainer(
						NodeDataStructProperty->ContainerPtrToValuePtr<void>(Node)));
				if (!Schematic)
					continue;
				nSchematics.Add(Schematic, HandleResearchTreeSchematic(Schematic, i.RegisteredObject));
			}
		}
	}
	for (auto& i : nRecipes)
	{
		i.Value.CalculateInfo();
	}
	for (auto& i : nItems)
	{
		CalculateDepth(i.Value);
	}
	
	CalculateCost();
}

void UContentInfoSubsystem::CalculateDepth(FNogs_Descriptor& ItemObject)
{
	if (ItemObject.ItemClass)
	{
		int32 RecipeIndex = 0;
		for (auto e : ItemObject.ProductInRecipe)
		{
			ItemObject.DepthToResource.Add(RecipeIndex, TArray<float>());
			TArray<TSubclassOf<class UFGRecipe>> Iterated;
			for (auto Ingredient : e.GetDefaultObject()->mIngredients)
			{
				bool BreakOut = false;
				TArray<TSubclassOf<class UFGRecipe>> Recipes = nItems.Find(Ingredient.ItemClass)->ProductInRecipe;

				while (!BreakOut)
				{
					TArray<TSubclassOf<class UFGRecipe>> DiscoveredRecipe = {};

					float TimeSum = 0.f;
					for (auto Recipe : Recipes)
					{
						if (Iterated.Contains(Recipe))
							continue;
						for (auto l : Recipe.GetDefaultObject()->mIngredients)
						{
							for (auto ä : nItems.Find(l.ItemClass)->ProductInRecipe)
							{
								if (!Iterated.Contains(ä))
								{
									DiscoveredRecipe.Add(ä);
								}
							}
						}
						Iterated.Add(Recipe);

						TimeSum += Recipe.GetDefaultObject()->mManufactoringDuration;
					}

					if (DiscoveredRecipe.Num() == 0)
					{
						BreakOut = true;
					}
					else
					{
						ItemObject.DepthToResource[RecipeIndex].Add(TimeSum / Recipes.Num());
						Recipes = DiscoveredRecipe;
					}
				}
			}
			Iterated.Empty();
			RecipeIndex++;
		}
	}
}

FNogs_Schematic UContentInfoSubsystem::HandleSchematic(TSubclassOf<class UFGSchematic> Schematic)
{
	return nSchematics.Add(Schematic, FNogs_Schematic(Schematic, this));
}

void UContentInfoSubsystem::AddToSchematicArrayProp(UPARAM(ref)FNogs_Schematic& Obj,
                                                    TSubclassOf<UFGSchematic> Schematic, int32 Index)
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
		UClass * Class = LoadClass<UContentInfoSubsystem>(nullptr,TEXT("/ContentInfo/BP_ContentInfoSubsystem.BP_ContentInfoSubsystem_C"));
		if(Class)
			return Cast<UContentInfoSubsystem>(World->GetSubsystemBase(Class));
		else
			return nullptr;
	}	
	return nullptr;

}

void FNogs_Schematic::DiscoverDependendies()
{
	UContentInfoSubsystem* System = UContentInfoSubsystem::CustomGet(ContextObj);

	for (auto k : System->nSchematics)
	{
		TArray<UFGAvailabilityDependency*> Out_SchematicDependencies;
		k.Key.GetDefaultObject()->GetSchematicDependencies(k.Key, Out_SchematicDependencies);
		for (auto j : Out_SchematicDependencies)
		{
			if (!j)
				continue;
			if (Cast<UFGSchematicPurchasedDependency>(j))
			{
				TArray<TSubclassOf<class UFGSchematic>> Out_Schematics;
				Cast<UFGSchematicPurchasedDependency>(j)->GetSchematics(Out_Schematics);
				if (Out_Schematics.Contains(nClass))
				{
					if (!nDependingOnThis.Contains(k.Key))
					{
						nDependingOnThis.Add(k.Key);
					}
				}
			}
		}

		TArray<UFGUnlock*> Unlocks = k.Key.GetDefaultObject()->GetUnlocks(k.Key);
		for (auto i : Unlocks)
		{
			if (Cast<UFGUnlockSchematic>(i))
			{
				TArray<TSubclassOf<class UFGSchematic>> UnlockSchematics = Cast<UFGUnlockSchematic>(i)->
					GetSchematicsToUnlock();
				if (UnlockSchematics.Contains(nClass))
				{
					if (!nUnlockedBy.Contains(k.Key))
					{
						nUnlockedBy.Add(k.Key);
					}
				}
			}
		}
	}
}

FNogs_Recipe::FNogs_Recipe()
{
	nRecipeClass = nullptr;
	nUnlockedBy = {};
	ContextObj = nullptr;
};

FNogs_Recipe::FNogs_Recipe(const TSubclassOf<UFGRecipe> Class, FNogs_Schematic Schematic)
{
	nRecipeClass = Class;
	nUnlockedBy.Add(Schematic.nClass);
	ContextObj = Schematic.ContextObj;
}

void FNogs_Recipe::CalculateInfo()
{
	int32 idx = 0;
	const auto CDO = nRecipeClass.GetDefaultObject();
	IngredientToProductRatio.Empty();
	for (const auto i : CDO->mIngredients)
	{
		IngredientToProductRatio.Add(idx, TArray<float>());
		for (const auto e : CDO->mProduct)
		{
			IngredientToProductRatio[idx].Add(e.Amount / i.Amount);
		}
		idx++;
	}
}

void FNogs_Recipe::DiscoverMachines() const
{
	TArray<TSubclassOf<UObject>> BuildClasses;
	nRecipeClass.GetDefaultObject()->GetProducedIn(BuildClasses);
	BuildClasses.Remove(nullptr);
	if (Products().IsValidIndex(0))
	{
		UContentInfoSubsystem* System = UContentInfoSubsystem::CustomGet(ContextObj);

		for (auto h : BuildClasses)
		{
			if (h->IsChildOf(AFGBuildGun::StaticClass()) && Products()[0]->IsChildOf(UFGBuildDescriptor::StaticClass()))
			{
				TSubclassOf<UFGBuildDescriptor> Desc = *Products()[0];
				TSubclassOf<AFGBuildable> Buildable = Desc.GetDefaultObject()->GetBuildClass(Desc);
				if (!System->nBuildGunBuildings.Contains(*Desc))
					System->nBuildGunBuildings.Add(Buildable, *Desc);
			}
		}
	}
}

void FNogs_Recipe::DiscoverItem() const
{
	UContentInfoSubsystem* System = UContentInfoSubsystem::CustomGet(ContextObj);
	for (auto k : Ingredients())
	{
		FNogs_Descriptor Item = FNogs_Descriptor(nRecipeClass, k);
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

FNogs_Schematic::FNogs_Schematic(): ContextObj(nullptr)
{
}

FNogs_Schematic::FNogs_Schematic(TSubclassOf<UFGSchematic> inClass, UObject* ContextIn)
{
	nClass = inClass;
	ContextObj = ContextIn;
	DiscoverUnlocks();
}

void FNogs_Schematic::DiscoverUnlocks()
{
	GatherDependencies();

	TArray<UFGUnlock*> Unlocks = nClass.GetDefaultObject()->GetUnlocks(nClass);
	for (auto i : Unlocks)
	{
		// Recipe unlocks make struct for it and save Buildings found
		if (Cast<UFGUnlockRecipe>(i))
		{
			DiscoverRecipes(Cast<UFGUnlockRecipe>(i));
		} // schematics unlocks cause recursion
		else if (Cast<UFGUnlockSchematic>(i))
		{
			DiscoverSchematics(Cast<UFGUnlockSchematic>(i));
		}
	}
}

void FNogs_Schematic::DiscoverSchematics(UFGUnlockSchematic* Unlock) const
{
	TArray<TSubclassOf<class UFGSchematic>> UnlockSchematics = Unlock->GetSchematicsToUnlock();
	UContentInfoSubsystem* System = UContentInfoSubsystem::CustomGet(ContextObj);

	for (auto j : UnlockSchematics)
	{
		if (!System->nSchematics.Contains(j))
		{
			System->HandleSchematic(j);
		}
	}
}

void FNogs_Schematic::DiscoverRecipes(UFGUnlockRecipe* Unlock) const
{
	TArray<TSubclassOf<class UFGRecipe>> UnlockRecipes = Unlock->GetRecipesToUnlock();
	UContentInfoSubsystem* System = UContentInfoSubsystem::CustomGet(ContextObj);

	for (auto j : UnlockRecipes)
	{
		if (!j)
			continue;

		if (!System->nRecipes.Contains(j))
		{
			FNogs_Recipe rep = FNogs_Recipe(j, *this);
			System->nRecipes.Add(j, rep);
			rep.DiscoverItem();
			rep.DiscoverMachines();
		}
		else
		{
			(*System->nRecipes.Find(j)).nUnlockedBy.Add(nClass);
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
