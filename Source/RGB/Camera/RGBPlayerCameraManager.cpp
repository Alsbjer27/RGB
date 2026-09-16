// Fill out your copyright notice in the Description page of Project Settings.


#include "RGBPlayerCameraManager.h"
#include "RGBArenaCameraZone.h"

void ARGBPlayerCameraManager::SetViewTarget(AActor* NewViewTarget, FViewTargetTransitionParams TransitionParams)
{
    if (NewViewTarget && PendingViewTarget.Target == NewViewTarget) {
        return;
    }

    TransitionParams.BlendFunction = bUseLinearBlend ? VTBlend_Linear : VTBlend_EaseInOut;

    TransitionParams.BlendExp = 2.0f;
    TransitionParams.bLockOutgoing = true;

    Super::SetViewTarget(NewViewTarget, TransitionParams);

    if (PendingViewTarget.Target && bHasBaseView) {
        ViewTarget.POV = LastBaseView;
        BlendStartFlattenAmount = CurrentFlattenAmount;
    }
}

void ARGBPlayerCameraManager::DoUpdateCamera(float DeltaTime)
{
    Super::DoUpdateCamera(DeltaTime);

    LastBaseView = GetCameraCacheView();

    if (!bHasBaseView) {
        CurrentFlattenAmount = GetTargetFlattenAmount(ViewTarget.Target);
        BlendStartFlattenAmount = CurrentFlattenAmount;
        bHasBaseView = true;
    }

    if (PendingViewTarget.Target && BlendParams.BlendTime > 0.0f) {
        const float TimeFraction = FMath::Clamp(1.0f - BlendTimeToGo / BlendParams.BlendTime, 0.0f, 1.0f);

        const float BlendAlpha = BlendParams.GetBlendAlpha(TimeFraction);

        CurrentFlattenAmount = FMath::Lerp(BlendStartFlattenAmount, GetTargetFlattenAmount(PendingViewTarget.Target), BlendAlpha);
    }
    else {
        CurrentFlattenAmount = GetTargetFlattenAmount(ViewTarget.Target);
    }

    FMinimalViewInfo FinalView = LastBaseView;

    if (CurrentFlattenAmount > 0.0f) {
        ApplyDollyZoom(FinalView, CurrentFlattenAmount);
    }

    SetCameraCachePOV(FinalView);
}

bool ARGBPlayerCameraManager::ApplyDollyZoom(FMinimalViewInfo& View, float FlattenAmount) const
{
    if (View.ProjectionMode != ECameraProjectionMode::Perspective) {
        return false;
    }

    const FVector Forward = View.Rotation.Vector();

    if (FMath::Abs(Forward.Y) < KINDA_SMALL_NUMBER) {
        return false;
    }

    const double CurrentDistance = -View.Location.Y / Forward.Y;

    if (!FMath::IsFinite(CurrentDistance) || CurrentDistance <= 0.0 || FlatViewDistance <= 0.0f || !FMath::IsFinite(View.FOV) || View.FOV <= 0.0f || View.FOV >= 179.0f || !FMath::IsFinite(FlatViewDistance)) {
        return false;
    }

    const double Amount = FMath::Clamp(static_cast<double>(FlattenAmount), 0.0, 1.0);
    const double TargetDistance = FMath::Max(CurrentDistance, static_cast<double>(FlatViewDistance));
    const double NewDistance = FMath::Lerp(CurrentDistance, TargetDistance, Amount);
    const double HalfFOVRadians = FMath::DegreesToRadians(static_cast<double>(View.FOV) * 0.5);
    const double VisibleWidth = 2.0 * CurrentDistance * FMath::Tan(HalfFOVRadians);
    const FVector FocusPoint = View.Location + Forward * CurrentDistance;
    
    View.Location = FocusPoint - Forward * NewDistance;

    View.FOV = static_cast<float>(FMath::RadiansToDegrees(2.0 * FMath::Atan(VisibleWidth / (2.0 * NewDistance))));

    return true;
}

float ARGBPlayerCameraManager::GetTargetFlattenAmount(const AActor* Target) const
{
    if (const ARGBArenaCameraZone* Zone = Cast<ARGBArenaCameraZone>(Target)) {
        switch (Zone->GetArenaViewMode())
        {
        case ERGBArenaViewMode::NormalPerspective:
            return 0.0f;

        case ERGBArenaViewMode::FlatPerspective:
            return 1.0f;

        case ERGBArenaViewMode::UseManagerDefault:
            break;
        }
    }

    switch (CameraTestMode) {
    case ERGBCameraTestMode::NormalPerspective:
        return 0.0f;

    case ERGBCameraTestMode::FlatArenaOnly:
        return IsValid(Target) && Target->IsA<ARGBArenaCameraZone>() ? 1.0f : 0.0f;

    case ERGBCameraTestMode::FlatBothViews:
        return 1.0f;
    }
    return 0.0f;
}
