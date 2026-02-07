// Copyright (c) Jared Taylor

#include "PlayMontageTypes.h"

#include "AnimNotifyPro.h"
#include "AnimNotifyStatePro.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PlayMontageTypes)

void FAnimNotifyProEvent::ClearTimers()
{
	// Invalidate timer if valid
	if (Timer.IsValid())
	{
		Timer.Invalidate();
	}

	// We will crash if we attempt to unbind a delegate connected to a null object
	if (!TaskOwner.IsValid())
	{
		return;
	}
	
	// Unbind the delegate if bound
	if (TimerDelegate.IsBound())
	{
		TimerDelegate.Unbind();
	}
}

bool FAnimNotifyProEvent::IsValidEvent() const
{
	return NotifyId > 0 && TaskOwner.IsValid() && (Notify != nullptr || NotifyState != nullptr);
}
