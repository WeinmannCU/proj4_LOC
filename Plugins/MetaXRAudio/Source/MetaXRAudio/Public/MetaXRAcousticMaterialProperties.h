// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetUserData.h"
#include "Engine/DataAsset.h"
#include "MetaXRAudioRoomAcousticProperties.h"
#include "MetaXRAudioSpectrum.h"
#include "MetaXR_Audio.h"
#include "MetaXR_Audio_AcousticRayTracing.h"

#include "MetaXRAcousticMaterialProperties.generated.h"

#define META_XR_AUDIO_MIN_ABSOPRTION 0.0f
#define META_XR_AUDIO_MAX_ABSOPRTION 1.0f
#define META_XR_AUDIO_MIN_TRANSMISSION 0.0f
#define META_XR_AUDIO_MAX_TRANSMISSION 1.0f
#define META_XR_AUDIO_MIN_SCATTERING 0.0f
#define META_XR_AUDIO_MAX_SCATTERING 1.0f
#define META_XR_AUDIO_DEFAULT_MATERIAL EMetaXRAudioMaterialPreset::MetaDefault

USTRUCT(BlueprintType)
struct FMetaXRAcousticMaterialData {
  GENERATED_BODY()

  UPROPERTY(
      BlueprintReadWrite,
      EditAnywhere,
      Category = "AcousticMaterialProperties",
      meta =
          (SpectrumRangeMin = "0.0",
           SpectrumRangeMax = "1.0",
           SpectrumGraphColor = "R=0.52,G=0.68,B=1.0",
           SpectrumRangeLabel = "Absorption"))
  FMetaXRAudioSpectrum Absorption;

  UPROPERTY(
      BlueprintReadWrite,
      EditAnywhere,
      Category = "AcousticMaterialProperties",
      meta =
          (SpectrumRangeMin = "0.0",
           SpectrumRangeMax = "1.0",
           SpectrumGraphColor = "R=1.0,G=0.66,B=0.03",
           SpectrumRangeLabel = "Transmission"))
  FMetaXRAudioSpectrum Transmission;

  UPROPERTY(
      BlueprintReadWrite,
      EditAnywhere,
      Category = "AcousticMaterialProperties",
      meta =
          (SpectrumRangeMin = "0.0",
           SpectrumRangeMax = "1.0",
           SpectrumGraphColor = "R=1.0,G=0.25,B=0.25",
           SpectrumRangeLabel = "Scattering"))
  FMetaXRAudioSpectrum Scattering;

  void Clone(const FMetaXRAcousticMaterialData& Other) {
    Absorption.Clone(Other.Absorption);
    Transmission.Clone(Other.Transmission);
    Scattering.Clone(Other.Scattering);
  }

  bool IsEmpty() const {
    return Absorption.Points.Num() == 0 && Transmission.Points.Num() == 0 && Scattering.Points.Num() == 0;
  }

  void ApplyPreset(EMetaXRAudioMaterialPreset Preset);
};

UCLASS(BlueprintType)
class METAXRAUDIO_API UMetaXRAcousticMaterialProperties : public UDataAsset {
  GENERATED_BODY()

 public:
  UMetaXRAcousticMaterialProperties() {
    ApplyPreset(Preset);
  }

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AcousticMaterialProperties")
  FMetaXRAcousticMaterialData Data;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AcousticMaterialProperties")
  EMetaXRAudioMaterialPreset Preset = META_XR_AUDIO_DEFAULT_MATERIAL;

  // The color used to visualize an Acoustic Geometry set to use this material
  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AcousticMaterialProperties")
  FLinearColor Color = FLinearColor::Yellow;

  void AppendHash(FString& hash) const;

  void ConstructMaterial(ovrAudioMaterial Material);

  void ApplyPreset(EMetaXRAudioMaterialPreset NewPreset) {
    Data.ApplyPreset(NewPreset);
    Preset = NewPreset;
  }

// When the Editor detects a change to this UDataAsset,
// we need to apply the preset to our spectrum. Otherwise, changing the enum will
// not result in corresponding change to spectrum widget while the details panel is open.
#if WITH_EDITOR
  void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) final {
    Super::PostEditChangeProperty(PropertyChangedEvent);

    const FName ChangedProperty = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
    if (ChangedProperty == GET_MEMBER_NAME_CHECKED(UMetaXRAcousticMaterialProperties, Preset))
      Data.ApplyPreset(Preset);
  }
#endif
};

// Asset User Data applied to a USceneComponent (though its primarly used with UStaticMeshComponent's).
// IF applied to Static Mesh Component, this tells the Acoustic Geometry component that MaterialPreset
// is being applied to that specific mesh.
UCLASS()
class METAXRAUDIO_API UAcousticMaterialUserData : public UAssetUserData {
  GENERATED_BODY()

 public:
  // NOTE: only 1 UAcousticMaterialUserData is allowed per Static Mesh, BUT you can specify multiple
  // UMetaXRAcousticMaterialProperties per UAcousticMaterialUserData.
  // IF multiple UMetaXRAcousticMaterialProperties are specified THEN 0 would apply to Submesh 0 of the static mesh, 1 to submesh 1...
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Acoustic Material UserData")
  TArray<TObjectPtr<UMetaXRAcousticMaterialProperties>> MaterialPresets;
};
