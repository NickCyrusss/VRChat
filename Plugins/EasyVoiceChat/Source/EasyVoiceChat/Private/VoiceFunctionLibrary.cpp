// Copyright 2019 313 Studios. All Rights Reserved.


#include "VoiceFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/KismetMathLibrary.h"

TArray<APawn*> UVoiceFunctionLibrary::GetAllPawnsFromState(UObject * WorldContextObject, APawn* CurrentPlayer, float Distance)
{
	AGameStateBase* GameState = UGameplayStatics::GetGameState(WorldContextObject);

	const bool bUseDistance = Distance > 0.f;

	FVector PlayerLocation;

	if (CurrentPlayer)
	{
		 PlayerLocation = CurrentPlayer->GetActorLocation();
	}

	APawn* Pawn;
	TArray<APawn*> PawnArray;

	if (GameState)
	{
		for (APlayerState* State : GameState->PlayerArray)
		{
			Pawn = State->GetPawn();
			
			if(Pawn)
			{
				if (bUseDistance)
				{
					// if using distance filtering, only add pawns that are below the user defined distance
					if (UKismetMathLibrary::Vector_Distance(Pawn->GetActorLocation(), PlayerLocation) <= Distance)
					{
						PawnArray.Add(Pawn);
					}
				}
				else
				{
					PawnArray.Add(Pawn);
				}

			}
		}
	}

	return PawnArray;
}
