// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "PlayMontageTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PlayMontageProStatics.generated.h"

class UAnimMontage;
class IPlayMontageProInterface;

/**
 * Common utility functions for PlayMontagePro shared between different PlayMontage nodes.
 */
UCLASS()
class PLAYMONTAGEPRO_API UPlayMontageProStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Calculates the play rate required to scale a montage to a specific duration.
	 * @param Montage The montage to calculate the play rate for.
	 * @param Duration The target duration to scale the montage to.
	 * @return The calculated play rate.
	 */
	UFUNCTION(BlueprintPure, Category=Animation)
	static float GetMontagePlayRateScaledByDuration(const UAnimMontage* Montage, float Duration);
	
public:
	/**
	 * Gathers notifies from the montage and returns them in the Notifies array.
	 * @param TaskOwner The ability task or outer owning this operation.
	 * @param Montage The montage to gather notifies from.
	 * @param NotifyId The current notify ID, which will be incremented for each notify found.
	 * @param Notifies The array to store the gathered notifies.
	 * @param NotifyStatePairs A map to store pairs of notify state begin and end events.
	 * @param Section The section of the montage to gather notifies from.
	 * @param StartPosition The starting position of the montage, used to calculate notify times.
	 * @param TimeDilation The time dilation factor to apply to the notify times.
	 */
	static void GatherNotifies(const UObject* TaskOwner, UAnimMontage* Montage, uint32& NotifyId, TArray<FAnimNotifyProEvent>& Notifies, 
		TMap<FAnimNotifyProEvent, FAnimNotifyProEvent>& NotifyStatePairs, const FName& Section, float StartPosition, float TimeDilation);

	/**
	 * Handles historic notifies, triggering them before the start time if specified, or marking them as skipped.
	 * @param Notifies The array of notifies to handle.
	 * @param NotifyStatePairs A map of notify state pairs to assist in handling end states.
	 * @param bTriggerNotifiesBeforeStartTime Whether to trigger notifies before the start time.
	 * @param StartTime The start time of the montage, used to determine which notifies are historic.
	 * @param Interface The interface to use for broadcasting notify events.
	 */
	static void HandleHistoricNotifies(TArray<FAnimNotifyProEvent>& Notifies, TMap<FAnimNotifyProEvent, FAnimNotifyProEvent>& NotifyStatePairs, 
		bool bTriggerNotifiesBeforeStartTime, float StartTime, IPlayMontageProInterface* Interface);

	/**
	 * Sets up timers for the notifies in the Notifies array, using the provided interface to create timer delegates.
	 * @param Interface The interface to use for creating timer delegates.
	 * @param World The world context to use for setting up timers.
	 * @param Notifies The array of notifies to set up timers for.
	 */
	static void SetupNotifyTimers(IPlayMontageProInterface* Interface, const UWorld* World, TArray<FAnimNotifyProEvent>& Notifies);

	/**
	 * Clears the timers for the notifies in the Notifies array.
	 * @param World The world context to use for clearing timers.
	 * @param Notifies The array of notifies to clear timers for.
	 */
	static void ClearNotifyTimers(const UWorld* World, TArray<FAnimNotifyProEvent>& Notifies);

	/**
	 * Broadcasts a notify event using the provided interface.
	 * @param Event The notify event to broadcast.
	 * @param NotifyStatePair The paired notify event, e.g. end or start state of a notify state.
	 * @param Interface The interface to use for broadcasting the event.
	 */
	static void BroadcastNotifyEvent(FAnimNotifyProEvent& Event, FAnimNotifyProEvent* NotifyStatePair, IPlayMontageProInterface* Interface);

	/**
	 * Ensures that broadcast notify events are triggered for the specified event type.
	 * @param EventType The type of event to ensure is broadcasted.
	 * @param Notifies The array of notifies to check and broadcast.
	 * @param NotifyStatePairs A map of notify state pairs to assist in broadcasting end states.
	 * @param Interface The interface to use for broadcasting the events.
	 */
	static void EnsureBroadcastNotifyEvents(EAnimNotifyProEventType EventType, TArray<FAnimNotifyProEvent>& Notifies, 
		TMap<FAnimNotifyProEvent, FAnimNotifyProEvent>& NotifyStatePairs, IPlayMontageProInterface* Interface);

	/**
	 * Handles time dilation for the montage, adjusting the TimeDilation factor and triggering notifies as needed.
	 * Requires that the mesh component is ticking pose.
	 * @param Interface The interface to use for broadcasting notify events.
	 * @param MeshComp The skinned mesh component associated with the montage.
	 * @param TimeDilation The current time dilation factor to adjust.
	 * @param Notifies The array of notifies to handle.
	 */
	static void HandleTimeDilation(IPlayMontageProInterface* Interface, const USkinnedMeshComponent* MeshComp, float& TimeDilation, TArray<FAnimNotifyProEvent>& Notifies);
};
