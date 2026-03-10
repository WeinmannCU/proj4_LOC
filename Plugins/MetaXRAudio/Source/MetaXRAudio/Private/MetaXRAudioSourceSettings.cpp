// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
// Copyright Epic Games, Inc. All Rights Reserved.

#include "MetaXRAudioSourceSettings.h"
#include "MetaXRAudioDllManager.h"
#include "MetaXRAudioUtilities.h"
#include "MetaXR_Audio.h"
#include "MetaXR_Audio_AcousticRayTracing.h"

UMetaXRAudioSourceSettings::UMetaXRAudioSourceSettings()
    : bEnableAcoustics(true),
      bEnableSpatialization(true),
      GainBoostLevelDecibels(0.0f),
      ReverbSendLevelDecibels(0.0f),
      bEnableMetaDistanceAttenuation(true),
      HRTFIntensity(1.0f),
      VolumetricRadius(0.0f),
      EarlyReflectionsSendLevelDecibels(0.0f),
      DirectivityPattern(EMetaXRAudioDirectivityPattern::None),
      DirectivityIntensity(1.0f),
      bDirectSoundEnabled(true),
      ReverbReach(0.5f),
      OcclusionIntensity(1.0f),
      bMediumAbsorption(true) {}

// Apply these source settings to a given context's source
void UMetaXRAudioSourceSettings::UpdateParameters(ovrAudioContext Context, uint32 ChangedSourceId) {
  TRACE_CPUPROFILER_EVENT_SCOPE_STR("UMetaXRAudioSourceSettings::UpdateParameters");

  uint32 Flags = 0;
  if (!bEnableAcoustics) {
    Flags |= ovrAudioSourceFlag_ReflectionsDisabled;
  }
  if (bMediumAbsorption) {
    Flags |= ovrAudioSourceFlag_MediumAbsorption;
  }

  ovrResult Result = OVRA_CALL(ovrAudio_SetAudioSourceFlags)(Context, ChangedSourceId, Flags);
  OVR_AUDIO_CHECK(Result, "Failed to set audio source flags");

  ovrAudioSourceAttenuationMode mode =
      bEnableMetaDistanceAttenuation ? ovrAudioSourceAttenuationMode_InverseSquare : ovrAudioSourceAttenuationMode_Fixed;
  Result = OVRA_CALL(ovrAudio_SetAudioSourceAttenuationMode)(
      Context, ChangedSourceId, mode, MetaXRAudioUtilities::dbToLinear(GainBoostLevelDecibels));
  OVR_AUDIO_CHECK(Result, "Failed to set audio source attenuation mode");

  Result = OVRA_CALL(ovrAudio_SetAudioSourceRadius)(Context, ChangedSourceId, VolumetricRadius);
  OVR_AUDIO_CHECK(Result, "Failed to set audio source volumetric radius");

  Result = OVRA_CALL(ovrAudio_SetAudioReverbSendLevel)(Context, ChangedSourceId, MetaXRAudioUtilities::dbToLinear(ReverbSendLevelDecibels));
  OVR_AUDIO_CHECK(Result, "Failed to set reverb send level");

  Result = OVRA_CALL(ovrAudio_SetAudioHrtfIntensity)(Context, ChangedSourceId, HRTFIntensity);
  OVR_AUDIO_CHECK(Result, "Failed to set HRTF intensity");

  Result = OVRA_CALL(ovrAudio_SetAudioReflectionsSendLevel)(
      Context, ChangedSourceId, MetaXRAudioUtilities::dbToLinear(EarlyReflectionsSendLevelDecibels));
  OVR_AUDIO_CHECK(Result, "Failed to set early reflections send level");

  Result = OVRA_CALL(ovrAudio_SetSourceDirectivityEnabled)(
      Context, ChangedSourceId, DirectivityPattern == EMetaXRAudioDirectivityPattern::None ? 0 : 1);
  OVR_AUDIO_CHECK(Result, "Failed to set set source directivity enabled");

  Result = OVRA_CALL(ovrAudio_SetSourceDirectivityIntensity)(Context, ChangedSourceId, DirectivityIntensity);
  OVR_AUDIO_CHECK(Result, "Failed to set set source directivity intensity");

  Result = OVRA_CALL(ovrAudio_SetSourceDirectEnabled)(Context, ChangedSourceId, bDirectSoundEnabled);
  OVR_AUDIO_CHECK(Result, "Failed to set set source direct sound enabled");

  Result = OVRA_CALL(ovrAudio_SetAudioSourceReverbReach)(Context, ChangedSourceId, ReverbReach);
  OVR_AUDIO_CHECK(Result, "Failed to set set source reverb reach");

  Result = OVRA_CALL(ovrAudio_SetSourceOcclusionIntensity)(Context, ChangedSourceId, OcclusionIntensity);
  OVR_AUDIO_CHECK(Result, "Failed to set set source occlusion Intensity");
}
