// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "TimerManager.h"
#include "PlayMontageTypes.generated.h"

class UAnimNotifyStatePro;
class UAnimNotifyPro;
class UAnimMontage;

/**
 * Legacy behavior for anim notifies on simulated proxies.
 * This enum is used to determine how anim notifies should behave on simulated proxies.
 * If set to Legacy, the notify will be triggered on simulated proxies no different to the old system.
 * If set to Disabled, the notify will not be triggered on simulated proxies, only on authority and local clients.
 */
UENUM(BlueprintType)
enum class EAnimNotifyLegacyType : uint8
{
	Legacy			UMETA(ToolTip="Legacy behavior, notify will be triggered on simulated proxies no different to the old system"),
	Disabled		UMETA(ToolTip="Notify will not be triggered on simulated proxies, only on authority and local clients"),
};

/**
 * Bitmask for anim notify events, used to determine which events should trigger callbacks.
 * Used by UAnimNotifyPro and UAnimNotifyStatePro.
 */
UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor="true"))
enum class EAnimNotifyProEventType : uint8
{
	None			= 0	UMETA(Hidden),
	OnCompleted		= 1 << 0,
	BlendOut		= 1 << 1,
	OnInterrupted	= 1 << 2,
	OnCancelled		= 1 << 3,
};
ENUM_CLASS_FLAGS(EAnimNotifyProEventType)

/**
 * Type of anim notify event, used to determine which callback to use.
 * Used by FAnimNotifyProEvent.
 */
enum class EAnimNotifyProType : uint8
{
	Notify,
	NotifyStateBegin,
	NotifyStateEnd,
};

/**
 * Struct representing an anim notify event.
 * Contains information about the notify, such as its ID, time, and whether it has been broadcast.
 * Used by PlayMontagePro to handle anim notifies and notify states.
 */
USTRUCT()
struct PLAYMONTAGEPRO_API FAnimNotifyProEvent
{
	GENERATED_BODY()
	
	FAnimNotifyProEvent(uint32 InNotifyId = 0, int32 InEnsureTriggerNotify = 0, EAnimNotifyProType InNotifyType = EAnimNotifyProType::Notify, float InTime = 0.f)
		: EnsureTriggerNotify(InEnsureTriggerNotify)
		, bEnsureEndStateIfTriggered(true)
		, Time(InTime)
		, NotifyId(InNotifyId)
		, bHasBroadcast(false)
		, bIsEndState(false)
		, bNotifySkipped(false)
		, NotifyStatePair(nullptr)
		, NotifyType(InNotifyType)
	{}

	/** Bitmask for ensuring that notifies are triggered if the montage aborts before they're reached when aborted due to these conditions */
	UPROPERTY()
	int32 EnsureTriggerNotify;

	/** If true, the end state will be ensured if the notify was triggered */
	UPROPERTY()
	bool bEnsureEndStateIfTriggered;

	/** Time at which the notify should be triggered */
	UPROPERTY()
	float Time;

	/** Unique ID for the notify, used to identify it in the list of notifies */
	UPROPERTY()
	uint32 NotifyId;

	/** Whether the notify has been broadcasted */
	UPROPERTY()
	bool bHasBroadcast;

	/** Whether this notify is an end state notify, used for notify states */
	UPROPERTY()
	bool bIsEndState;

	/** Whether the notify was skipped due to start position, used to determine if the notify should be broadcasted */
	UPROPERTY()
	bool bNotifySkipped;

	/** Pointer to the paired notify state, used for notify states to link begin and end states */
	TObjectPtr<FAnimNotifyProEvent> NotifyStatePair;

	/** Type of the notify, used to determine which callback to use */
	EAnimNotifyProType NotifyType;

	/** Timer handle for the notify */
	FTimerHandle Timer;

	/** Delegate to call when the timer expires */
	FTimerDelegate TimerDelegate;

	/** Weak pointer to the notify object, used to call the notify callback */
	UPROPERTY()
	TWeakObjectPtr<UAnimNotifyPro> Notify;

	/** Weak pointer to the notify state object, used to call the notify state callbacks */
	UPROPERTY()
	TWeakObjectPtr<UAnimNotifyStatePro> NotifyState;

	/** Function to call when the timer expires, used to trigger the notify */
	void OnTimer();

	/** Clears the timer and delegate, used to clean up the notify when it is no longer needed */
	void ClearTimers()
	{
		if (Timer.IsValid())			{ Timer.Invalidate(); }
		if (TimerDelegate.IsBound())	{ TimerDelegate.Unbind(); }
	}

	bool IsValid() const { return NotifyId > 0 && (Notify.IsValid() || NotifyState.IsValid()); }

	bool operator==(const FAnimNotifyProEvent& Other) const
	{
		return NotifyId > 0 && NotifyId == Other.NotifyId;
	}

	bool operator!=(const FAnimNotifyProEvent& Other) const
	{
		return !(*this == Other);
	}
};

/**
 * Parameters for Pro notifies, which trigger reliably unlike Epic's notify system.
 * Contains options for enabling Pro notifies, triggering notifies before the starting position,
 * and enabling custom time dilation for the montage.
 */
USTRUCT(BlueprintType)
struct PLAYMONTAGEPRO_API FProNotifyParams
{
	GENERATED_BODY()

	/** Whether to enable Pro notifies, which trigger reliably unlike Epic's notify system */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Notify)
	bool bEnableProNotifies = true;

	/** Whether to trigger notifies before the starting position */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Notify, meta=(EditCondition="bEnableProNotifies", EditConditionHides))
	bool bTriggerNotifiesBeforeStartTime = false;

	/** Whether to enable custom time dilation for the montage. Requires the mesh component to tick pose. May have additional performance overhead */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Notify, meta=(EditCondition="bEnableProNotifies", EditConditionHides))
	bool bEnableCustomTimeDilation = false;
};
