// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "ComponentVisualizer.h"
#include "CoreMinimal.h"

/**
 *    Editor visualization of Control Zone box
 */
class METAXRAUDIOEDITOR_API FMetaXRAcousticControlZoneVisualizer final : public FComponentVisualizer {
 public:
  void DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI) final;
};
