// Copyright (c) Jared Taylor

#include "Ability/AbilityTask_PlayMontageProAndWait.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemLog.h"
#include "AbilitySystemGlobals.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AbilityTask_PlayMontageProAndWait)

static bool GUseAggressivePlayMontageProAndWaitEndTask = true;
static FAutoConsoleVariableRef CVarAggressivePlayMontageProAndWaitEndTask(TEXT("AbilitySystem.PlayMontagePro.AggressiveEndTask"), GUseAggressivePlayMontageProAndWaitEndTask, TEXT("This should be set to true in order to avoid multiple callbacks off an AbilityTask_PlayMontageProAndWait node"));

static bool GPlayMontageProAndWaitFireInterruptOnAnimEndInterrupt = true;
static FAutoConsoleVariableRef CVarPlayMontageProAndWaitFireInterruptOnAnimEndInterrupt(TEXT("AbilitySystem.PlayMontagePro.FireInterruptOnAnimEndInterrupt"), GPlayMontageProAndWaitFireInterruptOnAnimEndInterrupt, TEXT("This is a fix that will cause AbilityTask_PlayMontageProAndWait to fire its Interrupt event if the underlying AnimInstance ends in an interrupted"));

void UAbilityTask_PlayMontageProAndWait::OnMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted)
{
	UPlayMontageProStatics::EnsureBroadcastNotifyEvents(
		bInterrupted ? EAnimNotifyProEventType::OnInterrupted : EAnimNotifyProEventType::BlendOut, Notifies, this);
	
	const bool bPlayingThisMontage = (Montage == MontageToPlay) && Ability && Ability->GetCurrentMontage() == MontageToPlay;
	if (bPlayingThisMontage)
	{
		// Reset AnimRootMotionTranslationScale
		ACharacter* Character = Cast<ACharacter>(GetAvatarActor());
		if (Character && (Character->GetLocalRole() == ROLE_Authority ||
							(Character->GetLocalRole() == ROLE_AutonomousProxy && Ability->GetNetExecutionPolicy() == EGameplayAbilityNetExecutionPolicy::LocalPredicted)))
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
			OnInterrupted.Broadcast();

			if (GUseAggressivePlayMontageProAndWaitEndTask)
			{
				EndTask();
			}
		}
		else
		{
			OnBlendOut.Broadcast();
		}
	}
}

void UAbilityTask_PlayMontageProAndWait::OnMontageBlendedIn(UAnimMontage* Montage)
{
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnBlendedIn.Broadcast();
	}
}

void UAbilityTask_PlayMontageProAndWait::OnGameplayAbilityCancelled()
{
	UPlayMontageProStatics::EnsureBroadcastNotifyEvents(EAnimNotifyProEventType::OnInterrupted, Notifies, this);

	if (StopPlayingMontage() || bAllowInterruptAfterBlendOut)
	{
		// Let the BP handle the interrupt as well
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			bAllowInterruptAfterBlendOut = false;
			OnInterrupted.Broadcast();
		}
	}

	if (GUseAggressivePlayMontageProAndWaitEndTask)
	{
		EndTask();
	}
}

void UAbilityTask_PlayMontageProAndWait::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	UPlayMontageProStatics::EnsureBroadcastNotifyEvents(
		bInterrupted ? EAnimNotifyProEventType::OnInterrupted : EAnimNotifyProEventType::OnCompleted, Notifies, this);

	if (!bInterrupted)
	{
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnCompleted.Broadcast();
		}
	}
	else if (bAllowInterruptAfterBlendOut && GPlayMontageProAndWaitFireInterruptOnAnimEndInterrupt)
	{
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnInterrupted.Broadcast();
		}
	}

	EndTask();
}

UAbilityTask_PlayMontageProAndWait* UAbilityTask_PlayMontageProAndWait::CreatePlayMontageProAndWaitProxy(
	UGameplayAbility* OwningAbility, FName TaskInstanceName, UAnimMontage* MontageToPlay, float Rate,
	FName StartSection, bool bTriggerNotifiesBeforeStartTime, bool bEnableCustomTimeDilation, bool bStopWhenAbilityEnds,
	float AnimRootMotionTranslationScale, float StartTimeSeconds, bool bAllowInterruptAfterBlendOut)
{

	UAbilitySystemGlobals::NonShipping_ApplyGlobalAbilityScaler_Rate(Rate);

	UAbilityTask_PlayMontageProAndWait* MyObj = NewAbilityTask<UAbilityTask_PlayMontageProAndWait>(OwningAbility, TaskInstanceName);
	MyObj->MontageToPlay = MontageToPlay;
	MyObj->Rate = Rate;
	MyObj->StartSection = StartSection;
	MyObj->bTriggerNotifiesBeforeStartTime = bTriggerNotifiesBeforeStartTime;
	MyObj->bEnableCustomTimeDilation = bEnableCustomTimeDilation;
	MyObj->AnimRootMotionTranslationScale = AnimRootMotionTranslationScale;
	MyObj->bStopWhenAbilityEnds = bStopWhenAbilityEnds;
	MyObj->bAllowInterruptAfterBlendOut = bAllowInterruptAfterBlendOut;
	MyObj->StartTimeSeconds = StartTimeSeconds;
	
	return MyObj;
}

void UAbilityTask_PlayMontageProAndWait::Activate()
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

				// -- PlayMontagePro --
				
				// Use the mesh comp's OnTickPose to detect time dilation changes
				if (bEnableCustomTimeDilation && GetMesh() && ActorInfo->AvatarActor.IsValid())
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
				UPlayMontageProStatics::GatherNotifies(MontageToPlay, NotifyId, Notifies, Section, StartTimeSeconds, TimeDilation);

				// Trigger notifies before start time and remove them, if we want to trigger them before the start time
				UPlayMontageProStatics::HandleHistoricNotifies(Notifies, bTriggerNotifiesBeforeStartTime, this);

				// Create timer delegates for notifies
				UPlayMontageProStatics::SetupNotifyTimers(this, GetWorld(), Notifies);
			}
		}
		else
		{
			ABILITY_LOG(Warning, TEXT("UAbilityTask_PlayMontageProAndWait call to PlayMontagePro failed!"));
		}
	}
	else
	{
		ABILITY_LOG(Warning, TEXT("UAbilityTask_PlayMontageProAndWait called on invalid AbilitySystemComponent"));
	}

	if (!bPlayedMontage)
	{
		ABILITY_LOG(Warning, TEXT("UAbilityTask_PlayMontageProAndWait called in Ability %s failed to play montage %s; Task Instance Name %s."), *Ability->GetName(), *GetNameSafe(MontageToPlay),*InstanceName.ToString());
		UPlayMontageProStatics::EnsureBroadcastNotifyEvents(EAnimNotifyProEventType::OnCancelled, Notifies, this);
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnCancelled.Broadcast();
		}
	}

	SetWaitingOnAvatar();
}

void UAbilityTask_PlayMontageProAndWait::ExternalCancel()
{
	UPlayMontageProStatics::EnsureBroadcastNotifyEvents(EAnimNotifyProEventType::OnCancelled, Notifies, this);
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnCancelled.Broadcast();
	}
	Super::ExternalCancel();
}

UAnimMontage* UAbilityTask_PlayMontageProAndWait::GetMontage() const
{
	return IsValid(MontageToPlay) ? MontageToPlay : nullptr;
}

USkeletalMeshComponent* UAbilityTask_PlayMontageProAndWait::GetMesh() const
{
	const bool bValidMesh = Ability && Ability->GetCurrentActorInfo() && Ability->GetCurrentActorInfo()->SkeletalMeshComponent.IsValid();
	return bValidMesh ? Ability->GetCurrentActorInfo()->SkeletalMeshComponent.Get() : nullptr;
}

void UAbilityTask_PlayMontageProAndWait::OnMontageSectionChanged(UAnimMontage* InMontage, FName SectionName,
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
	UPlayMontageProStatics::GatherNotifies(InMontage, NotifyId, Notifies, SectionName, StartTime, TimeDilation);

	// Create timer delegates for notifies
	UPlayMontageProStatics::SetupNotifyTimers(this, GetWorld(), Notifies);
}

void UAbilityTask_PlayMontageProAndWait::OnTickPose(USkinnedMeshComponent* SkinnedMeshComponent, float DeltaTime,
	bool NeedsValidRootMotion)
{
	UPlayMontageProStatics::HandleTimeDilation(this, SkinnedMeshComponent, TimeDilation, Notifies);
}

void UAbilityTask_PlayMontageProAndWait::OnDestroy(bool AbilityEnded)
{
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
			StopPlayingMontage();
		}
	}

	Super::OnDestroy(AbilityEnded);

}

bool UAbilityTask_PlayMontageProAndWait::StopPlayingMontage()
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

FString UAbilityTask_PlayMontageProAndWait::GetDebugString() const
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

	return FString::Printf(TEXT("PlayMontageProAndWait. MontageToPlay: %s  (Currently Playing): %s"), *GetNameSafe(MontageToPlay), *GetNameSafe(PlayingMontage));
}

