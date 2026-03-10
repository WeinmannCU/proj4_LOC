// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "MetaXR_Audio.h"
// Dll Manager included directly here as most code that uses the context will use OVRA_CALL
// We should probably move macros into this file (OVRA_CALL, ...)
#include "MetaXRAudioDllManager.h"

bool GetOVRAContext(ovrAudioContext& Context);
bool GetOVRAContext(ovrAudioContext& Context, const class AActor* Actor);
bool GetOVRAContext(ovrAudioContext& Context, const class AActor* Actor, const class UWorld* WorldPtr);
