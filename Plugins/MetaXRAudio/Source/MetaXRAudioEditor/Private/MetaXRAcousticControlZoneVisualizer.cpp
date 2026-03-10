// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
#include "MetaXRAcousticControlZoneVisualizer.h"

#include "EditorModes.h"
#include "MetaXRAcousticControlZone.h"
#include "MetaXRAudioLogging.h"

void DrawWireOutline(FPrimitiveDrawInterface* PDI, const UMetaXRAcousticControlZoneWrapper* ControlZone) {
  FVector NativeBoxSize, _;
  ControlZone->GetNativeSizes(NativeBoxSize, _);
  const FMatrix M = ControlZone->GetWorldMatrix();
  const FVector InnerExtents(FVector3f::One() * 100.0f);
  const FVector OuterExtents(NativeBoxSize / 2);

  const FLinearColor& VizColor = ControlZone->GetVisualizerColor();

  // Draw the wireboxes
  FVector WirePoints[2][8];
  FBox B = FBox::BuildAABB(FVector::Zero(), NativeBoxSize / 2);
  DrawWireBox(PDI, M, B, VizColor, SDPG_World, 2.0f);
  B.GetVertices(WirePoints[0]);

  B = FBox::BuildAABB(FVector::Zero(), InnerExtents);
  DrawWireBox(PDI, M, B, FColor::White, SDPG_World, 2.0f);
  B.GetVertices(WirePoints[1]);

  // Draw the lines connecting the corners to give a bit more perspective when camera in box...
  for (uint32 v = 0; v < 8; v++) {
    const FVector V0 = M.TransformPosition(WirePoints[0][v]);
    const FVector V1 = M.TransformPosition(WirePoints[1][v]);
    PDI->DrawLine(V0, V1, FColor::White, SDPG_World, 2.0f);
  }
}

void DrawGizmo(FPrimitiveDrawInterface* PDI, UMetaXRAcousticControlZoneWrapper* ControlZone) {
  FVector NativeBoxSize, _;
  ControlZone->GetNativeSizes(NativeBoxSize, _);
  const FMatrix BoxToWorld = ControlZone->GetWorldMatrix();

  const FMaterialRenderProxy* MaterialRenderProxy = ControlZone->GetVisualizerProxy();

  DrawBox(PDI, BoxToWorld, NativeBoxSize / 2.0, MaterialRenderProxy, SDPG_World);

  // Inner box (native inner size is 200, so radii is 100)
  const FVector3f InnerRadii3f = FVector3f::OneVector * 100.0f;
  const FVector InnerRadii(InnerRadii3f);
  DrawBox(PDI, BoxToWorld, InnerRadii, MaterialRenderProxy, SDPG_World);
}

void FMetaXRAcousticControlZoneVisualizer::DrawVisualization(
    const UActorComponent* Component,
    const FSceneView* View,
    FPrimitiveDrawInterface* PDI) {
  static const IConsoleVariable* const GlobalCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("MetaXRAudioGizmos"));
  if (GlobalCVar == nullptr) {
    METAXR_AUDIO_LOG_ERROR("Could not find MetaXRAudioGizmos CVar, gizmos will not be drawn.");
    return;
  }
  static const IConsoleVariable* const ControlZoneCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("MetaXRAudioGizmos.ControlZones"));
  if (ControlZoneCVar == nullptr) {
    METAXR_AUDIO_LOG_ERROR("Could not find MetaXRAudioGizmos.ControlZones CVar, gizmos will not be drawn.");
    return;
  }

  if (Component == nullptr || PDI == nullptr || GlobalCVar->GetInt() == 0 || ControlZoneCVar->GetInt() == 0)
    return;

  const UMetaXRAcousticControlZoneWrapper* ControlZone = Cast<const UMetaXRAcousticControlZoneWrapper>(Component);
  if (!ControlZone)
    return;

  DrawGizmo(PDI, const_cast<UMetaXRAcousticControlZoneWrapper*>(ControlZone));
  DrawWireOutline(PDI, ControlZone);
}
