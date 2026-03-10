// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
// Copyright Epic Games, Inc. All Rights Reserved.
#include "MetaXRAcousticControlZone.h"
#include "IMetaXRAudioPlugin.h"
#include "MetaXRAudioDllManager.h"
#include "MetaXRAudioLogging.h"
#include "MetaXRAudioPlatform.h"
#include "MetaXRAudioUtilities.h"
#ifdef META_NATIVE_UNREAL_PLUGIN
#include "MetaXRAudioContextManager.h"
#endif
#include "Materials/MaterialInstanceDynamic.h"

AMetaXRAcousticControlZone::AMetaXRAcousticControlZone() {
  MyRootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("MySceneComponent"));
  RootComponent = MyRootComponent;
  CreateDefaultSubobject<UMetaXRAcousticControlZoneWrapper>(TEXT("MyControlZone"), true);
  PrimaryActorTick.bCanEverTick = true;
  PrimaryActorTick.bStartWithTickEnabled = true;
}

void AMetaXRAcousticControlZone::BeginPlay() {
  Super::BeginPlay();
  // Only actually create the control zone during play mode
  if (MetaXRAudioUtilities::PlayModeActive(GetWorld())) {
    StartInternal();
  }

  if (ControlZoneHandle == nullptr) {
    return;
  }

  ApplyProperties();

  if (OVRA_CALL(ovrAudio_ControlZoneSetEnabled)(ControlZoneHandle, true) != ovrSuccess) {
    UE_LOG(LogAudio, Error, TEXT("Unable to enable Control Zone"));
  } else {
    UE_LOG(LogAudio, Log, TEXT("Enabled Control Zone %p"), ControlZoneHandle);
  }
}

void AMetaXRAcousticControlZone::Tick(float DeltaTime) {
  Super::Tick(DeltaTime);
  if (ControlZoneHandle == nullptr) {
    return;
  }

  // Check if the transform was changed.
  FTransform Transform = GetTransform();
  if (!Transform.Equals(PreviousTransform) || (FadeDistance != PreviousFadeDistance)) {
    ApplyTransform();
  }

  // Check if the spectrums were updated
  if (ShouldApplyProperties()) {
    ApplyProperties();
  }
}

void AMetaXRAcousticControlZone::BeginDestroy() {
  Super::BeginDestroy();
  DestroyInternal();
}

void AMetaXRAcousticControlZone::PostUnregisterAllComponents() {
  Super::PostUnregisterAllComponents();
  DestroyInternal();
}

void AMetaXRAcousticControlZone::StartInternal() {
  // Ensure the control zone is not initialized twice.
  if (ControlZoneHandle == nullptr) {
    // Create the internal Control Zone.
#ifdef META_NATIVE_UNREAL_PLUGIN
    if (OVRA_CALL(ovrAudio_CreateControlZone)(FMetaXRAudioContextManager::GetContext(this), &ControlZoneHandle) != ovrSuccess) {
      UE_LOG(
          LogAudio,
          Error,
          TEXT(
              "Unable to create Control Zone. Please check that the Spatialization Plugin and Reverb Plugin are set to Meta XR Audio in project settings. Restart the project after setting."));
#else
    if (OVRA_CALL(ovrAudio_CreateControlZone)(FMetaXRAudioLibraryManager::Get().GetPluginContext(), &ControlZoneHandle) != ovrSuccess) {
      UE_LOG(LogAudio, Error, TEXT("Unable to create Control Zone. Please check the Meta XR Audio binaries have been placed correctly."));
#endif
      return;
    } else {
      UE_LOG(LogAudio, Log, TEXT("Created Control Zone %p"), ControlZoneHandle);
    }
  }

  // Run the updates to initialize the control.
  ApplyProperties();
  ApplyTransform();
}

void AMetaXRAcousticControlZone::DestroyInternal() {
  if (ControlZoneHandle != nullptr) {
    UE_LOG(LogAudio, Log, TEXT("Destroying Control Zone %p"), ControlZoneHandle);
    if (OVRA_CALL(ovrAudio_DestroyControlZone)(ControlZoneHandle) != ovrSuccess) {
      UE_LOG(LogAudio, Error, TEXT("Unable to destroy Control Zone"));
      return;
    }
    ControlZoneHandle = nullptr;
  }
}

void AMetaXRAcousticControlZone::GetNativeSizes(FVector& OutBoxSize, FVector& OutFadeDistance) const {
  const FVector Scale = GetTransform().GetScale3D();
  OutFadeDistance =
      FVector(Scale.X ? FadeDistance / Scale.X : 0.0f, Scale.Y ? FadeDistance / Scale.Y : 0.0f, Scale.Z ? FadeDistance / Scale.Z : 0.0f);
  OutBoxSize = FVector(200.0f + OutFadeDistance.X, 200.0f + OutFadeDistance.Y, 200.0f + OutFadeDistance.Z);
}

void AMetaXRAcousticControlZone::ApplyTransform() {
  // Note both box size and fade distance must convert from UE space to Audio SDK space
  // UE:        x:forward, y:right, z:up
  // Meta XR:  x:right,   y:up,    z:backward
  FTransform UETransform = GetTransform();
  float OVRTransform[16];
  MetaXRAudioUtilities::ConvertUETransformToOVRTransform(UETransform, OVRTransform);

  if (OVRA_CALL(ovrAudio_ControlZoneSetTransform)(ControlZoneHandle, OVRTransform) != ovrSuccess) {
    UE_LOG(LogAudio, Log, TEXT("Failed to set transform for Control Zone %p"), ControlZoneHandle);
  } else {
    UE_LOG(LogAudio, Log, TEXT("Set transform for Control Zone %p"), ControlZoneHandle);
  }

  PreviousTransform = UETransform;

  // Box Size (converted from ovrAudio coordinates to UE coordinates)
  FVector NativeBoxSize, NativeFadeDistance;
  GetNativeSizes(NativeBoxSize, NativeFadeDistance);
  if (OVRA_CALL(ovrAudio_ControlZoneSetBox)(ControlZoneHandle, NativeBoxSize.Y, NativeBoxSize.Z, NativeBoxSize.X) != ovrSuccess) {
    UE_LOG(LogAudio, Error, TEXT("Failed to set box for Control Zone %p"), ControlZoneHandle);
  } else {
    UE_LOG(
        LogAudio,
        Log,
        TEXT("Set box size for Control Zone %p to {%f, %f ,%f}"),
        ControlZoneHandle,
        NativeBoxSize.X,
        NativeBoxSize.Y,
        NativeBoxSize.Z);
  }

  // Fade Distance (converted from ovrAudio coordinates to UE coordinates)
  if (OVRA_CALL(ovrAudio_ControlZoneSetFadeDistance)(ControlZoneHandle, NativeFadeDistance.Y, NativeFadeDistance.Z, NativeFadeDistance.X) !=
      ovrSuccess) {
    UE_LOG(LogAudio, Error, TEXT("Failed to set fade distance for Control Zone %p"), ControlZoneHandle);
  } else {
    UE_LOG(
        LogAudio,
        Log,
        TEXT("Set fade distance for Control Zone %p to {%f, %f ,%f}"),
        ControlZoneHandle,
        NativeFadeDistance.X,
        NativeFadeDistance.Y,
        NativeFadeDistance.Z);
  }

  PreviousFadeDistance = FadeDistance;
}

const bool AMetaXRAcousticControlZone::ShouldApplyProperties() const {
  return ReverbLevel.IsDirty || RT60.IsDirty || EarlyReflectionsLevel.IsDirty ||
      FMath::Abs(EarlyReflectionsTime - PreviousEarlyReflectionsTime) > 0.0001f;
}

void UploadSpectrumDataToOVR(
    ovrAudioControlZone ControlZoneHandle,
    FMetaXRAudioSpectrum* Spectrum,
    ovrAudioControlZoneProperty ControlZoneProperty) {
  if (OVRA_CALL(ovrAudio_ControlZoneReset)(ControlZoneHandle, ControlZoneProperty) != ovrSuccess) {
    METAXR_AUDIO_LOG_ERROR("Failed to reset property %i for Control Zone %p", ControlZoneProperty, ControlZoneHandle);
  }
  for (FMetaXRAudioPoint p : Spectrum->Points) {
    if (OVRA_CALL(ovrAudio_ControlZoneSetFrequency)(ControlZoneHandle, ControlZoneProperty, p.Frequency, p.Data) != ovrSuccess) {
      METAXR_AUDIO_LOG_ERROR("Failed to set property %i for Control Zone %p", ControlZoneProperty, ControlZoneHandle);
    } else {
      METAXR_AUDIO_LOG("Set property %i for Control Zone %p for %f Hz to %f", ControlZoneProperty, ControlZoneHandle, p.Frequency, p.Data);
    }
  }
  Spectrum->IsDirty = false;
}

void AMetaXRAcousticControlZone::ApplyProperties() {
  if (ControlZoneHandle == nullptr) {
    return;
  }

  UploadSpectrumDataToOVR(ControlZoneHandle, &RT60, ovrAudioControlZoneProperty_RT60);
  UploadSpectrumDataToOVR(ControlZoneHandle, &ReverbLevel, ovrAudioControlZoneProperty_ReverbLevel);
  UploadSpectrumDataToOVR(ControlZoneHandle, &EarlyReflectionsLevel, ovrAudioControlZoneProperty_ReflectionsLevel);

  if (OVRA_CALL(ovrAudio_ControlZoneReset)(ControlZoneHandle, ovrAudioControlZoneProperty_ReflectionsTime) != ovrSuccess) {
    METAXR_AUDIO_LOG_ERROR("Failed to reset Early Reflections Time for Control Zone %p", ControlZoneHandle);
  }
  if (OVRA_CALL(ovrAudio_ControlZoneSetFrequency)(
          ControlZoneHandle, ovrAudioControlZoneProperty_ReflectionsTime, 1000.0f, EarlyReflectionsTime) != ovrSuccess) {
    METAXR_AUDIO_LOG_ERROR("Failed to set Early Reflections Time for Control Zone %p", ControlZoneHandle);
  }

  PreviousEarlyReflectionsTime = EarlyReflectionsTime;
}

void SaveNewSpectrumData(
    FMetaXRAudioSpectrum* Spectrum,
    const TArray<float>& NewFrequencies,
    const TArray<float>& NewValues,
    float ValueMin,
    float ValueMax) {
  if (NewFrequencies.Num() != NewValues.Num()) {
    METAXR_AUDIO_LOG_ERROR("Cannot save control zone data because the frequencies and values arrays are not of equal length");
    return;
  }
  int NumPoints = FMath::Max(0, FMath::Min(META_XR_AUDIO_MAX_NUM_SPECTRUM_POINTS, NewFrequencies.Num()));
  Spectrum->Points.Empty();
  Spectrum->Points.SetNum(NumPoints);
  for (int i = 0; i < NumPoints; ++i) {
    float Frequency = FMath::Max(META_XR_AUDIO_MIN_FREQUENCY, FMath::Min(META_XR_AUDIO_MAX_FREQUENCY, NewFrequencies[i]));
    float Value = FMath::Max(ValueMin, FMath::Min(ValueMax, NewValues[i]));
    Spectrum->Points[i] = FMetaXRAudioPoint{Frequency, Value};
  }
  Spectrum->IsDirty = true;
}

void GetSpectrumData(const FMetaXRAudioSpectrum* Spectrum, TArray<float>& OutFrequencies, TArray<float>& OutValues) {
  const int32 PointCount = Spectrum->PointCount();
  OutFrequencies.SetNum(PointCount);
  OutValues.SetNum(PointCount);
  for (int i = 0; i < PointCount; ++i) {
    OutFrequencies[i] = Spectrum->Points[i].Frequency;
    OutValues[i] = Spectrum->Points[i].Data;
  }
}

void AMetaXRAcousticControlZone::SetControlZoneRT60(const TArray<float>& NewRT60Frequencies, const TArray<float>& NewRT60Values) {
  SaveNewSpectrumData(&RT60, NewRT60Frequencies, NewRT60Values, META_XR_AUDIO_CONTROL_ZONE_MIN_RT60, META_XR_AUDIO_CONTROL_ZONE_MAX_RT60);
}

void AMetaXRAcousticControlZone::GetControlZoneRT60(TArray<float>& OutRT60Frequencies, TArray<float>& OutRT60Values) {
  GetSpectrumData(&RT60, OutRT60Frequencies, OutRT60Values);
}

void AMetaXRAcousticControlZone::SetControlZoneReverbLevel(
    const TArray<float>& NewReverbLevelFrequencies,
    const TArray<float>& NewReverbLevelValues) {
  SaveNewSpectrumData(
      &ReverbLevel,
      NewReverbLevelFrequencies,
      NewReverbLevelValues,
      META_XR_AUDIO_CONTROL_ZONE_MIN_REVERB,
      META_XR_AUDIO_CONTROL_ZONE_MAX_REVERB);
}

void AMetaXRAcousticControlZone::GetControlZoneReverbLevel(TArray<float>& OutReverbLevelFrequencies, TArray<float>& OutReverbLevelValues) {
  GetSpectrumData(&ReverbLevel, OutReverbLevelFrequencies, OutReverbLevelValues);
}

void AMetaXRAcousticControlZone::SetControlZoneEarlyReflectionsLevel(
    const TArray<float>& NewEarlyReflectionsLevelFrequencies,
    const TArray<float>& NewEarlyReflectionsLevelValues) {
  SaveNewSpectrumData(
      &EarlyReflectionsLevel,
      NewEarlyReflectionsLevelFrequencies,
      NewEarlyReflectionsLevelValues,
      META_XR_AUDIO_CONTROL_ZONE_MIN_REVERB,
      META_XR_AUDIO_CONTROL_ZONE_MAX_REVERB);
}

void AMetaXRAcousticControlZone::GetControlZoneEarlyReflectionsLevel(
    TArray<float>& OutEarlyReflectionsLevelFrequencies,
    TArray<float>& OutEarlyReflectionsLevelValues) {
  GetSpectrumData(&EarlyReflectionsLevel, OutEarlyReflectionsLevelFrequencies, OutEarlyReflectionsLevelValues);
}

void UMetaXRAcousticControlZoneWrapper::BeginDestroy() {
  Super::BeginDestroy();

  if (VisualizerMaterial) {
    VisualizerMaterial->MarkAsGarbage();
    VisualizerMaterial = nullptr;
  }
}

const FLinearColor& UMetaXRAcousticControlZoneWrapper::GetVisualizerColor() const {
  const AMetaXRAcousticControlZone* RealControlZone = Cast<const AMetaXRAcousticControlZone>(GetOwner());
  checkf(
      RealControlZone,
      TEXT("This component only acts as a dummy component for visualizing control zone actors. It should not be used elsewhere"));
  return RealControlZone->Color;
}

void UMetaXRAcousticControlZoneWrapper::GetNativeSizes(FVector& OutBoxSize, FVector& OutFadeDistance) const {
  const AMetaXRAcousticControlZone* RealControlZone = Cast<const AMetaXRAcousticControlZone>(GetOwner());
  checkf(
      RealControlZone,
      TEXT("This component only acts as a dummy component for visualizing control zone actors. It should not be used elsewhere"));
  return RealControlZone->GetNativeSizes(OutBoxSize, OutFadeDistance);
}

FMatrix UMetaXRAcousticControlZoneWrapper::GetWorldMatrix() const {
  const AMetaXRAcousticControlZone* RealControlZone = Cast<const AMetaXRAcousticControlZone>(GetOwner());
  checkf(
      RealControlZone,
      TEXT("This component only acts as a dummy component for visualizing control zone actors. It should not be used elsewhere"));
  return RealControlZone->GetTransform().ToMatrixWithScale();
}

const FMaterialRenderProxy* UMetaXRAcousticControlZoneWrapper::GetVisualizerProxy() {
#if WITH_EDITORONLY_DATA
  if (!VisualizerMaterial) {
    VisualizerMaterial = UMaterialInstanceDynamic::Create(GEngine->GeomMaterial, GetOuter(), FName("ControlZoneVisualizerMat"));
    if (!VisualizerMaterial)
      return nullptr;
  }

  const FLinearColor& Color = GetVisualizerColor();

  if (PrevColorUpdate.Equals(Color, 0.0001f))
    return VisualizerMaterial->GetRenderProxy();

  static const FName ColorParamID("Color");
  VisualizerMaterial->SetVectorParameterValue(ColorParamID, Color);
  PrevColorUpdate = Color;
  return VisualizerMaterial->GetRenderProxy();
#else
  return nullptr;
#endif
}
