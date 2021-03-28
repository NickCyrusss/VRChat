// Copyright 2019 313 Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "VoiceFunctionLibrary.generated.h"

UCLASS()
class EASYVOICECHAT_API UVoiceFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	/* Get player pawns from the game state. Optionally filter by a distance from the current player */
	UFUNCTION(BlueprintPure, Category = "Voice", meta = (WorldContext = "WorldContextObject"))
		static TArray<APawn*> GetAllPawnsFromState(UObject* WorldContextObject, APawn* CurrentPlayer, float Distance = 0.f);
	
};
