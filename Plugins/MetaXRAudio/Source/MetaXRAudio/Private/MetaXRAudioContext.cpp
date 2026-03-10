// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
#include "MetaXRAudioContext.h"
#include "MetaXRAudioPlatform.h"
#ifdef META_NATIVE_UNREAL_PLUGIN
#include "MetaXRAudioContextManager.h"
#endif

bool GetOVRAContext(ovrAudioContext& Context) {
  Context = nullptr;
#ifdef META_NATIVE_UNREAL_PLUGIN
  Context = FMetaXRAudioContextManager::GetContext(nullptr, nullptr);
#else
  Context = FMetaXRAudioLibraryManager::Get().GetPluginContext();
#endif
  return Context != nullptr;
}

bool GetOVRAContext(ovrAudioContext& Context, const AActor* Actor) {
  Context = nullptr;
#ifdef META_NATIVE_UNREAL_PLUGIN
  if (Actor != nullptr)
    Context = FMetaXRAudioContextManager::GetContext(Actor, Actor->GetWorld());
  else
    Context = FMetaXRAudioContextManager::GetContext(nullptr, nullptr);
#else
  Context = FMetaXRAudioLibraryManager::Get().GetPluginContext();
#endif
  return Context != nullptr;
}

bool GetOVRAContext(ovrAudioContext& Context, const AActor* Actor, const UWorld* WorldPtr) {
  Context = nullptr;
#ifdef META_NATIVE_UNREAL_PLUGIN
  Context = FMetaXRAudioContextManager::GetContext(Actor, WorldPtr);
#else
  Context = FMetaXRAudioLibraryManager::Get().GetPluginContext();
#endif
  return Context != nullptr;
}
