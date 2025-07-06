// Copyright (c) Jared Taylor

#include "PlayMontageProCallbackProxy.h"

#include "PlayMontageProStatics.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PlayMontageProCallbackProxy)

//////////////////////////////////////////////////////////////////////////
// UPlayMontageProCallbackProxy

UPlayMontageProCallbackProxy::UPlayMontageProCallbackProxy(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, MontageInstanceID(INDEX_NONE)
	, bInterruptedCalledBeforeBlendingOut(false)
{
}

UPlayMontageProCallbackProxy* UPlayMontageProCallbackProxy::CreateProxyObjectForPlayMontagePro(
	USkeletalMeshComponent* InSkeletalMeshComponent,
	UAnimMontage* MontageToPlay,
	float PlayRate,
	float StartingPosition,
	FName StartingSection,
	bool bTriggerNotifiesBeforeStartTime,
	bool bEnableCustomTimeDilation,
	bool bShouldStopAllMontages)
{
	UPlayMontageProCallbackProxy* Proxy = NewObject<UPlayMontageProCallbackProxy>();
	Proxy->SetFlags(RF_StrongRefOnFrame);
	Proxy->PlayMontagePro(InSkeletalMeshComponent, MontageToPlay, PlayRate, StartingPosition, StartingSection,
		bTriggerNotifiesBeforeStartTime, bEnableCustomTimeDilation, bShouldStopAllMontages);
	return Proxy;
}

bool UPlayMontageProCallbackProxy::PlayMontagePro(USkeletalMeshComponent* InSkeletalMeshComponent,
	UAnimMontage* MontageToPlay,
	float PlayRate,
	float StartingPosition,
	FName StartingSection,
	bool bTriggerNotifiesBeforeStartTime,
	bool bEnableCustomTimeDilation,
	bool bShouldStopAllMontages)
{
	MeshComp = InSkeletalMeshComponent;
	Montage = MontageToPlay;
	
	bool bPlayedSuccessfully = false;
	if (InSkeletalMeshComponent)
	{
		if (UAnimInstance* AnimInstance = InSkeletalMeshComponent->GetAnimInstance())
		{
			const float MontageLength = AnimInstance->Montage_Play(MontageToPlay, PlayRate, EMontagePlayReturnType::MontageLength, StartingPosition, bShouldStopAllMontages);
			bPlayedSuccessfully = MontageLength > 0.f;

			if (bPlayedSuccessfully)
			{
				// -- Engine default handling --
				
				AnimInstancePtr = AnimInstance;
				if (const FAnimMontageInstance* MontageInstance = AnimInstance->GetActiveInstanceForMontage(MontageToPlay))
				{
					MontageInstanceID = MontageInstance->GetInstanceID();
				}

				if (StartingSection != NAME_None)
				{
					AnimInstance->Montage_JumpToSection(StartingSection, MontageToPlay);

					// PlayMontagePro needs to update StartingPosition to account for the section jump
					const float NewPosition = AnimInstance->Montage_GetPosition(MontageToPlay);
					StartingPosition += (NewPosition - StartingPosition);
				}

				BlendingOutDelegate.BindUObject(this, &ThisClass::OnMontageBlendingOut);
				AnimInstance->Montage_SetBlendingOutDelegate(BlendingOutDelegate, MontageToPlay);

				MontageEndedDelegate.BindUObject(this, &ThisClass::OnMontageEnded);
				AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, MontageToPlay);

				// -- PlayMontagePro --
				
				// Use the mesh comp's OnTickPose to detect time dilation changes
				if (bEnableCustomTimeDilation)
				{
					TimeDilation = MeshComp->GetOwner()->CustomTimeDilation;
					TickPoseHandle = MeshComp->OnTickPose.AddUObject(this, &ThisClass::OnTickPose);
				}
				else
				{
					TimeDilation = 1.f;
				}

				// Handle section changes
				AnimInstance->OnMontageSectionChanged.AddDynamic(this, &ThisClass::OnMontageSectionChanged);

				// Gather notifies from montage
				const FName Section = AnimInstance->Montage_GetCurrentSection(MontageToPlay);
				UPlayMontageProStatics::GatherNotifies(MontageToPlay, NotifyId, Notifies, Section, StartingPosition, TimeDilation);

				// Trigger notifies before start time and remove them, if we want to trigger them before the start time
				UPlayMontageProStatics::HandleHistoricNotifies(Notifies, bTriggerNotifiesBeforeStartTime, this);

				// Create timer delegates for notifies
				UPlayMontageProStatics::SetupNotifyTimers(this, MeshComp->GetWorld(), Notifies);
			}
		}
	}

	if (!bPlayedSuccessfully)
	{
		OnInterrupted.Broadcast(NAME_None);
	}

	return bPlayedSuccessfully;
}

bool UPlayMontageProCallbackProxy::IsNotifyValid(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointNotifyPayload) const
{
	return ((MontageInstanceID != INDEX_NONE) && (BranchingPointNotifyPayload.MontageInstanceID == MontageInstanceID));
}

void UPlayMontageProCallbackProxy::OnMontageBlendingOut(UAnimMontage* InMontage, bool bInterrupted)
{
	if (bInterrupted)
	{
		UPlayMontageProStatics::EnsureBroadcastNotifyEvents(EAnimNotifyProEventType::OnInterrupted, Notifies, this);
		OnInterrupted.Broadcast(NAME_None);
		bInterruptedCalledBeforeBlendingOut = true;
	}
	else
	{
		UPlayMontageProStatics::EnsureBroadcastNotifyEvents(EAnimNotifyProEventType::BlendOut, Notifies, this);
		OnBlendOut.Broadcast(NAME_None);
	}
	bFinished = true;
}

void UPlayMontageProCallbackProxy::OnMontageEnded(UAnimMontage* InMontage, bool bInterrupted)
{
	if (!bInterrupted)
	{
		UPlayMontageProStatics::EnsureBroadcastNotifyEvents(EAnimNotifyProEventType::OnCompleted, Notifies, this);
		OnCompleted.Broadcast(NAME_None);
	}
	else if (!bInterruptedCalledBeforeBlendingOut)
	{
		UPlayMontageProStatics::EnsureBroadcastNotifyEvents(EAnimNotifyProEventType::OnInterrupted, Notifies, this);
		OnInterrupted.Broadcast(NAME_None);
	}
	
	UPlayMontageProStatics::ClearNotifyTimers(MeshComp->GetWorld(), Notifies);
	bFinished = true;
}

void UPlayMontageProCallbackProxy::OnMontageSectionChanged(UAnimMontage* InMontage, FName SectionName, bool bLooped)
{
	if (bFinished || !AnimInstancePtr.IsValid() || !Montage.IsValid() || InMontage != Montage || !MeshComp.IsValid() || !MeshComp->GetWorld())
	{
		return;
	}

	const float StartTime = AnimInstancePtr->Montage_GetPosition(InMontage);

	// End previous notify timers
	UPlayMontageProStatics::ClearNotifyTimers(MeshComp->GetWorld(), Notifies);

	// Gather notifies from montage
	UPlayMontageProStatics::GatherNotifies(InMontage, NotifyId, Notifies, SectionName, StartTime, TimeDilation);

	// Create timer delegates for notifies
	UPlayMontageProStatics::SetupNotifyTimers(this, MeshComp->GetWorld(), Notifies);
}

void UPlayMontageProCallbackProxy::OnTickPose(USkinnedMeshComponent* SkinnedMeshComponent, float DeltaTime,
	bool NeedsValidRootMotion)
{
	UPlayMontageProStatics::HandleTimeDilation(this, SkinnedMeshComponent, TimeDilation, Notifies);
}

void UPlayMontageProCallbackProxy::BeginDestroy()
{
	if (MeshComp.IsValid() && TickPoseHandle.IsValid() && MeshComp->OnTickPose.IsBoundToObject(this))
	{
		MeshComp->OnTickPose.Remove(TickPoseHandle);
	}
	
	Super::BeginDestroy();
}
