// Copyright 2019 313 Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "OnlineSubsystemUtils/Public/VoipListenerSynthComponent.h"
#include "VoipAudioComponent.generated.h"

/**
  Decodes and plays compressed voice data passed over the network
 */
UCLASS(Blueprintable, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class EASYVOICECHAT_API UVoipAudioComponent : public UVoipListenerSynthComponent
{
	GENERATED_BODY()

public:

	UVoipAudioComponent();
	
	virtual void BeginPlay() override;

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/* Plays compressed voice data */
	UFUNCTION(BlueprintCallable, Category = "Voip Audio")
		void PlayVoiceData(const TArray<uint8> &CompressedVoiceData);

protected:

	virtual void OnEndGenerate() override;
	virtual void OnBeginGenerate() override;

private:

	/* Voice buffer used by this audio component */
	TArray<uint8> UncompressedVoiceData;

	/* Voice decoder interface */
	TSharedPtr<class IVoiceDecoder> VoiceDecoder;

	/* Component is active when the audio component is registered */
	bool bVoiceStarted = false;

	/* Used to check if the audio is being generated on the audio render thread */
	bool bVoiceGenerating = false;

};
