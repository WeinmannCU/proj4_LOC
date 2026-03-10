// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
#pragma once

#include "Async/Async.h"
#include "Async/AsyncWork.h"
#include "Components/ActorComponent.h"
#include "Components/PrimitiveComponent.h"
#include "DebugRenderSceneProxy.h"
#include "MetaXR_Audio.h"
#include "MetaXR_Audio_AcousticRayTracing.h"

#include "MetaXRAcousticMap.generated.h"

// Fwd declare
class UMetaXRAcousticMaterial;
class UMetaXRAcousticGeometry;

/*
 * MetaXRAudio geometry components are used to customize an a static mesh actor's acoustic properties.
 */
UENUM(BlueprintType)
enum class EAcousticMapStatus : uint8 {
  Empty = 0 UMETA(DisplayName = "Empty"),
  Mapped = (1 << 0) UMETA(DisplayName = "Mapped"),
  Ready = (1 << 1) | Mapped UMETA(DisplayName = "Ready"),
};

class FAsyncSceneMappingTask : public FNonAbandonableTask {
 public:
  UMetaXRAcousticMap* AcousticMapComponent;
  ovrAudioSceneIR Map;
  ovrAudioSceneIRParameters Parameters;
  bool bMapOnly = false;

  FAsyncSceneMappingTask(UMetaXRAcousticMap* InMapComponent, ovrAudioSceneIR InMap, ovrAudioSceneIRParameters InParameters)
      : AcousticMapComponent(InMapComponent), Map(InMap), Parameters(InParameters) {}

  FAsyncSceneMappingTask(FAsyncSceneMappingTask& Other)
      : AcousticMapComponent(Other.AcousticMapComponent), Map(Other.Map), Parameters(Other.Parameters) {}

  void DoWork();

  FORCEINLINE TStatId GetStatId() const {
    RETURN_QUICK_DECLARE_CYCLE_STAT(FAsyncSceneMappingTask, STATGROUP_ThreadPoolAsyncTasks);
  }
};

UCLASS(
    ClassGroup = (Audio),
    HideCategories = (Activation, Collision, Cooking),
    meta =
        (BlueprintSpawnableComponent,
         DisplayName = "Meta XR Acoustic Map",
         ToolTip = "Precompute information about the acoustics into an Acoustic Map to reduce resource usage"))
class METAXRAUDIO_API UMetaXRAcousticMap final : public UPrimitiveComponent {
  GENERATED_BODY()

 public:
  UMetaXRAcousticMap();

  // The path to the serialized acoustic map, relative to the project's Content directory
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Acoustics")
  FString FilePath;

  // Only bake data for game objects marked as static when checked
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Acoustics")
  bool bStaticOnly = false;

  // Disables the creation of map data points for areas far above the environment's floor
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Acoustics")
  bool bNoFloating = true;

  // Precompute edge diffraction data for smooth occlusion. If disabled, a lower-quality but faster fallback diffraction will be used.
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Acoustics")
  bool bDiffraction = true;

  // The size in centimeters of the smallest space that will be precomputed
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Acoustics")
  float MinSpacing = 100.0f;

  // The maximum distance in centimeters between precomputed data points
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Acoustics")
  float MaxSpacing = 1000.0f;

  // The distance above the floor in centimeters where data points are placed
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Acoustics")
  float HeadHeight = 150.0f;

  // The maximum height in centimeters above the floor where data points are placed
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Acoustics")
  float MaxHeight = 300.0f;

  // \brief The gravity vector indicates the direction which would match the direction of gravity within the game
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Acoustics")
  FVector GravityVector = FVector(0, 0, -1.0f); // Default gravity vector

  // The number of reflections generated for each point in the acoustic map
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Acoustics")
  int32 ReflectionCount = 6;

  // Indicates the user has selected to allow the use of manually placed points
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Acoustics")
  bool bCustomPointsEnabled = false;

  // Indicates that user has added at least one manually placed point
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Acoustics")
  bool bHasCustomPoints = false;

  FString GetFilePath() const {
    return FilePath;
  }
  void LoadData();
  void StartInternal(bool AutoLoad = true);
  void DestroyInternal();

#if WITH_EDITOR
  bool Compute(bool bMapOnly, bool bBlockingCompute = false);
  void FinishCompute();
  void CancelCompute();
  FVector GetNewPointForRay(const FVector& EditorCameraPosition, const FVector& EditorCameraDirection) const;
  void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) final;
  bool IsComputeCanceled() const {
    return bComputeCanceled;
  }
  void SetDescription(FString NewDescription) {
    Description = NewDescription;
  }
  void SetComputeProgress(float NewComputeProgress) {
    ComputeProgress = NewComputeProgress;
  }
  void SetComputeTimeSeconds(float NewComputeTime) {
    ComputeTime = NewComputeTime;
  }
  FString GetDescription() const {
    return Description;
  }
  float GetComputeProgress() const {
    return ComputeProgress;
  }
  float GetComputeTimeSeconds() const {
    return ComputeTime;
  }
  void StartTimer() {
    StartTimeSeconds = FPlatformTime::Seconds();
  }
  double CheckTimer() const {
    return FPlatformTime::Seconds() - StartTimeSeconds;
  }
  void SetStageStartingTimeSeconds(double NewTime) {
    StageStartingTimeSeconds = NewTime;
  }
  double GetStageStartingTimeSeconds() const {
    return StageStartingTimeSeconds;
  }
  FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const final;
#endif

 private:
  void OnRegister() final;
  void OnUnregister() final;
  void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) final;
  void BeginDestroy() final;
  FPrimitiveSceneProxy* CreateSceneProxy() final;
  void CheckMapTransformValid();
  void ApplyTransform();
  bool IsPlaymodeActive() const;
  void UpdateCachedPoints();

  ovrAudioSceneIR CachedMap = nullptr;
  ovrAudioSceneIRParameters MapParameters;
  FTransform PreviousTransform;

#if WITH_EDITOR
  void PostEditComponentMove(bool bFinished) final;
  void Activate(bool bReset = false) final;
  void Deactivate() final;
  void CheckIfOnlyMapInLevel();
  void GenerateFileNameIfEmpty();
  void GatherGeometriesAndMaterials();
  void OnCVarStateChanged(IConsoleVariable* CVar);

#pragma region GIZMO
  int GetGizmoPointCount() const;
  bool IsGizmoPointSelected() const;
  bool HasGizmoPoints() const;
  FVector GetGizmoPointUE(int PointIndex) const;
  TArray<FVector> GetGizmoPointsUE() const;
  FVector GetSelectedGizmoPointUE() const;
  float GetGizmoSphereRadiusWS() const;
  void AddGizmoPoint(const FVector& NewPointUE);
  void RemoveGizmoPoint(const int PointIndex);
  void RemoveSelectedGizmoPoint();
  void MoveSelectedGizmoPoint(const FVector& NewLocationUE);
  void SetGizmoPoints(TArray<float>&& NewPointsOVR, const int NumPoints);
  void SetGizmoSelectedPoint(const int SelectedPoint);
  void ResetGizmoSelectedPoint();
  void SetupGizmoMeshComponent();
#pragma endregion

  FString Hash;
  bool bComputing = false;
  bool bComputeFinished = false;
  bool bComputeCanceled = false;
  bool bComputeSucceeded = false;
  FString Description;
  float ComputeProgress;
  float ComputeTime;
  double StartTimeSeconds;
  double StageStartingTimeSeconds;
  EAcousticMapStatus Status;
  TSharedPtr<FAsyncTask<FAsyncSceneMappingTask>> MappingTask;
  TArray<UMetaXRAcousticGeometry*> Geometries;
  TArray<UMetaXRAcousticMaterial*> Materials;

  // Stores all the information about the acoustic map points in OVR coordinate system
  TArray<FVector> PointsOVR;
  int32 SelectedPointIndex = -1;
  class UMaterialInstanceDynamic* GizmoMaterialPtr;
  FDelegateHandle AcousticMapCVarDH, GlobalMetaCVarDH;
#endif
#if WITH_EDITORONLY_DATA
  UPROPERTY(Transient)
  TObjectPtr<class UInstancedStaticMeshComponent> GizmoMeshComponent;
#endif

  friend class FMetaXRAcousticMapDetails;
  friend class FAsyncSceneMappingTask;
  friend class FMetaXRAudioEditorMode;
  friend class UMetaXRAcousticMapTestUtils;
};
