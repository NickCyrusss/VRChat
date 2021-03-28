// Copyright 2019 313 Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VoipManagerComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FVoiceGenerated, const TArray<uint8>&, VoiceData, const float, MicLevel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPlayerStartTalking);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPlayerStopTalking);

class AController;

/*
 Generates voice data and sends it to blueprints for processing
*/
UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class EASYVOICECHAT_API UVoipManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	

	UVoipManagerComponent();

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/* Call this to start the voice capture. Ensures voice only starts on a local player machine 
	   @return true if capture started successfully
	*/
	UFUNCTION(BlueprintCallable, Category = "Voip Manager")
		bool InitVoice(AController* Controller);

	/* Called when voice data is generated, passes an array of compressed voice data to blueprints. Use on inherited components */
	UFUNCTION(BlueprintImplementableEvent, Category = "Voip Manager")
		void OnVoiceGeneratedBP(const TArray<uint8> &VoiceBuffer, const float MicLevel);

	/* Called on the client when the player starts talking */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "Voip Manager")
		void OnPlayerStartTalking();

	/* Called on the client when the player stops talking */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "Voip Manager")
		void OnPlayerStopTalking();

	/* Delegate called when voice is generated */
	UPROPERTY(BlueprintAssignable, Category = "VOIP")
		FVoiceGenerated VoiceGenerated;

	/* Called when the player starts talking */
	UPROPERTY(BlueprintAssignable, Category = "VOIP")
		FPlayerStartTalking PlayerTalking;

	/* Called when the player stops talking */
	UPROPERTY(BlueprintAssignable, Category = "VOIP")
		FPlayerStopTalking PlayerStopTalking;

private:

	/* Capture interfaces */
	TSharedPtr<class IVoiceCapture> VoiceCapture;
	TSharedPtr<class IVoiceEncoder> VoiceEncoder;
	TSharedPtr<class IVoiceDecoder> VoiceDecoder;

	/* Voice buffers */
	TArray<uint8> DecompressedVoiceBuffer;
	TArray<uint8> CompressedVoiceBuffer;

	/* Internally used buffer which is adjusted to the size of the encoded, and is passed to blueprints */
	TArray<uint8> OutCompressedVoiceBuffer;

	/* Used internally for recieving voice data */
	TArray<uint8> DecodedVoiceBuffer;

	/* Remainder of voice data carried over if the encode buffer has leftovers */
	TArray<uint8> VoiceRemainder;

	/* The size of the remaining voice buffer */
	uint32 VoiceRemainderSize;

	/* The size of the compressed voice buffer */
	uint32 CompressedBytesAvailable;

	/* The time in seconds this voip was last transmitted */
	float LastSeen = 0.0f;

	/* The threshold in which Stop Talking event will be called after transmission to compensate for buffering */
	UPROPERTY(EditDefaultsOnly, Category="VOIP")
		float StopTalkingThreshold = 0.2f;
	
	/* Set to true whenever the player is talking */
	bool bVoiceActive;
};
