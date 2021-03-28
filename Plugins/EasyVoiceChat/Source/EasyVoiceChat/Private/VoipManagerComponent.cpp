// Copyright 2019 313 Studios. All Rights Reserved.


#include "VoipManagerComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Voice/Public/VoiceModule.h"
#include "Voice/Public/Interfaces/VoiceCodec.h"
#include "Engine/Player.h"

#define MAX_VOICE_REMAINDER_SIZE 1 * 1024

UVoipManagerComponent::UVoipManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

// Called every frame
void UVoipManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!VoiceCapture.IsValid() || !VoiceEncoder.IsValid())
	{
		return;
	}
	
	// Empty the voice buffers
	DecompressedVoiceBuffer.Empty(UVOIPStatics::GetMaxUncompressedVoiceDataSizePerChannel());
	CompressedVoiceBuffer.Empty(UVOIPStatics::GetMaxCompressedVoiceDataSize());

	uint32 NewVoiceDataBytes = 0;
	EVoiceCaptureState::Type VoiceResult = VoiceCapture->GetCaptureState(NewVoiceDataBytes);

	if (!bVoiceActive && NewVoiceDataBytes == 0)
	{
		// No capture data present, stop processing
		return;
	}
	uint32 TotalVoiceBytes = NewVoiceDataBytes + VoiceRemainderSize;

	if (TotalVoiceBytes > UVOIPStatics::GetMaxUncompressedVoiceDataSizePerChannel())
	{
		TotalVoiceBytes = UVOIPStatics::GetMaxUncompressedVoiceDataSizePerChannel();
	}

	DecompressedVoiceBuffer.AddUninitialized(TotalVoiceBytes);

	// If there's still audio left from a previous ReadLocalData call that didn't get output, copy that first into the decompressed voice buffer
	if (VoiceRemainderSize > 0)
	{
		FMemory::Memcpy(DecompressedVoiceBuffer.GetData(), VoiceRemainder.GetData(), VoiceRemainderSize);
	}

	// Get new uncompressed data
	uint8* RemainingDecompressedBufferPtr = DecompressedVoiceBuffer.GetData() + VoiceRemainderSize;
	uint32 ByteWritten = 0;
	VoiceResult = VoiceCapture->GetVoiceData(DecompressedVoiceBuffer.GetData() + VoiceRemainderSize, NewVoiceDataBytes, ByteWritten);

	TotalVoiceBytes = ByteWritten + VoiceRemainderSize;

	if ((VoiceResult == EVoiceCaptureState::Ok || VoiceResult == EVoiceCaptureState::NoData) && TotalVoiceBytes > 0)
	{
		// Prepare the encoded buffer (e.g. opus)
		CompressedBytesAvailable = UVOIPStatics::GetMaxCompressedVoiceDataSize();
		CompressedVoiceBuffer.AddUninitialized(UVOIPStatics::GetMaxCompressedVoiceDataSize());

		check(((uint32)CompressedVoiceBuffer.Num()) <= UVOIPStatics::GetMaxCompressedVoiceDataSize());

		// Run the uncompressed audio through the opus decoder, note that it may not encode all data, which results in some remaining data
		VoiceRemainderSize = VoiceEncoder->Encode(DecompressedVoiceBuffer.GetData(), TotalVoiceBytes, CompressedVoiceBuffer.GetData(), CompressedBytesAvailable);

		// Save off any unencoded remainder
		if (VoiceRemainderSize > 0)
		{
			if (VoiceRemainderSize > MAX_VOICE_REMAINDER_SIZE)
			{
				VoiceRemainderSize = MAX_VOICE_REMAINDER_SIZE;
			}

			VoiceRemainder.AddUninitialized(MAX_VOICE_REMAINDER_SIZE);
			FMemory::Memcpy(VoiceRemainder.GetData(), DecompressedVoiceBuffer.GetData() + (TotalVoiceBytes - VoiceRemainderSize), VoiceRemainderSize);
		}
	}

	if (VoiceResult != EVoiceCaptureState::Ok || TotalVoiceBytes == 0)
	{
		if (bVoiceActive)
		{
			// get the current time in seconds
			const double CurTime = FPlatformTime::Seconds();

			// Work out the time since this player last talked
			double TimeSince = CurTime - LastSeen;

			if (TimeSince >= StopTalkingThreshold)
			{
				// Notify blueprints that we've stopped talking
				OnPlayerStopTalking();
				PlayerStopTalking.Broadcast();
				bVoiceActive = false;
			}
		}
	}

	// Pass the data to blueprints if there is voice data
	if (CompressedBytesAvailable > 0)
	{
		// update the last seen time, used for the talking threshold
		LastSeen = FPlatformTime::Seconds();
		if (!bVoiceActive)
		{
			OnPlayerStartTalking();
			PlayerTalking.Broadcast();
			bVoiceActive = true;
		}

		// copy the encoded values into a new array, which only has encoded bytes in
		// This prevents copying the entire array, which has lots of unused space and will just saturate the actor channel
		OutCompressedVoiceBuffer.Empty(CompressedBytesAvailable);
		OutCompressedVoiceBuffer.AddUninitialized(CompressedBytesAvailable);
		FMemory::Memcpy(OutCompressedVoiceBuffer.GetData(), CompressedVoiceBuffer.GetData(), CompressedBytesAvailable);

		const float MicLevel = 1/*VoiceCapture.Get()->GetCurrentAmplitude()*/;

		VoiceGenerated.Broadcast(OutCompressedVoiceBuffer, MicLevel);

		OnVoiceGeneratedBP(OutCompressedVoiceBuffer, MicLevel);
	}
}

bool UVoipManagerComponent::InitVoice(AController* Controller)
{
	if (Controller)
	{
		FVoiceModule& VoiceModule = FVoiceModule::Get();

		APlayerController* PlayerController = Cast<APlayerController>(Controller);

		if (!PlayerController)
		{
			return false;
		}

		if (Controller->GetNetOwningPlayer())
		{
			Controller->GetNetOwningPlayer()->ConsoleCommand("voice.playback.ShouldResync 0");
		}

		// Only create voice capture on local clients
		if (VoiceModule.IsVoiceEnabled() && Controller->IsLocalController())
		{
			VoiceCapture = FVoiceModule::Get().CreateVoiceCapture();
			VoiceEncoder = FVoiceModule::Get().CreateVoiceEncoder();
			VoiceDecoder = FVoiceModule::Get().CreateVoiceDecoder();

			bool bSuccess = VoiceCapture.IsValid() && VoiceEncoder.IsValid();

			if (bSuccess)
			{
				VoiceCapture->Start();

				CompressedVoiceBuffer.Empty(UVOIPStatics::GetMaxCompressedVoiceDataSize());
				DecompressedVoiceBuffer.Empty(UVOIPStatics::GetMaxUncompressedVoiceDataSizePerChannel());

				return true;
			}
		}
	}

	return false;
}


