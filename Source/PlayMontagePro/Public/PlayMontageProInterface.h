// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "PlayMontageTypes.h"
#include "UObject/Interface.h"
#include "PlayMontageProInterface.generated.h"

class UAnimMontage;

UINTERFACE()
class UPlayMontageProInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface for PlayMontagePro to allow broadcasting notify events and handling callbacks.
 * Shared functionality between different PlayMontage nodes.
 */
class PLAYMONTAGEPRO_API IPlayMontageProInterface
{
	GENERATED_BODY()

public:
	virtual void BroadcastNotifyEvent(FAnimNotifyProEvent& Event) = 0;

	virtual UAnimMontage* GetMontage() const = 0;
	virtual USkeletalMeshComponent* GetMesh() const = 0;

	virtual FTimerDelegate CreateTimerDelegate(FAnimNotifyProEvent& Event) = 0;
	void OnNotifyTimer(FAnimNotifyProEvent* Event)
	{
		BroadcastNotifyEvent(*Event);
	}
};
