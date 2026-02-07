// Copyright (c) Jared Taylor

#include "K2Node_PlayMontagePro.h"

#include "Containers/UnrealString.h"
#include "EdGraph/EdGraphPin.h"
#include "HAL/Platform.h"
#include "Internationalization/Internationalization.h"
#include "Misc/AssertionMacros.h"
#include "PlayMontageProCallbackProxy.h"
#include "UObject/NameTypes.h"
#include "UObject/ObjectPtr.h"

#define LOCTEXT_NAMESPACE "K2Node"

UK2Node_PlayMontagePro::UK2Node_PlayMontagePro(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ProxyFactoryFunctionName = GET_FUNCTION_NAME_CHECKED(UPlayMontageProCallbackProxy, CreateProxyObjectForPlayMontagePro);
	ProxyFactoryClass = UPlayMontageProCallbackProxy::StaticClass();
	ProxyClass = UPlayMontageProCallbackProxy::StaticClass();
}

FText UK2Node_PlayMontagePro::GetTooltipText() const
{
	return LOCTEXT("K2Node_PlayMontagePro_Tooltip", "Plays a Montage on a SkeletalMeshComponent with custom notify support using UAnimNotifyPro and UAnimNotifyStatePro.");
}

FText UK2Node_PlayMontagePro::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("PlayMontagePro", "Play Montage Pro");
}

FText UK2Node_PlayMontagePro::GetMenuCategory() const
{
	return LOCTEXT("PlayMontageProCategory", "Animation|Montage");
}

void UK2Node_PlayMontagePro::GetPinHoverText(const UEdGraphPin& Pin, FString& HoverTextOut) const
{
	Super::GetPinHoverText(Pin, HoverTextOut);

	static const FName NAME_InSkeletalMeshComponent = FName(TEXT("InSkeletalMeshComponent"));
	static const FName NAME_MontageToPlay = FName(TEXT("MontageToPlay"));
	static const FName NAME_PlayRate = FName(TEXT("PlayRate"));
	static const FName NAME_StartingPosition = FName(TEXT("StartingPosition"));
	static const FName NAME_StartingSection = FName(TEXT("StartingSection"));
	static const FName NAME_TriggerNotifiesBeforeStartTime = FName(TEXT("bTriggerNotifiesBeforeStartTime"));
	static const FName NAME_EnableCustomTimeDilation = FName(TEXT("bEnableCustomTimeDilation"));
	static const FName NAME_ShouldStopAllMontages = FName(TEXT("bShouldStopAllMontages"));
	static const FName NAME_OnNotify = FName(TEXT("OnNotify"));
	static const FName NAME_OnNotifyBegin = FName(TEXT("OnNotifyStateBegin"));
	static const FName NAME_OnNotifyEnd = FName(TEXT("OnNotifyStateEnd"));
	
	if (Pin.PinName == NAME_InSkeletalMeshComponent)
	{
		const FText ToolTipText = LOCTEXT("K2Node_PlayMontagePro_InSkeletalMeshComponent_Tooltip", "The SkeletalMeshComponent to play the montage on.");
		HoverTextOut = FString::Printf(TEXT("%s\n%s"), *ToolTipText.ToString(), *HoverTextOut);
	}
	else if (Pin.PinName == NAME_MontageToPlay)
	{
		const FText ToolTipText = LOCTEXT("K2Node_PlayMontagePro_MontageToPlay_Tooltip", "The montage to play.");
		HoverTextOut = FString::Printf(TEXT("%s\n%s"), *ToolTipText.ToString(), *HoverTextOut);
	}
	else if (Pin.PinName == NAME_PlayRate)
	{
		const FText ToolTipText = LOCTEXT("K2Node_PlayMontagePro_PlayRate_Tooltip", "The rate at which to play the montage.");
		HoverTextOut = FString::Printf(TEXT("%s\n%s"), *ToolTipText.ToString(), *HoverTextOut);
	}
	else if (Pin.PinName == NAME_StartingPosition)
	{
		const FText ToolTipText = LOCTEXT("K2Node_PlayMontagePro_StartingPosition_Tooltip", "The position in the montage to start playing from.");
		HoverTextOut = FString::Printf(TEXT("%s\n%s"), *ToolTipText.ToString(), *HoverTextOut);
	}
	else if (Pin.PinName == NAME_StartingSection)
	{
		const FText ToolTipText = LOCTEXT("K2Node_PlayMontagePro_StartingSection_Tooltip", "The section of the montage to start playing from.");
		HoverTextOut = FString::Printf(TEXT("%s\n%s"), *ToolTipText.ToString(), *HoverTextOut);
	}
	else if (Pin.PinName == NAME_TriggerNotifiesBeforeStartTime)
	{
		const FText ToolTipText = LOCTEXT("K2Node_PlayMontagePro_TriggerNotifiesBeforeStartTime_Tooltip", "Whether to trigger notifies before the starting position.");
		HoverTextOut = FString::Printf(TEXT("%s\n%s"), *ToolTipText.ToString(), *HoverTextOut);
	}
	if (Pin.PinName == NAME_EnableCustomTimeDilation)
	{
		const FText ToolTipText = LOCTEXT("K2Node_PlayMontagePro_EnableCustomTimeDilation_Tooltip", "Whether to enable custom time dilation for the montage. Requires the mesh component to tick pose. May have additional performance overhead.");
		HoverTextOut = FString::Printf(TEXT("%s\n%s"), *ToolTipText.ToString(), *HoverTextOut);
	}
	else if (Pin.PinName == NAME_ShouldStopAllMontages)
	{
		const FText ToolTipText = LOCTEXT("K2Node_PlayMontagePro_ShouldStopAllMontages_Tooltip", "Whether to stop all other montages before playing this one.");
		HoverTextOut = FString::Printf(TEXT("%s\n%s"), *ToolTipText.ToString(), *HoverTextOut);
	}
	else if (Pin.PinName == NAME_OnNotify)
	{
		const FText ToolTipText = LOCTEXT("K2Node_PlayMontagePro_OnNotify_Tooltip", "Event called when using a UAnimNotifyPro Notify in a Montage.");
		HoverTextOut = FString::Printf(TEXT("%s\n%s"), *ToolTipText.ToString(), *HoverTextOut);
	}
	else if (Pin.PinName == NAME_OnNotifyBegin)
	{
		const FText ToolTipText = LOCTEXT("K2Node_PlayMontagePro_OnNotifyBegin_Tooltip", "Event called when using a UAnimNotifyStatePro Notify State in a Montage.");
		HoverTextOut = FString::Printf(TEXT("%s\n%s"), *ToolTipText.ToString(), *HoverTextOut);
	}
	else if (Pin.PinName == NAME_OnNotifyEnd)
	{
		const FText ToolTipText = LOCTEXT("K2Node_PlayMontagePro_OnNotifyEnd_Tooltip", "Event called when using a UAnimNotifyStatePro Notify State in a Montage.");
		HoverTextOut = FString::Printf(TEXT("%s\n%s"), *ToolTipText.ToString(), *HoverTextOut);
	}
}

#undef LOCTEXT_NAMESPACE
