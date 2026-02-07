// Copyright (c) Jared Taylor

#include "Ability/AbilityTask_PlayMontageProAdvancedAndWait.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemLog.h"
#include "AbilitySystemGlobals.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AbilityTask_PlayMontageProAdvancedAndWait)

static bool GUseAggressivePlayMontageProAdvancedAndWaitEndTask = true;
static FAutoConsoleVariableRef CVarAggressivePlayMontageProAdvancedAndWaitEndTask(TEXT("AbilitySystem.PlayMontageProAdvanced.AggressiveEndTask"), GUseAggressivePlayMontageProAdvancedAndWaitEndTask, TEXT("This should be set to true in order to avoid multiple callbacks off an AbilityTask_PlayMontageProAdvancedAndWait node"));

static bool GPlayMontageProAdvancedAndWaitFireInterruptOnAnimEndInterrupt = true;
static FAutoConsoleVariableRef CVarPlayMontageProAdvancedAndWaitFireInterruptOnAnimEndInterrupt(TEXT("AbilitySystem.PlayMontageProAdvanced.FireInterruptOnAnimEndInterrupt"), GPlayMontageProAdvancedAndWaitFireInterruptOnAnimEndInterrupt, TEXT("This is a fix that will cause AbilityTask_PlayMontageProAdvancedAndWait to fire its Interrupt event if the underlying AnimInstance ends in an interrupted"));

void UAbilityTask_PlayMontageProAdvancedAndWait::OnMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted)
{
	UPlayMontageProStatics::EnsureBroadcastNotifyEvents(
		bInterrupted ? EAnimNotifyProEventType::OnInterrupted : EAnimNotifyProEventType::BlendOut, Notifies, NotifyStatePairs, this);
	
	const bool bPlayingThisMontage = (Montage == MontageToPlay) && Ability && Ability->GetCurrentMontage() == MontageToPlay;
	if (bPlayingThisMontage)
	{
		// Reset AnimRootMotionTranslationScale
		ACharacter* Character = Cast<ACharacter>(GetAvatarActor());
		if (Character && (Character->GetLocalRole() == ROLE_Authority || (Character->GetLocalRole() == ROLE_AutonomousProxy &&
			Ability->GetNetExecutionPolicy() == EGameplayAbilityNetExecutionPolicy::LocalPredicted)))
		{
			Character->SetAnimRootMotionTranslationScale(1.f);
		}
	}

	if (bPlayingThisMontage && (bInterrupted || !bAllowInterruptAfterBlendOut))
	{
		if (UAbilitySystemComponent* ASC = AbilitySystemComponent.Get())
		{
			ASC->ClearAnimatingAbility(Ability);
		}
	}

	if (ShouldBroadcastAbilityTaskDelegates())
	{
		if (bInterrupted)
		{
            bAllowInterruptAfterBlendOut = false;
			OnInterrupted.Broadcast(FGameplayTag(), FGameplayEventData());

			if (GUseAggressivePlayMontageProAdvancedAndWaitEndTask)
			{
				EndTask();
			}
		}
		else
		{
			OnBlendOut.Broadcast(FGameplayTag(), FGameplayEventData());
		}
	}
}

void UAbilityTask_PlayMontageProAdvancedAndWait::OnMontageBlendedIn(UAnimMontage* Montage)
{
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnBlendedIn.Broadcast(FGameplayTag(), FGameplayEventData());
	}
}

void UAbilityTask_PlayMontageProAdvancedAndWait::OnGameplayAbilityCancelled()
{
	UPlayMontageProStatics::EnsureBroadcastNotifyEvents(EAnimNotifyProEventType::OnInterrupted, Notifies, NotifyStatePairs, this);
	
	if (StopPlayingMontage(OverrideBlendOutTimeOnCancelAbility) || bAllowInterruptAfterBlendOut)
	{
		// Let the BP handle the interrupt as well
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			bAllowInterruptAfterBlendOut = false;
			OnInterrupted.Broadcast(FGameplayTag(), FGameplayEventData());
		}
	}

	if (GUseAggressivePlayMontageProAdvancedAndWaitEndTask)
	{
		EndTask();
	}
}

void UAbilityTask_PlayMontageProAdvancedAndWait::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	UPlayMontageProStatics::EnsureBroadcastNotifyEvents(
		bInterrupted ? EAnimNotifyProEventType::OnInterrupted : EAnimNotifyProEventType::OnCompleted, Notifies, NotifyStatePairs, this);
	
	if (!bInterrupted)
	{
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnCompleted.Broadcast(FGameplayTag(), FGameplayEventData());
		}
	}
	else if (bAllowInterruptAfterBlendOut && GPlayMontageProAdvancedAndWaitFireInterruptOnAnimEndInterrupt)
	{
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnInterrupted.Broadcast(FGameplayTag(), FGameplayEventData());
		}
	}

	EndTask();
}

UAbilityTask_PlayMontageProAdvancedAndWait* UAbilityTask_PlayMontageProAdvancedAndWait::
CreatePlayMontageProAdvancedAndWaitProxy(UGameplayAbility* OwningAbility, FName TaskInstanceName,
	FGameplayTagContainer EventTags, UAnimMontage* MontageToPlay, bool bDrivenMontagesMatchDriverDuration, float Rate,
	FName StartSection, FProNotifyParams ProNotifyParams, bool bOverrideBlendIn, FMontageBlendSettings BlendInOverride,
	bool bStopWhenAbilityEnds, float AnimRootMotionTranslationScale, float StartTimeSeconds,
	bool bAllowInterruptAfterBlendOut, float OverrideBlendOutTimeOnCancelAbility,
	float OverrideBlendOutTimeOnEndAbility)
{
	UAbilitySystemGlobals::NonShipping_ApplyGlobalAbilityScaler_Rate(Rate);

	UAbilityTask_PlayMontageProAdvancedAndWait* MyObj = NewAbilityTask<UAbilityTask_PlayMontageProAdvancedAndWait>(OwningAbility, TaskInstanceName);
	MyObj->EventTags = EventTags;
	MyObj->MontageToPlay = MontageToPlay;
	MyObj->bDrivenMontagesMatchDriverDuration = bDrivenMontagesMatchDriverDuration;
	MyObj->Rate = Rate;
	MyObj->StartSection = StartSection;
	MyObj->ProNotifyParams = ProNotifyParams;
	MyObj->bOverrideBlendIn = bOverrideBlendIn;
	MyObj->BlendInOverride = BlendInOverride;
	MyObj->AnimRootMotionTranslationScale = AnimRootMotionTranslationScale;
	MyObj->bStopWhenAbilityEnds = bStopWhenAbilityEnds;
	MyObj->bAllowInterruptAfterBlendOut = bAllowInterruptAfterBlendOut;
	MyObj->StartTimeSeconds = StartTimeSeconds;
	MyObj->OverrideBlendOutTimeOnCancelAbility = OverrideBlendOutTimeOnCancelAbility;
	MyObj->OverrideBlendOutTimeOnEndAbility = OverrideBlendOutTimeOnEndAbility;
	
	return MyObj;
}

void UAbilityTask_PlayMontageProAdvancedAndWait::Activate()
{
	if (Ability == nullptr)
	{
		return;
	}

	bool bPlayedMontage = false;

	if (UAbilitySystemComponent* ASC = AbilitySystemComponent.Get())
	{
		const FGameplayAbilityActorInfo* ActorInfo = Ability->GetCurrentActorInfo();
		UAnimInstance* AnimInstance = ActorInfo->GetAnimInstance();
		if (AnimInstance != nullptr)
		{
			// Bind to event callback
			EventHandle = ASC->AddGameplayEventTagContainerDelegate(EventTags,
				FGameplayEventTagMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::OnGameplayEvent));

			if (ASC->PlayMontage(Ability, Ability->GetCurrentActivationInfo(), MontageToPlay, Rate, StartSection, StartTimeSeconds) > 0.f)
			{
				// Playing a montage could potentially fire off a callback into game code which could kill this ability! Early out if we are  pending kill.
				if (ShouldBroadcastAbilityTaskDelegates() == false)
				{
					return;
				}

				InterruptedHandle = Ability->OnGameplayAbilityCancelled.AddUObject(this, &ThisClass::OnGameplayAbilityCancelled);

				BlendedInDelegate.BindUObject(this, &ThisClass::OnMontageBlendedIn);
				AnimInstance->Montage_SetBlendedInDelegate(BlendedInDelegate, MontageToPlay);

				BlendingOutDelegate.BindUObject(this, &ThisClass::OnMontageBlendingOut);
				AnimInstance->Montage_SetBlendingOutDelegate(BlendingOutDelegate, MontageToPlay);

				MontageEndedDelegate.BindUObject(this, &ThisClass::OnMontageEnded);
				AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, MontageToPlay);

				ACharacter* Character = Cast<ACharacter>(GetAvatarActor());
				if (Character && (Character->GetLocalRole() == ROLE_Authority ||
								  (Character->GetLocalRole() == ROLE_AutonomousProxy && Ability->GetNetExecutionPolicy() == EGameplayAbilityNetExecutionPolicy::LocalPredicted)))
				{
					Character->SetAnimRootMotionTranslationScale(AnimRootMotionTranslationScale);
				}

				bPlayedMontage = true;

				if (ProNotifyParams.bEnableProNotifies)
				{
					// -- PlayMontagePro --
			
					// Use the mesh comp's OnTickPose to detect time dilation changes
					if (ProNotifyParams.bEnableCustomTimeDilation && GetMesh() && ActorInfo->AvatarActor.IsValid())
					{
						TimeDilation = ActorInfo->AvatarActor->CustomTimeDilation;
						TickPoseHandle = GetMesh()->OnTickPose.AddUObject(this, &ThisClass::OnTickPose);
					}
					else
					{
						TimeDilation = 1.f;
					}

					if (StartSection != NAME_None)
					{
						// PlayMontagePro needs to update StartingPosition to account for the section jump
						const float NewPosition = AnimInstance->Montage_GetPosition(MontageToPlay);
						StartTimeSeconds += (NewPosition - StartTimeSeconds);
					}

					// Handle section changes
					AnimInstance->OnMontageSectionChanged.AddDynamic(this, &ThisClass::OnMontageSectionChanged);

					// Gather notifies from montage
					const FName Section = AnimInstance->Montage_GetCurrentSection(MontageToPlay);
					UPlayMontageProStatics::GatherNotifies(this, MontageToPlay, NotifyId, Notifies, NotifyStatePairs, Section, StartTimeSeconds, TimeDilation);

					// Trigger notifies before start time and remove them, if we want to trigger them before the start time
					UPlayMontageProStatics::HandleHistoricNotifies(Notifies, NotifyStatePairs, ProNotifyParams.bTriggerNotifiesBeforeStartTime, StartTimeSeconds, this);

					// Create timer delegates for notifies
					UPlayMontageProStatics::SetupNotifyTimers(this, GetWorld(), Notifies);
				}
			}
		}
		else
		{
			ABILITY_LOG(Warning, TEXT("UAbilityTask_PlayMontageProAdvancedAndWait call to PlayMontagePro failed!"));
		}
	}
	else
	{
		ABILITY_LOG(Warning, TEXT("UAbilityTask_PlayMontageProAdvancedAndWait called on invalid AbilitySystemComponent"));
	}

	if (!bPlayedMontage)
	{
		ABILITY_LOG(Warning, TEXT("UAbilityTask_PlayMontageProAdvancedAndWait called in Ability %s failed to play montage %s; Task Instance Name %s."), *Ability->GetName(), *GetNameSafe(MontageToPlay),*InstanceName.ToString());
		UPlayMontageProStatics::EnsureBroadcastNotifyEvents(EAnimNotifyProEventType::OnCancelled, Notifies, NotifyStatePairs, this);
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnCancelled.Broadcast(FGameplayTag(), FGameplayEventData());
		}
	}

	SetWaitingOnAvatar();
}

void UAbilityTask_PlayMontageProAdvancedAndWait::ExternalCancel()
{
	UPlayMontageProStatics::EnsureBroadcastNotifyEvents(EAnimNotifyProEventType::OnCancelled, Notifies, NotifyStatePairs, this);
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnCancelled.Broadcast(FGameplayTag(), FGameplayEventData());
	}
	Super::ExternalCancel();
}

UAnimMontage* UAbilityTask_PlayMontageProAdvancedAndWait::GetMontage() const
{
	return IsValid(MontageToPlay) ? MontageToPlay : nullptr;
}

USkeletalMeshComponent* UAbilityTask_PlayMontageProAdvancedAndWait::GetMesh() const
{
	const bool bValidMesh = Ability && Ability->GetCurrentActorInfo() && Ability->GetCurrentActorInfo()->SkeletalMeshComponent.IsValid();
	return bValidMesh ? Ability->GetCurrentActorInfo()->SkeletalMeshComponent.Get() : nullptr;
}

void UAbilityTask_PlayMontageProAdvancedAndWait::OnGameplayEvent(FGameplayTag EventTag,
	const FGameplayEventData* Payload)
{
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		FGameplayEventData TempData = *Payload;
		TempData.EventTag = EventTag;

		OnEventReceived.Broadcast(EventTag, TempData);
	}
}

void UAbilityTask_PlayMontageProAdvancedAndWait::OnMontageSectionChanged(UAnimMontage* InMontage, FName SectionName,
	bool bLooped)
{
	if (!ShouldBroadcastAbilityTaskDelegates() || !IsValid(MontageToPlay) || InMontage != MontageToPlay)
	{
		return;
	}

	const FGameplayAbilityActorInfo* ActorInfo = Ability->GetCurrentActorInfo();
	const UAnimInstance* AnimInstance = ActorInfo ? ActorInfo->GetAnimInstance() : nullptr;
	
	if (!ActorInfo || !AnimInstance)
	{
		return;
	}

	const float StartTime = AnimInstance->Montage_GetPosition(InMontage);

	// End previous notify timers
	UPlayMontageProStatics::ClearNotifyTimers(GetWorld(), Notifies);

	// Gather notifies from montage
	UPlayMontageProStatics::GatherNotifies(this, InMontage, NotifyId, Notifies, NotifyStatePairs, SectionName, StartTime, TimeDilation);

	// Create timer delegates for notifies
	UPlayMontageProStatics::SetupNotifyTimers(this, GetWorld(), Notifies);
}

void UAbilityTask_PlayMontageProAdvancedAndWait::OnTickPose(USkinnedMeshComponent* SkinnedMeshComponent, float DeltaTime,
	bool NeedsValidRootMotion)
{
	UPlayMontageProStatics::HandleTimeDilation(this, SkinnedMeshComponent, TimeDilation, Notifies);
}

void UAbilityTask_PlayMontageProAdvancedAndWait::OnDestroy(bool AbilityEnded)
{
	UPlayMontageProStatics::EnsureBroadcastNotifyEvents(EAnimNotifyProEventType::OnCompleted, Notifies, NotifyStatePairs, this);
	
	if (TickPoseHandle.IsValid() && GetMesh())
	{
		if (TickPoseHandle.IsValid() && GetMesh()->OnTickPose.IsBoundToObject(this))
		{
			GetMesh()->OnTickPose.Remove(TickPoseHandle);
		}
	}
	
	// Note: Clearing montage end delegate isn't necessary since its not a multicast and will be cleared when the next montage plays.
	// (If we are destroyed, it will detect this and not do anything)

	// This delegate, however, should be cleared as it is a multicast
	if (Ability)
	{
		Ability->OnGameplayAbilityCancelled.Remove(InterruptedHandle);
		if (AbilityEnded && bStopWhenAbilityEnds)
		{
			StopPlayingMontage(OverrideBlendOutTimeOnEndAbility);
		}
	}

	if (UAbilitySystemComponent* ASC = AbilitySystemComponent.IsValid() ? AbilitySystemComponent.Get() : nullptr)
	{
		ASC->RemoveGameplayEventTagContainerDelegate(EventTags, EventHandle);
	}

	Super::OnDestroy(AbilityEnded);
}

bool UAbilityTask_PlayMontageProAdvancedAndWait::StopPlayingMontage(float OverrideBlendOutTime)
{
	if (Ability == nullptr)
	{
		return false;
	}

	const FGameplayAbilityActorInfo* ActorInfo = Ability->GetCurrentActorInfo();
	if (ActorInfo == nullptr)
	{
		return false;
	}

	const UAnimInstance* AnimInstance = ActorInfo->GetAnimInstance();
	if (AnimInstance == nullptr)
	{
		return false;
	}

	// Check if the montage is still playing
	// The ability would have been interrupted, in which case we should automatically stop the montage
	UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	if (ASC && Ability)
	{
		if (ASC->GetAnimatingAbility() == Ability
			&& ASC->GetCurrentMontage() == MontageToPlay)
		{
			// Unbind delegates so they don't get called as well
			FAnimMontageInstance* MontageInstance = AnimInstance->GetActiveInstanceForMontage(MontageToPlay);
			if (MontageInstance)
			{
				MontageInstance->OnMontageBlendedInEnded.Unbind();
				MontageInstance->OnMontageBlendingOutStarted.Unbind();
				MontageInstance->OnMontageEnded.Unbind();
			}

			ASC->CurrentMontageStop();
			return true;
		}
	}

	return false;
}

FString UAbilityTask_PlayMontageProAdvancedAndWait::GetDebugString() const
{
	const UAnimMontage* PlayingMontage = nullptr;
	if (Ability)
	{
		const FGameplayAbilityActorInfo* ActorInfo = Ability->GetCurrentActorInfo();
		const UAnimInstance* AnimInstance = ActorInfo->GetAnimInstance();

		if (AnimInstance != nullptr)
		{
			PlayingMontage = AnimInstance->Montage_IsActive(MontageToPlay) ? ToRawPtr(MontageToPlay) : AnimInstance->GetCurrentActiveMontage();
		}
	}

	return FString::Printf(TEXT("PlayMontageProAdvancedAndWait. MontageToPlay: %s  (Currently Playing): %s"), *GetNameSafe(MontageToPlay), *GetNameSafe(PlayingMontage));
}

