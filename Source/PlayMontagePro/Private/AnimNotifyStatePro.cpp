// Copyright (c) Jared Taylor


#include "AnimNotifyStatePro.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimMontage.h"

#if WITH_EDITOR
#include "Animation/DebugSkelMeshComponent.h"
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimNotifyStatePro)

UAnimNotifyStatePro::UAnimNotifyStatePro(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITORONLY_DATA
	// Pale yellow color
	NotifyColor = FColor(255, 255, 200);
	
	auto ImplementedInBlueprint = [](const UFunction* Func) -> bool
	{
		return Func && ensure(Func->GetOuter())
			&& Func->GetOuter()->IsA(UBlueprintGeneratedClass::StaticClass());
	};

	{
		static FName FuncName = FName(TEXT("Received_NotifyBegin"));
		const UFunction* BPFunc = GetClass()->FindFunctionByName(FuncName);
		bHasBlueprintNotifyBegin = ImplementedInBlueprint(BPFunc);
	}

	{
		static FName FuncName = FName(TEXT("Received_NotifyTick"));
		const UFunction* BPFunc = GetClass()->FindFunctionByName(FuncName);
		bHasBlueprintNotifyTick = ImplementedInBlueprint(BPFunc);
	}

	{
		static FName FuncName = FName(TEXT("Received_NotifyEnd"));
		const UFunction* BPFunc = GetClass()->FindFunctionByName(FuncName);
		bHasBlueprintNotifyEnd = ImplementedInBlueprint(BPFunc);
	}
#endif
}

#if WITH_EDITOR

EDataValidationResult UAnimNotifyStatePro::IsDataValid(class FDataValidationContext& Context) const
{
#if WITH_EDITORONLY_DATA
	if (bHasBlueprintNotifyBegin)
	{
		Context.AddError(
			FText::Format(
				NSLOCTEXT("AnimNotifyStatePro", "BlueprintReceivedNotifyBeginWarning",
					"AnimNotifyPro {0} has a Blueprint implementation of Received_NotifyBegin, which is not supported. "
					"Please use K2_OnNotifyBegin instead."),
				FText::FromString(GetName())
			)
		);
		return EDataValidationResult::Invalid;
	}
	if (bHasBlueprintNotifyTick)
	{
		Context.AddError(
			FText::Format(
				NSLOCTEXT("AnimNotifyStatePro", "BlueprintReceivedNotifyTickWarning",
					"AnimNotifyPro {0} has a Blueprint implementation of Received_NotifyTick, which is not supported. "
					"Ticking is not supported with Pro."),
				FText::FromString(GetName())
			)
		);
		return EDataValidationResult::Invalid;
	}
	if (bHasBlueprintNotifyEnd)
	{
		Context.AddError(
			FText::Format(
				NSLOCTEXT("AnimNotifyStatePro", "BlueprintReceivedNotifyEndWarning",
					"AnimNotifyPro {0} has a Blueprint implementation of Received_NotifyEnd, which is not supported. "
					"Please use K2_OnNotifyEnd instead."),
				FText::FromString(GetName())
			)
		);
		return EDataValidationResult::Invalid;
	}
#endif
	return Super::IsDataValid(Context);
}

#endif

bool UAnimNotifyStatePro::WantsSimulatedProxyNotify(const USkeletalMeshComponent* MeshComp) const
{
	if (SimulatedProxyBehavior == EAnimNotifyLegacyType::Legacy)
	{
		const AActor* Owner = MeshComp->GetOwner();
		if (Owner->GetNetMode() != NM_Standalone && Owner->GetLocalRole() == ROLE_SimulatedProxy)
		{
			return true;
		}
	}

	return false;
}

void UAnimNotifyStatePro::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
#if WITH_EDITOR
	// Editor support -- for previewing in the editor
	if (MeshComp->IsA<UDebugSkelMeshComponent>() && ShouldFireInEditor())
	{
		UAnimMontage* Montage = Animation ? Cast<UAnimMontage>(Animation) : nullptr;
		OnNotifyBegin(MeshComp, Montage);
	}
#endif

	if (WantsSimulatedProxyNotify(MeshComp))
	{
		// Legacy behavior, notify will be triggered on simulated proxies no different to the old system
		UAnimMontage* Montage = Animation ? Cast<UAnimMontage>(Animation) : nullptr;
		OnNotifyBegin(MeshComp, Montage);
	}
}

void UAnimNotifyStatePro::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
#if WITH_EDITOR
	// Editor support -- for previewing in the editor
	if (MeshComp->IsA<UDebugSkelMeshComponent>() && ShouldFireInEditor())
	{
		UAnimMontage* Montage = Animation ? Cast<UAnimMontage>(Animation) : nullptr;
		OnNotifyEnd(MeshComp, Montage);
	}
#endif

	if (WantsSimulatedProxyNotify(MeshComp))
	{
		// Legacy behavior, notify will be triggered on simulated proxies no different to the old system
		UAnimMontage* Montage = Animation ? Cast<UAnimMontage>(Animation) : nullptr;
		OnNotifyEnd(MeshComp, Montage);
	}
}

bool UAnimNotifyStatePro::ShouldTriggerNotify(USkeletalMeshComponent* MeshComp) const
{
	return !MeshComp || MeshComp->GetNetMode() != NM_DedicatedServer || bTriggerOnDedicatedServer;
}

void UAnimNotifyStatePro::NotifyBeginCallback(USkeletalMeshComponent* MeshComp, UAnimMontage* Montage)
{
	if (ShouldTriggerNotify(MeshComp))
	{
		OnNotifyBegin(MeshComp, Montage);
	}
}

void UAnimNotifyStatePro::NotifyEndCallback(USkeletalMeshComponent* MeshComp, UAnimMontage* Montage)
{
	if (ShouldTriggerNotify(MeshComp))
	{
		OnNotifyEnd(MeshComp, Montage);
	}
}

void UAnimNotifyStatePro::OnNotifyBegin(USkeletalMeshComponent* MeshComp, UAnimMontage* Montage)
{
	K2_OnNotifyBegin(MeshComp, Montage);
}

void UAnimNotifyStatePro::OnNotifyEnd(USkeletalMeshComponent* MeshComp, UAnimMontage* Montage)
{
	K2_OnNotifyEnd(MeshComp, Montage);
}
