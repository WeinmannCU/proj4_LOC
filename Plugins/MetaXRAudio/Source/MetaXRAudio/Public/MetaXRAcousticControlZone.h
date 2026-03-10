// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "Components/PrimitiveComponent.h"
#include "MetaXRAudioSpectrum.h"
#include "MetaXR_Audio.h"
#include "MetaXR_Audio_AcousticRayTracing.h"

#include "MetaXRAcousticControlZone.generated.h"

#define META_XR_AUDIO_CONTROL_ZONE_MIN_RT60 -5.0f
#define META_XR_AUDIO_CONTROL_ZONE_MAX_RT60 5.0f
#define META_XR_AUDIO_CONTROL_ZONE_MIN_REVERB -24.0f
#define META_XR_AUDIO_CONTROL_ZONE_MAX_REVERB 24.0f
#define META_XR_AUDIO_CONTROL_ZONE_MIN_REFLECTIONS -24.0f
#define META_XR_AUDIO_CONTROL_ZONE_MAX_REFLECTIONS 24.0f

class FMaterialRenderProxy;

UCLASS(
    Placeable,
    ClassGroup = (Audio),
    HideCategories = (Cooking, Physics, Networking),
    meta =
        (BlueprintSpawnableComponent,
         DisplayName = "Meta XR Acoustic Control Zone",
         ToolTip = "Adjust the reverb level and RT60 in a particular region"))
class METAXRAUDIO_API AMetaXRAcousticControlZone : public AActor {
  GENERATED_BODY()

 public:
  AMetaXRAcousticControlZone();

  // The color used to visualize this control zone's boundaries
  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MetaXRAudioControlZone")
  FLinearColor Color = FLinearColor(0.0f, 1.0f, 1.0f, 1.0f);

  // Adjust the blending of the control zone settings with the base map settings outside the boxSize
  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MetaXRAudioControlZone", meta = (ClampMin = "0.0", UIMin = "0.0"))
  float FadeDistance = 100.0f;

  // A scalar (broadband) property describing the early reflections time adjustment in the Zone
  UPROPERTY(
      BlueprintReadWrite,
      EditAnywhere,
      Category = "MetaXRAudioControlZone",
      meta = (ClampMin = "0.01", ClampMax = "10.0", UIMin = "0.01", UIMax = "10.0"))
  float EarlyReflectionsTime = 1.0f;

  UPROPERTY(
      BlueprintReadWrite,
      EditAnywhere,
      Category = "MetaXRAudioControlZone",
      meta = (SpectrumRangeMin = "-5.0f", SpectrumRangeMax = "5.0f", SpectrumGraphColor = "R=0.0,G=0.8,B=0.5", SpectrumRangeLabel = "RT60"))
  FMetaXRAudioSpectrum RT60;

  // Set the RT60 of a control zone
  UFUNCTION(BlueprintCallable, Category = "MetaXRAudioControlZone")
  void SetControlZoneRT60(const TArray<float>& NewRT60Frequencies, const TArray<float>& NewRT60Values);

  // Get the RT60 of a control zone
  UFUNCTION(BlueprintCallable, Category = "MetaXRAudioControlZone")
  void GetControlZoneRT60(TArray<float>& OutRT60Frequencies, TArray<float>& OutRT60Values);

  UPROPERTY(
      BlueprintReadWrite,
      EditAnywhere,
      Category = "MetaXRAudioControlZone",
      meta =
          (SpectrumRangeMin = "-24.0f",
           SpectrumRangeMax = "24.0f",
           SpectrumGraphColor = "R=0.8,G=0.5,B=0.8",
           SpectrumRangeLabel = "Reverb Level"))
  FMetaXRAudioSpectrum ReverbLevel;

  // Set the reverb level of a control zone
  UFUNCTION(BlueprintCallable, Category = "MetaXRAudioControlZone")
  void SetControlZoneReverbLevel(const TArray<float>& NewReverbLevelFrequencies, const TArray<float>& NewReverbLevelValues);

  // Get the reverb level of a control zone
  UFUNCTION(BlueprintCallable, Category = "MetaXRAudioControlZone")
  void GetControlZoneReverbLevel(TArray<float>& OutReverbLevelFrequencies, TArray<float>& OutReverbLevelValues);

  UPROPERTY(
      BlueprintReadWrite,
      EditAnywhere,
      Category = "MetaXRAudioControlZone",
      meta =
          (SpectrumRangeMin = "-24.0f",
           SpectrumRangeMax = "24.0f",
           SpectrumGraphColor = "R=0.8,G=0.5,B=0.05",
           SpectrumRangeLabel = "Early Reflection Level"))
  FMetaXRAudioSpectrum EarlyReflectionsLevel;

  // Set the Early Reflections level of a control zone
  UFUNCTION(BlueprintCallable, Category = "MetaXRAudioControlZone")
  void SetControlZoneEarlyReflectionsLevel(
      const TArray<float>& NewEarlyReflectionsLevelFrequencies,
      const TArray<float>& NewEarlyReflectionsLevelValues);

  // Get the Early Reflections level of a control zone
  UFUNCTION(BlueprintCallable, Category = "MetaXRAudioControlZone")
  void GetControlZoneEarlyReflectionsLevel(
      TArray<float>& OutEarlyReflectionsLevelFrequencies,
      TArray<float>& OutEarlyReflectionsLevelValues);

  // The scene component provides the actor a reference so it can be transformed
  UPROPERTY()
  USceneComponent* MyRootComponent;

  void GetNativeSizes(FVector& OutBoxSize, FVector& OutFadeDistance) const;
  void ApplyProperties();

 private:
  virtual void BeginPlay() override;
  virtual void Tick(float DeltaTime) override;
  virtual void BeginDestroy() override;
  virtual void PostUnregisterAllComponents() override;
  void StartInternal();
  void DestroyInternal();
  void ApplyTransform();
  const bool ShouldApplyProperties() const;

 private:
  ovrAudioContext CachedContext;
  ovrAudioControlZone ControlZoneHandle = nullptr;
  FTransform PreviousTransform;
  float PreviousFadeDistance;
  float PreviousEarlyReflectionsTime;
};

// This acts only as a Dummy Component for visualizing Control Zone
// using: FMetaXRAcousticControlZoneVisualizer
UCLASS(Hidden)
class METAXRAUDIO_API UMetaXRAcousticControlZoneWrapper final : public UActorComponent {
  GENERATED_BODY()

 public:
  void BeginDestroy() final;

  const FLinearColor& GetVisualizerColor() const;
  void GetNativeSizes(FVector& OutBoxSize, FVector& OutFadeDistance) const;
  FMatrix GetWorldMatrix() const;
  const FMaterialRenderProxy* GetVisualizerProxy();

 private:
  UPROPERTY(Transient)
  TObjectPtr<UMaterialInstanceDynamic> VisualizerMaterial = nullptr;

  FLinearColor PrevColorUpdate;
};
