// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
// Copyright Epic Games, Inc. All Rights Reserved.

#include "MetaXRAudio.h"
#include "Features/IModularFeatures.h"
#include "IMetaXRAudioPlugin.h"
#include "MetaXRAmbisonicSpatializer.h"
#include "MetaXRAudioReverb.h"

TAudioSpatializationPtr FMetaXRSpatializationPluginFactory::CreateNewSpatializationPlugin(FAudioDevice* OwningDevice) {
  FMetaXRAudioPlugin* Plugin = &FModuleManager::GetModuleChecked<FMetaXRAudioPlugin>("MetaXRAudio");
  if (Plugin != nullptr) {
    Plugin->RegisterAudioDevice(OwningDevice);
  } else {
    UE_LOG(LogAudio, Error, TEXT("MetaXRAudio Plugin is not loaded and therefore cannot create a new spatialization plugin"));
  }

  return TAudioSpatializationPtr(new MetaXRAudioSpatializationAudioMixer());
}

TAudioReverbPtr FMetaXRReverbPluginFactory::CreateNewReverbPlugin(FAudioDevice* OwningDevice) {
  FMetaXRAudioPlugin* Plugin = &FModuleManager::GetModuleChecked<FMetaXRAudioPlugin>("MetaXRAudio");
  if (Plugin != nullptr) {
    Plugin->RegisterAudioDevice(OwningDevice);
  } else {
    UE_LOG(LogAudio, Error, TEXT("MetaXRAudio Plugin is not loaded and therefore cannot create a new reverb plugin"));
  }

  return TAudioReverbPtr(new MetaXRAudioReverb());
}
