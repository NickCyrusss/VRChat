// Copyright 2019 313 Studios. All Rights Reserved.


#include "VoipAudioComponent.h"
#include "Net/VoiceConfig.h"
#include "Voice/Public/Voice.h"

UVoipAudioComponent::UVoipAudioComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UVoipAudioComponent::BeginPlay()
{
	Super::BeginPlay();

	VoiceDecoder = FVoiceModule::Get().CreateVoiceDecoder();

	OpenPacketStream(UVOIPStatics::GetVoiceSampleRate(), UVOIPStatics::GetNumBufferedPackets(), UVOIPStatics::GetBufferingDelay());

	ResetBuffer(UVOIPStatics::GetVoiceSampleRate(), UVOIPStatics::GetBufferingDelay());
	Initialize(UVOIPStatics::GetVoiceSampleRate());
	//Start();

	bVoiceStarted = true;
}

void UVoipAudioComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction * ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (IsIdling() && IsActive())
	{
		// Stop when we're idle to free up for other streaming audio sources
		// this tells the synth to stop on the render thread, must use bVoiceGenerating to make sure it has stopped
		Stop();
		
		SetActive(false);
		UncompressedVoiceData.Empty();
	}
	
}


void UVoipAudioComponent::PlayVoiceData(const TArray<uint8>& CompressedVoiceData)
{
	if (!bVoiceStarted)
	{
		return;
	}

	if (!IsPlaying() || !IsActive())
	{
		if (bVoiceGenerating)
		{
			// if the synth is still generating voice, then don't try to restart voice
			// this will give errors, and cause a packet buffer overflow
			return;
		}

		ResetBuffer(UVOIPStatics::GetVoiceSampleRate(), UVOIPStatics::GetBufferingDelay());

		Start();
	}


	const int32 NumCompressedBytes = CompressedVoiceData.Num();

	if (VoiceDecoder.IsValid())
	{
		uint32 BytesWritten = UVOIPStatics::GetMaxUncompressedVoiceDataSizePerChannel();

		UncompressedVoiceData.Empty(UVOIPStatics::GetMaxUncompressedVoiceDataSizePerChannel());
		UncompressedVoiceData.AddUninitialized(UVOIPStatics::GetMaxUncompressedVoiceDataSizePerChannel());

		// Decompress the data
		VoiceDecoder->Decode(CompressedVoiceData.GetData(), NumCompressedBytes, UncompressedVoiceData.GetData(), BytesWritten);

		// If there is no data, return
		if (BytesWritten <= 0)
		{
			return;
		}

		// submit the array to the synth component for playback
		SubmitPacket((float*)UncompressedVoiceData.GetData(), BytesWritten, UVOIPStatics::GetVoiceSampleRate(), EVoipStreamDataFormat::Int16);
	}
}

void UVoipAudioComponent::OnEndGenerate()
{
	Super::OnEndGenerate();
	bVoiceGenerating = false;
}

void UVoipAudioComponent::OnBeginGenerate()
{
	Super::OnBeginGenerate();
	bVoiceGenerating = true;
}
