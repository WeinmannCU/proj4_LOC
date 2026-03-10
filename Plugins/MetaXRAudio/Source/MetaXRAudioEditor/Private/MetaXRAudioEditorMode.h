// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "BaseBehaviors/KeyAsModifierInputBehavior.h"
#include "BaseGizmos/TransformProxy.h"
#include "DynamicMeshBuilder.h"
#include "EdMode.h"
#include "Editor.h"
#include "MaterialShared.h"
#include "MetaXRAcousticMap.h"

class FMetaXRAudioEditorMode final : public FEdMode {
 public:
  const static FEditorModeID EM_MetaXRAcousticMapEditorModeId;

  void Enter() final;
  void Tick(FEditorViewportClient* ViewportClient, float DeltaTime) final;
  void Exit() final;
  bool InputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event) final;
  void ResetTransformProxy();

 private:
  bool PerformRaycast(FEditorViewportClient* ViewportClient, const FVector2D& MousePosition);

  UPROPERTY(Transient)
  TObjectPtr<UTransformProxy> TransformProxy;
  UPROPERTY(Transient)
  TObjectPtr<UCombinedTransformGizmo> TransformGizmo;
  // Define sphere parameters
  UMetaXRAcousticMap* EditedComponent = nullptr;
};
