# PlayMontagePro (PMP) <img align="right" width=128, height=128 src="https://github.com/Vaei/PlayMontagePro/blob/main/Resources/Icon128.png">

> [!IMPORTANT]
> **Play Montage Pro (PMP)**
> <br>Reliable Gameplay Notify System
> <br>Multi-Mesh/Montage Support
> <br>Additional Blending Parameters
> <br>And its **FREE!**

> [!TIP]
> Suitable for both singleplayer and multiplayer games
> <br>Supports UE5.4+

> [!CAUTION]
> PMP is currently in beta
> <br>There are many non-standard setups that have not been fully tested

## Why use PMP?

> [!CAUTION]
> Did you know? Anim Notifies are not guaranteed to trigger :confused:

> [!NOTE]
> Have you heard the common phrase "Animation should not affect gameplay"?
> <br>But you've probably already built notifies that reload your weapon - and more - without experiencing issues :disguised_face:
> <br><br>Its a trap that catches you later in production: The heavier your game gets, the less likely Anim Notifies are to fire :dizzy_face: 
> <br><br>This is true for Queued notifies. The alternative is Branching Point notifies
> <br>However, when two or more Branching Point notifies trigger on the same frame, only one will trigger! :skull:
> <br><br>What if you had a notify that changes the movement mode away from Flying? You can get **stuck** in Flying! :space_invader:
> <br><br>There is no anim notify system in Unreal that ensures your notifies will fire reliably. PMP is the solution to this. :boom:

> [!TIP]
> At it's core PMP uses timers to trigger notifies when you play a montage or change montage sections
> <br>Timers are reliable! :rocket:

## When to use PMP

When your notifies need to affect gameplay reliably, use PMP.

Otherwise, use the existing notify system.

## Features

* ProNotifySystem
	* Gameplay Timers triggering notifies reliably
 	* Trigger notifies placed prior to the anim start time
  	* Ensure notifies trigger on anim end, even if they were not reached
* Multi-mesh support with Driver, Replicated Driven, and Local Driven Montages (`gas-pro` branch only)
	* Driven Montages optionally match the duration of the Driver montage
 	* Example use-case: TP character mesh Reloads (Driver), so their TP weapon plays a matching replicated driven montage (replicated so simulated proxies play the montage), FP character mesh and weapon both play their own Local Driven Montages (not replicated)
  * Additional Blend in and out parameters (`gas-pro` branch only)

## Limitations

> [!TIP]
> ProNotifySystem is timer-based and does not run through the animation system
> <br>This makes it reliable, but it works differently, and may produce different results.

* Anim Notify States supported Start and End - But not Tick
* Only supports notifies on Montages not their AnimSequences
* Trigger Settings such as `NotifyTriggerChance` will not do anything
	* You can optionally override `ShouldTriggerNotify()` in C++ to implement this behaviour yourself
 * SimulatedProxies typically don't get calls to play montages thus cannot operate on timers and don't support Pro Notifies as a result
 	* SimulatedProxies as well as Editor can optionally use the engine's notify system instead
  * `FAnimNotifyEventReference` does not exist for notify callbacks
  * `CustomTimeDilation` is a per-actor Time Dilation, however there are no callbacks or even setter for this property
  	* ProNotifySystem relies on `USkinnedMeshComponent::OnTickPose` to detect changes. If your dedicated server doesn't tick the mesh pose it will not work.
   	* There is likely a performance overhead with enabling this

## Considerations

* ProNotifySystem is very different to Epic's implementation
	* Triggering behaviour may not always be identical - but you can report this if you believe it is a bug
 	* Use this for gameplay critical systems only, and use Epic's for cosmetics
  	* Because SimulatedProxies and Editor run on Epic's system, results may not be consistent

## Get PMP

There are 3 branches available. The precompiled binaries are for `gas-pro` branch only as it contains all the features.

* `main`	`PlayMontagePro()` only, no GAS dependency, supports Pro Notify System only
* `gas` 	`PlayMontageProAndWait()` with support for gameplay abilities, supports Pro Notify System only
* `gas-pro`:	`PlayMontageProAdvancedAndWait()`, with support for multiple driven meshes and gameplay events and additional blend parameters

> [!WARNING]
> [Download the pre-compiled binaries here](https://github.com/Vaei/PlayMontagePro/wiki/How-to-Use)

## How to Use
> [!IMPORTANT]
> [Read the Wiki to Learn How to use PlayMontagePro](https://github.com/Vaei/PlayMontagePro/wiki/How-to-Use)

## Credits
Code was used from [GASShooter](https://github.com/tranek/GASShooter/) for multi-mesh/montage support and events

## Changelog

### 1.1.1
* Switch to pale yellow notify color to differientate

### 1.1.0
* Stop checking `ShouldBroadcastAbilityTaskDelegates()` for `EnsureBroadcastNotifyEvents()`
	* Causing unintended behaviour and these notifies already track their broadcast states themselves
	* Fixes a bug with ending montage due to ability end in particular.
* Add `EnsureBroadcastNotifyEvents()` to `OnDestroy()`
* `EnsureBroadcastNotifyEvents()` from `OnMontageBlendingOut()` no longer inappropriately ensures end states for notify states
* Re-order `UPlayMontageProCallbackProxy` broadcast/ensure for consistency with other nodes

### 1.0.0
* Initial Release
