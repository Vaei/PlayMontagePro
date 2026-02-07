// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "PlayMontageTypes.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyStatePro.generated.h"

/**
 * Base class for anim notify states that can be used with PlayMontagePro.
 * Uses timers to ensure that notify states are triggered at the correct time.
 * Unlike Epic's UAnimNotifyState, which is not guaranteed to trigger.
 */
UCLASS(Abstract, EditInlineNew, Blueprintable, const)
class PLAYMONTAGEPRO_API UAnimNotifyStatePro : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	/** Ensure that notifies are triggered if the montage aborts before they're reached when aborted due to these conditions */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=AnimNotify, meta=(BitMask, BitmaskEnum="/Script/PlayMontagePro.EAnimNotifyProEventType"))
	int32 EnsureTriggerNotify = 0;

	/** If disabled this notify will be skipped on dedicated servers */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=AnimNotify)
	bool bTriggerOnDedicatedServer = true;

	/** SimulatedProxies do not typically execute PlayMontage nodes and need to route through the legacy notify system */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=AnimNotify)
	EAnimNotifyLegacyType SimulatedProxyBehavior = EAnimNotifyLegacyType::Legacy;

#if WITH_EDITORONLY_DATA

protected:
	bool bHasBlueprintNotifyBegin;
	bool bHasBlueprintNotifyTick;
	bool bHasBlueprintNotifyEnd;

#endif
	
public:
	UAnimNotifyStatePro(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

	bool WantsSimulatedProxyNotify(const USkeletalMeshComponent* MeshComp) const;
	
	virtual void NotifyBegin(USkeletalMeshComponent * MeshComp, UAnimSequenceBase * Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override final;
	virtual void NotifyTick(USkeletalMeshComponent * MeshComp, UAnimSequenceBase * Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override final {}
	virtual void NotifyEnd(USkeletalMeshComponent * MeshComp, UAnimSequenceBase * Animation, const FAnimNotifyEventReference& EventReference) override final;
	
	virtual void BranchingPointNotifyBegin(FBranchingPointNotifyPayload& BranchingPointPayload) override final {}
	virtual void BranchingPointNotifyTick(FBranchingPointNotifyPayload& BranchingPointPayload, float FrameDeltaTime) override final {}
	virtual void BranchingPointNotifyEnd(FBranchingPointNotifyPayload& BranchingPointPayload) override final {}

public:
	virtual bool ShouldTriggerNotify(USkeletalMeshComponent* MeshComp) const;
	
	virtual void NotifyBeginCallback(USkeletalMeshComponent* MeshComp, UAnimMontage* Montage, float TotalDuration);
	virtual void NotifyEndCallback(USkeletalMeshComponent* MeshComp, UAnimMontage* Montage);
	
	virtual void OnNotifyBegin(USkeletalMeshComponent* MeshComp, UAnimMontage* Montage, float TotalDuration);
	virtual void OnNotifyEnd(USkeletalMeshComponent* MeshComp, UAnimMontage* Montage);
	
	UFUNCTION(BlueprintImplementableEvent, meta=(DisplayName="On Notify Begin"))
	bool K2_OnNotifyBegin(USkeletalMeshComponent* MeshComp, UAnimMontage* Montage, float TotalDuration) const;

	UFUNCTION(BlueprintImplementableEvent, meta=(DisplayName="On Notify End"))
	bool K2_OnNotifyEnd(USkeletalMeshComponent* MeshComp, UAnimMontage* Montage) const;

#if WITH_EDITOR
	virtual bool CanBePlaced(UAnimSequenceBase* Animation) const override
	{
		return Animation && Animation->IsA(UAnimMontage::StaticClass());
	}
#endif
};
