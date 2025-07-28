// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "PlayMontageProInterface.h"
#include "PlayMontageProStatics.h"
#include "PlayMontageTypes.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "Animation/AnimInstance.h"
#include "PlayMontageProCallbackProxy.generated.h"

class UAnimNotifyStatePro;
class UAnimNotifyPro;
class UAnimMontage;
class USkeletalMeshComponent;
struct FBranchingPointNotifyPayload;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMontageProPlayDelegate, FName, NotifyName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMontageProPlayNotifyDelegate, const FAnimNotifyProEvent&, Event);

UCLASS()
class PLAYMONTAGEPRO_API UPlayMontageProCallbackProxy : public UObject, public IPlayMontageProInterface
{
	GENERATED_UCLASS_BODY()

	// Called when Montage finished playing and wasn't interrupted
	UPROPERTY(BlueprintAssignable)
	FOnMontageProPlayDelegate OnCompleted;

	// Called when Montage starts blending out and is not interrupted
	UPROPERTY(BlueprintAssignable)
	FOnMontageProPlayDelegate OnBlendOut;

	// Called when Montage has been interrupted (or failed to play)
	UPROPERTY(BlueprintAssignable)
	FOnMontageProPlayDelegate OnInterrupted;

	UPROPERTY(BlueprintAssignable)
	FOnMontageProPlayNotifyDelegate OnNotify;
	
	UPROPERTY(BlueprintAssignable)
	FOnMontageProPlayNotifyDelegate OnNotifyStateBegin;

	UPROPERTY(BlueprintAssignable)
	FOnMontageProPlayNotifyDelegate OnNotifyStateEnd;

	UPROPERTY()
	uint32 NotifyId = 0;

	UPROPERTY()
	TWeakObjectPtr<UAnimMontage> Montage;

	UPROPERTY()
	TWeakObjectPtr<USkeletalMeshComponent> MeshComp;
	
	UPROPERTY()
	TArray<FAnimNotifyProEvent> Notifies;
	
	// Called to perform the query internally
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"))
	static UPlayMontageProCallbackProxy* CreateProxyObjectForPlayMontagePro(
		USkeletalMeshComponent* InSkeletalMeshComponent, 
		UAnimMontage* MontageToPlay, 
		float PlayRate = 1.f, 
		float StartingPosition = 0.f, 
		FName StartingSection = NAME_None,
		bool bTriggerNotifiesBeforeStartTime = false,
		bool bEnableCustomTimeDilation = false,
		bool bShouldStopAllMontages = true);

public:
	// Begin IPlayMontageProInterface
	virtual void BroadcastNotifyEvent(FAnimNotifyProEvent& Event) override { UPlayMontageProStatics::BroadcastNotifyEvent(Event, this); }
	virtual void NotifyCallback(const FAnimNotifyProEvent& Event) override { OnNotify.Broadcast(Event); }
	virtual void NotifyBeginCallback(const FAnimNotifyProEvent& Event) override { OnNotifyStateBegin.Broadcast(Event); }
	virtual void NotifyEndCallback(const FAnimNotifyProEvent& Event) override { OnNotifyStateEnd.Broadcast(Event); }

	virtual UAnimMontage* GetMontage() const override final { return Montage.IsValid() ? Montage.Get() : nullptr; }
	virtual USkeletalMeshComponent* GetMesh() const override final { return MeshComp.IsValid() ? MeshComp.Get() : nullptr; }

	virtual FTimerDelegate CreateTimerDelegate(FAnimNotifyProEvent& Event) override { return FTimerDelegate::CreateUObject(this, &IPlayMontageProInterface::OnNotifyTimer, &Event); }
	// ~End IPlayMontageProInterface
	
protected:
	UFUNCTION()
	void OnMontageBlendingOut(UAnimMontage* InMontage, bool bInterrupted);

	UFUNCTION()
	void OnMontageEnded(UAnimMontage* InMontage, bool bInterrupted);

	UFUNCTION()
	void OnMontageSectionChanged(UAnimMontage* InMontage, FName SectionName, bool bLooped);

	bool bFinished = false;
	
	FDelegateHandle TickPoseHandle;

	float TimeDilation = 1.f;
	
	UFUNCTION()
	void OnTickPose(USkinnedMeshComponent* SkinnedMeshComponent, float DeltaTime, bool NeedsValidRootMotion);

	virtual void BeginDestroy() override;
	
private:
	TWeakObjectPtr<UAnimInstance> AnimInstancePtr;
	int32 MontageInstanceID;
	uint32 bInterruptedCalledBeforeBlendingOut : 1;

	bool IsNotifyValid(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointNotifyPayload) const;

	FOnMontageBlendingOutStarted BlendingOutDelegate;
	FOnMontageEnded MontageEndedDelegate;

protected:
	/** Plays a montage on the specified skeletal mesh component.
	 * @param InSkeletalMeshComponent The skeletal mesh component to play the montage on.
	 * @param MontageToPlay The montage to play.
	 * @param PlayRate The rate at which to play the montage.
	 * @param StartingPosition The position in the montage to start playing from.
	 * @param StartingSection The section of the montage to start playing from.
	 * @param bTriggerNotifiesBeforeStartTime Whether to trigger notifies before the starting position.
	 * @param bEnableCustomTimeDilation Whether to enable custom time dilation for the montage. Requires the mesh component to tick pose. May have additional performance overhead.
	 * @param bShouldStopAllMontages Whether to stop all other montages before playing this one.
	 * @return True if the montage was played successfully, false otherwise.
	 */
	bool PlayMontagePro(
		USkeletalMeshComponent* InSkeletalMeshComponent,
		UAnimMontage* MontageToPlay,
		float PlayRate = 1.f,
		float StartingPosition = 0.f,
		FName StartingSection = NAME_None,
		bool bTriggerNotifiesBeforeStartTime = false,
		bool bEnableCustomTimeDilation = false,
		bool bShouldStopAllMontages = true);
};
