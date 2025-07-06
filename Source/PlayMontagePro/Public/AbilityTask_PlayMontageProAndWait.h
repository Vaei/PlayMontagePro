// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "PlayMontageProInterface.h"
#include "PlayMontageProStatics.h"
#include "PlayMontageTypes.h"
#include "UObject/ObjectMacros.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Animation/AnimInstance.h"
#include "AbilityTask_PlayMontageProAndWait.generated.h"

class UAnimMontage;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMontageWaitSimpleDelegate);

/** Ability task to simply play a montage. Many games will want to make a modified version of this task that looks for game-specific events */
UCLASS()
class PLAYMONTAGEPRO_API UAbilityTask_PlayMontageProAndWait : public UAbilityTask, public IPlayMontageProInterface
{
	GENERATED_BODY()
public:

	UPROPERTY(BlueprintAssignable)
	FMontageWaitSimpleDelegate	OnCompleted;

	UPROPERTY(BlueprintAssignable)
	FMontageWaitSimpleDelegate	OnBlendedIn;

	UPROPERTY(BlueprintAssignable)
	FMontageWaitSimpleDelegate	OnBlendOut;

	UPROPERTY(BlueprintAssignable)
	FMontageWaitSimpleDelegate	OnInterrupted;

	UPROPERTY(BlueprintAssignable)
	FMontageWaitSimpleDelegate	OnCancelled;

	UFUNCTION()
	void OnMontageBlendedIn(UAnimMontage* Montage);

	UFUNCTION()
	void OnMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted);

	/** Callback function for when the owning Gameplay Ability is cancelled */
	UFUNCTION()
	void OnGameplayAbilityCancelled();

	UFUNCTION()
	void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	/** 
	 * Start playing an animation montage on the avatar actor and wait for it to finish
	 * If StopWhenAbilityEnds is true, this montage will be aborted if the ability ends normally. It is always stopped when the ability is explicitly cancelled.
	 * On normal execution, OnBlendOut is called when the montage is blending out, and OnCompleted when it is completely done playing
	 * OnInterrupted is called if another montage overwrites this, and OnCancelled is called if the ability or task is cancelled
	 *
	 * @param OwningAbility
	 * @param TaskInstanceName Set to override the name of this task, for later querying
	 * @param MontageToPlay The montage to play on the character
	 * @param Rate Change to play the montage faster or slower
	 * @param StartSection If not empty, named montage section to start from
	 * @param bTriggerNotifiesBeforeStartTime Whether to trigger notifies before the starting position.
	 * @param bEnableCustomTimeDilation Whether to enable custom time dilation for the montage. Requires the mesh component to tick pose. May have additional performance overhead.
	 * @param bStopWhenAbilityEnds If true, this montage will be aborted if the ability ends normally. It is always stopped when the ability is explicitly cancelled
	 * @param AnimRootMotionTranslationScale Change to modify size of root motion or set to 0 to block it entirely
	 * @param StartTimeSeconds Starting time offset in montage, this will be overridden by StartSection if that is also set
	 * @param bAllowInterruptAfterBlendOut If true, you can receive OnInterrupted after an OnBlendOut started (otherwise OnInterrupted will not fire when interrupted, but you will not get OnComplete).
	 */
	UFUNCTION(BlueprintCallable, Category="Ability|Tasks", meta = (DisplayName="PlayMontageProAndWait",
		HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
	static UAbilityTask_PlayMontageProAndWait* CreatePlayMontageProAndWaitProxy(UGameplayAbility* OwningAbility,
		FName TaskInstanceName, UAnimMontage* MontageToPlay, float Rate = 1.f, FName StartSection = NAME_None,
		bool bTriggerNotifiesBeforeStartTime = false, bool bEnableCustomTimeDilation = false,
		bool bStopWhenAbilityEnds = true, float AnimRootMotionTranslationScale = 1.f, float StartTimeSeconds = 0.f,
		bool bAllowInterruptAfterBlendOut = false);

	virtual void Activate() override;

	/** Called when the ability is asked to cancel from an outside node. What this means depends on the individual task. By default, this does nothing other than ending the task. */
	virtual void ExternalCancel() override;

	virtual FString GetDebugString() const override;
	
public:
	// Begin IPlayMontageProInterface
	virtual void BroadcastNotifyEvent(FAnimNotifyProEvent& Event) override { UPlayMontageProStatics::BroadcastNotifyEvent(Event, this); }
	virtual void NotifyCallback(const FAnimNotifyProEvent& Event) override {}
	virtual void NotifyBeginCallback(const FAnimNotifyProEvent& Event) override {}
	virtual void NotifyEndCallback(const FAnimNotifyProEvent& Event) override {}

	virtual UAnimMontage* GetMontage() const override final;
	virtual USkeletalMeshComponent* GetMesh() const override final;

	virtual FTimerDelegate CreateTimerDelegate(FAnimNotifyProEvent& Event) override { return FTimerDelegate::CreateUObject(this, &IPlayMontageProInterface::OnNotifyTimer, &Event); }
	// ~End IPlayMontageProInterface
	
protected:
	UFUNCTION()
	void OnMontageSectionChanged(UAnimMontage* InMontage, FName SectionName, bool bLooped);
	
	UFUNCTION()
	void OnTickPose(USkinnedMeshComponent* SkinnedMeshComponent, float DeltaTime, bool NeedsValidRootMotion);

	virtual void OnDestroy(bool AbilityEnded) override;

	/** Checks if the ability is playing a montage and stops that montage, returns true if a montage was stopped, false if not. */
	bool StopPlayingMontage();

	FOnMontageBlendedInEnded BlendedInDelegate;
	FOnMontageBlendingOutStarted BlendingOutDelegate;
	FOnMontageEnded MontageEndedDelegate;
	FDelegateHandle InterruptedHandle;

	UPROPERTY()
	TObjectPtr<UAnimMontage> MontageToPlay;

	UPROPERTY()
	float Rate;

	UPROPERTY()
	FName StartSection;

	UPROPERTY()
	bool bTriggerNotifiesBeforeStartTime;

	UPROPERTY()
	bool bEnableCustomTimeDilation;

	UPROPERTY()
	float AnimRootMotionTranslationScale;

	UPROPERTY()
	float StartTimeSeconds;

	UPROPERTY()
	bool bStopWhenAbilityEnds;

	UPROPERTY()
	bool bAllowInterruptAfterBlendOut;

	UPROPERTY()
	uint32 NotifyId = 0;
	
	UPROPERTY()
	TArray<FAnimNotifyProEvent> Notifies;
	
	FDelegateHandle TickPoseHandle;
	
	float TimeDilation = 1.f;
};
