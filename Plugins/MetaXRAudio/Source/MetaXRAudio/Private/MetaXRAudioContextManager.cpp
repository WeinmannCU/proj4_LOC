// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
// Copyright Epic Games, Inc. All Rights Reserved.

#include "MetaXRAudioContextManager.h"
#include "AudioMixerDevice.h"
#include "Engine/World.h"
#include "Features/IModularFeatures.h"
#include "IMetaXRAudioPlugin.h"
#include "MetaXRAmbisonicSpatializer.h"
#include "MetaXRAudioLogging.h"
#include "MetaXRAudioReverb.h"
#include "MetaXRAudioSettings.h"
#include "ProfilingDebugging/CountersTrace.h"

TRACE_DECLARE_INT_COUNTER(COUNTER_METAXRAUDIO_NumListeners, TEXT("MetaXR/Audio/NumListeners"));
TRACE_DECLARE_INT_COUNTER(COUNTER_METAXRAUDIO_NumContexts, TEXT("MetaXR/Audio/NumContexts"));

ovrAudioContext FMetaXRAudioContextManager::SerializationContext = nullptr;
FCriticalSection FMetaXRAudioContextManager::SerializationContextLock;
TMap<const FAudioDevice*, ovrAudioContext> FMetaXRAudioContextManager::ContextMap;
FCriticalSection FMetaXRAudioContextManager::ContextCritSection;

FMetaXRAudioContextManager::FMetaXRAudioContextManager() {
  // empty
}

FMetaXRAudioContextManager::~FMetaXRAudioContextManager() {
  SerializationContext = nullptr;
}

void FMetaXRAudioContextManager::OnListenerInitialize(FAudioDevice* AudioDevice, UWorld* ListenerWorld) {
  TRACE_COUNTER_INCREMENT(COUNTER_METAXRAUDIO_NumListeners);
}

void FMetaXRAudioContextManager::OnListenerShutdown(FAudioDevice* AudioDevice) {
  FMetaXRAudioPlugin* Plugin = &FModuleManager::GetModuleChecked<FMetaXRAudioPlugin>("MetaXRAudio");
  const FString MetaXRSpatializerPluginName = Plugin->GetSpatializationPluginFactory()->GetDisplayName();
  const FString CurrentSpatializerPluginName = AudioPluginUtilities::GetDesiredPluginName(EAudioPlugin::SPATIALIZATION);
  if (CurrentSpatializerPluginName.Equals(MetaXRSpatializerPluginName)) // we have a match!
  {
    MetaXRAudioSpatializationAudioMixer* Spatializer =
        static_cast<MetaXRAudioSpatializationAudioMixer*>(AudioDevice->GetSpatializationPluginInterface().Get());

    if (Spatializer != nullptr) {
      Spatializer->ClearContext();
    }
  }

  const FString MetaXRReverbPluginName = Plugin->GetReverbPluginFactory()->GetDisplayName();
  const FString CurrentReverbPluginName = AudioPluginUtilities::GetDesiredPluginName(EAudioPlugin::REVERB);
  if (CurrentReverbPluginName.Equals(MetaXRReverbPluginName)) {
    MetaXRAudioReverb* Reverb = static_cast<MetaXRAudioReverb*>(AudioDevice->ReverbPluginInterface.Get());

    if (Reverb != nullptr) {
      Reverb->ClearContext();
    }
  }

  // FIXME:
  // There's a possibility this will leak if a Meta XR Binaural submix is created,
  // but Meta XR audio is not specified as the spatialization or reverb plugin.
  // This is a niche use case, but could be solved by having the Meta XR soundfield explicitly destroy
  // a context.
  DestroyContextForAudioDevice(AudioDevice);
  SerializationContext = nullptr;
  TRACE_COUNTER_DECREMENT(COUNTER_METAXRAUDIO_NumListeners);
}

ovrAudioContext FMetaXRAudioContextManager::GetOrCreateSerializationContext(const AActor* Parent) {
  FScopeLock ScopeLock(&SerializationContextLock);

  ovrAudioContext PluginContext = FMetaXRAudioLibraryManager::Get().GetPluginContext();
  if (PluginContext != nullptr) {
    return PluginContext;
  }

  if (SerializationContext == nullptr) {
    ovrResult Result = OVRA_CALL(ovrAudio_CreateContext)(&SerializationContext, nullptr);
    if (Result != ovrSuccess) {
      const TCHAR* ErrString = GetMetaXRErrorString(Result);
      UE_LOG(
          LogAudio,
          Error,
          TEXT("Meta XR Audio SDK Error - %s: %s"),
          TEXT("Failed to create Meta XR Audio context for serialization"),
          ErrString);
      return nullptr;
    } else {
      UE_LOG(LogAudio, Log, TEXT("Meta XR Audio Create Context %p"), SerializationContext);
      TRACE_COUNTER_INCREMENT(COUNTER_METAXRAUDIO_NumContexts);
    }
  }

  return SerializationContext;
}

ovrAudioContext FMetaXRAudioContextManager::GetContextForAudioDevice(const FAudioDevice* InAudioDevice) {
  FScopeLock ScopeLock(&ContextCritSection);
  ovrAudioContext* Context = ContextMap.Find(InAudioDevice);
  if (Context) {
    return *Context;
  } else {
    return nullptr;
  }
}

ovrAudioContext FMetaXRAudioContextManager::GetContext(const AActor* Actor, const UWorld* World) {
  ovrAudioContext PluginContext = FMetaXRAudioLibraryManager::Get().GetPluginContext();
  if (PluginContext != nullptr)
    return PluginContext;

  FMetaXRAudioPlugin* Plugin = &FModuleManager::GetModuleChecked<FMetaXRAudioPlugin>("MetaXRAudio");
  if (Plugin == nullptr) {
    METAXR_AUDIO_LOG_ERROR("Meta XR Audio Plugin is not loaded and therefore cannot get an audio context");
    return nullptr;
  }

  // Note: getting the context from the spatializer, should we get it from the reverb instead?
  const FString MetaXRSpatializerPluginName = Plugin->GetSpatializationPluginFactory()->GetDisplayName();
  const FString CurrentSpatializerPluginName = AudioPluginUtilities::GetDesiredPluginName(EAudioPlugin::SPATIALIZATION);
  if (CurrentSpatializerPluginName.Equals(MetaXRSpatializerPluginName)) // we have a match!
  {
    if (World == nullptr && Actor != nullptr)
      World = Actor->GetWorld();

    if (World == nullptr)
      return nullptr;

    FAudioDevice* AudioDevice = World->GetAudioDevice().GetAudioDevice();
    if (AudioDevice == nullptr) {
      // This happens when cooking for native UE AudioMixer integration
      return FMetaXRAudioContextManager::GetOrCreateSerializationContext(Actor);
    } else {
      MetaXRAudioSpatializationAudioMixer* Spatializer =
          static_cast<MetaXRAudioSpatializationAudioMixer*>(AudioDevice->GetSpatializationPluginInterface().Get());
      if (Spatializer == nullptr) {
        METAXR_AUDIO_LOG_WARNING(
            "Invalid Spatialization Plugin specified, make sure the Spatialization Plugin is set to MetaXRAudio and AudioMixer is enabled!");
        return nullptr;
      }

      PluginContext = FMetaXRAudioContextManager::GetContextForAudioDevice(AudioDevice);
      if (!PluginContext) {
        PluginContext = FMetaXRAudioContextManager::CreateContextForAudioDevice(AudioDevice);
      }
    }
  }

  return PluginContext;
}

ovrAudioContext FMetaXRAudioContextManager::CreateContextForAudioDevice(FAudioDevice* InAudioDevice) {
  const UMetaXRAudioSettings* Settings = GetDefault<UMetaXRAudioSettings>();

  ovrAudioContextConfiguration ContextConfig = {0};
  ContextConfig.acc_BufferLength = InAudioDevice->GetBufferLength();
  ContextConfig.acc_MaxNumSources = Settings->VoiceLimit;
  ContextConfig.acc_SampleRate = InAudioDevice->GetSampleRate();
  ContextConfig.acc_Size = sizeof(ovrAudioContextConfiguration);

  ovrAudioContext NewContext = nullptr;
  ovrResult Result = OVRA_CALL(ovrAudio_CreateContext)(&NewContext, &ContextConfig);

  if (ensure(Result == ovrSuccess)) {
    FScopeLock ScopeLock(&ContextCritSection);
    UE_LOG(LogAudio, Log, TEXT("Meta XR Audio Create Context %p"), NewContext);
    TRACE_COUNTER_INCREMENT(COUNTER_METAXRAUDIO_NumContexts);
    return ContextMap.Add(InAudioDevice, NewContext);
  } else {
    return nullptr;
  }
}

void FMetaXRAudioContextManager::DestroyContextForAudioDevice(const FAudioDevice* InAudioDevice) {
  FScopeLock ScopeLock(&ContextCritSection);
  ovrAudioContext* Context = ContextMap.Find(InAudioDevice);

  if (Context) {
    UE_LOG(LogAudio, Log, TEXT("Meta XR Audio Destroy Context %p"), Context);
    OVRA_CALL(ovrAudio_DestroyContext)(*Context);
    ContextMap.Remove(InAudioDevice);
    TRACE_COUNTER_DECREMENT(COUNTER_METAXRAUDIO_NumContexts);
  }
}
