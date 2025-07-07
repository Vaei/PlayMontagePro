// Copyright (c) Jared Taylor


#include "AnimNotifyPro.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimMontage.h"

#if WITH_EDITOR
#include "Animation/DebugSkelMeshComponent.h"
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimNotifyPro)

UAnimNotifyPro::UAnimNotifyPro(const FObjectInitializer& ObjectInitializer)
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
		static FName FuncName = FName(TEXT("Received_Notify"));
		const UFunction* BPFunc = GetClass()->FindFunctionByName(FuncName);
		bHasBlueprintReceivedNotify = ImplementedInBlueprint(BPFunc);
	}
#endif
}

#if WITH_EDITOR

EDataValidationResult UAnimNotifyPro::IsDataValid(class FDataValidationContext& Context) const
{
#if WITH_EDITORONLY_DATA
	if (bHasBlueprintReceivedNotify)
	{
		Context.AddError(
			FText::Format(
				NSLOCTEXT("AnimNotifyPro", "BlueprintReceivedNotifyWarning",
					"AnimNotifyPro {0} has a Blueprint implementation of Received_Notify, which is not supported. "
					"Please use K2_OnNotify instead."),
				FText::FromString(GetName())
			)
		);
		return EDataValidationResult::Invalid;
	}
#endif
	return Super::IsDataValid(Context);
}

#endif

void UAnimNotifyPro::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
#if WITH_EDITOR
	// Editor support -- for previewing in the editor
	if (MeshComp->IsA<UDebugSkelMeshComponent>() && ShouldFireInEditor())
	{
		UAnimMontage* Montage = Animation ? Cast<UAnimMontage>(Animation) : nullptr;
		OnNotify(MeshComp, Montage);
	}
#endif

	if (SimulatedProxyBehavior == EAnimNotifyLegacyType::Legacy)
	{
		const AActor* Owner = MeshComp->GetOwner();
		if (Owner->GetNetMode() != NM_Standalone && Owner->GetLocalRole() == ROLE_SimulatedProxy)
		{
			// Legacy behavior, notify will be triggered on simulated proxies no different to the old system
			UAnimMontage* Montage = Animation ? Cast<UAnimMontage>(Animation) : nullptr;
			OnNotify(MeshComp, Montage);
		}
	}
}

bool UAnimNotifyPro::ShouldTriggerNotify(USkeletalMeshComponent* MeshComp) const
{
	return !MeshComp || MeshComp->GetNetMode() != NM_DedicatedServer || bTriggerOnDedicatedServer;
}

void UAnimNotifyPro::NotifyCallback(USkeletalMeshComponent* MeshComp, UAnimMontage* Montage)
{
	if (!MeshComp || MeshComp->GetNetMode() != NM_DedicatedServer || bTriggerOnDedicatedServer)
	{
		OnNotify(MeshComp, Montage);
	}
}

void UAnimNotifyPro::OnNotify(USkeletalMeshComponent* MeshComp, UAnimMontage* Montage)
{
	K2_OnNotify(MeshComp, Montage);
}
