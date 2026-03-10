// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "Components/ActorComponent.h"
#include "Components/PrimitiveComponent.h"
#include "DebugRenderSceneProxy.h"
#include "DynamicMeshBuilder.h"
#include "HAL/CriticalSection.h"
#include "LandscapeInfo.h"
#include "MetaXR_Audio.h"
#include "MetaXR_Audio_AcousticRayTracing.h"

#include "MetaXRAcousticGeometry.generated.h"

// Fwd declare
class UMetaXRAcousticMaterialProperties;
class UMetaXRAcousticGeometry;
class FAcousticGeoGizmoData;
class FMetaXRAcousticGeometrySceneProxy;

// Custom deleter for FAcousticGeoGizmoData
struct FAcousticGeoGizmoDataDeleter {
  void operator()(const FAcousticGeoGizmoData* ptr) const;
};

/*
 * MetaXRAudio geometry components are used to customize an a static mesh actor's acoustic properties.
 */
typedef uint32_t MetaXRAudioMeshFlags;

UENUM(BlueprintType)
enum class ETraversalMode : uint8 { Default, Actor, SceneComponent };

UCLASS(
    ClassGroup = (Audio),
    HideCategories = (Activation, Collision, Cooking),
    meta =
        (BlueprintSpawnableComponent,
         DisplayName = "Meta XR Acoustic Geometry",
         ToolTip = "Analyze a mesh to generate acoustics, occlusion, and diffraction"))
class METAXRAUDIO_API UMetaXRAcousticGeometry final : public UPrimitiveComponent {
  GENERATED_BODY()

 public:
  UMetaXRAcousticGeometry();
  ~UMetaXRAcousticGeometry();

  // if IncludeChildren is true, children (attached) meshes will be merged
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Acoustics")
  bool bIncludeChildren = true;

  // TraversalType indicates how the Acoustic mesh will be built.
  // The one you should choose depends on the use case. IF you want the
  // acoustic mesh built as a hierarchy of actors (actors with static meshes and acoustic materials that are parented under each other in
  // scene) then you should choose Actor mode. IF this component is part of a Blueprint that is built as a hierarchy of static meshes (under
  // a single actor). then you should choose SceneComponent mode. NOTE: you could have a Blueprint with this component, but you still want
  // to use actor traversal because the static meshes are attached to different actors in the scene.
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Acoustics")
  ETraversalMode TraversalMode = ETraversalMode::Default;

  // Maximum tolerable mesh simplification error in centimeters
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Acoustics")
  float MaxError = 10.0f;

  // Which LOD to use for the acoustic geometry when using an LOD Group. The lowest value of 0 corresponds to the highest quality mesh.
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Acoustics")
  int32 LOD = 0;

  // The path to the serialized mesh file, relative to the project's Content directory
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Acoustics")
  FString FilePath;

  // If enabled the geometry file will be read from disk instead of computing the geometry each time at startup
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Acoustics")
  bool bFileEnabled = true;

  // Flags that indicate how the geometry mesh should be simplified to create an acoustic mesh
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Acoustics")
  int32 MeshFlags = ovrAudioMeshFlags_enableMeshSimplification;

  // Automatically choose Acoustic Materials during baking using each mesh's Physical Material
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Acoustics")
  bool bUsePhysicalMaterials;

  /// \brief A hash code of the game objects contributing to the current serialized acoustic geometry, for detecting changes.
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Acoustics")
  FString HierarchyHash;

#if WITH_EDITOR
  void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) final;
  bool NeedsRebake() const {
    return bNeedsRebake;
  }
  static const bool IsValidAcousticGeoFilePath(const FString& FilePath);
  FPrimitiveSceneProxy* CreateSceneProxy() final;
  FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const final;
#endif
#if WITH_EDITORONLY_DATA
  void OnObjectSelected(UObject* Object);
#endif

  bool UploadGeometry();
  bool StartInternal();
  bool DestroyInternal();
  bool ReadFile();
  bool WriteFile();
  FString ComputeHash() const;
  bool WriteFileInternal(ovrAudioGeometry GeometryHandle);
  bool IncludesChildren() const {
    return bIncludeChildren;
  }
  bool DiffractionEnabled() const {
    return MeshFlags & ovrAudioMeshFlags_enableDiffraction;
  }
  void SetDiffractionEnabled(bool bEnabled) {
    if (bEnabled)
      MeshFlags |= ovrAudioMeshFlags_enableDiffraction;
    else
      MeshFlags &= ~ovrAudioMeshFlags_enableDiffraction;
  }
  bool IsFileEnabled() const {
    return bFileEnabled;
  }
  ovrAudioGeometry GetHandle() const {
    return OvrGeometry;
  }
  FString GetFilePath() const {
    return FilePath;
  }

  // Structures
  struct METAXRAUDIO_API FMeshMaterial {
    const UStaticMeshComponent* StaticMesh = nullptr;
    int32 LOD = 0;
    TArray<UMetaXRAcousticMaterialProperties*> Materials;

    const bool IsInstanced() const;
    const int32 GetInstancedCount() const;
    // Gets the static mesh matrix relative to UMetaXRAcousticGeometry.
    // InstanceID = -1 means NON-instanced mesh matrix. Meaning, if the static mesh is non-instanced call with InstanceID = -1
    // IF the mesh is instanced call with InstanceID >= 0. 0 will get the matrix of the first instance, 1 will get matrix of second
    // instance, etc... False return value means failure to get the matrix (likely because caller used InstanceID >= 0 on NON-instanced mesh
    // OR InstanceID was invalid).
    const bool GetAcousticMeshLocalMatrix(const FMatrix& AcousticGeoCompWorldMatrix, const int32 InstanceID, FMatrix& OutLocalMatrix) const;
  };

  struct METAXRAUDIO_API FLandscapeMaterial {
    ULandscapeInfo* LandscapeInfo = nullptr;
    TArray<UMetaXRAcousticMaterialProperties*> Materials;
  };

  // Define visitor class skeleton and declare the implementations
  class METAXRAUDIO_API ITransformVisitor {
   public:
    ITransformVisitor(const bool bIncludeChildren) : bShouldIncludeChildren(bIncludeChildren) {}
    virtual ~ITransformVisitor() = default;

    virtual TArray<UMetaXRAcousticMaterialProperties*> VisitActor(
        const AActor* CurrentActor,
        const TArray<UMetaXRAcousticMaterialProperties*>* UserData) = 0;
    virtual TArray<UMetaXRAcousticMaterialProperties*> VisitSceneComponent(
        const USceneComponent* CurrentSceneComponent,
        const TArray<UMetaXRAcousticMaterialProperties*>* UserData) = 0;

    void TraverseActorHierarchy(const AActor* Actor);
    void TraverseSceneComponentHierarchy(const USceneComponent* SceneComponent);

   private:
    void TraverseActorHierarchyInternal(
        const AActor* Actor,
        const bool bParentWasExcluded,
        const TArray<UMetaXRAcousticMaterialProperties*>* ParentData = nullptr);

    void TraverseSceneComponentHierarchyInternal(
        const USceneComponent* SceneComponent,
        const bool bParentWasExcluded,
        const TArray<UMetaXRAcousticMaterialProperties*>* ParentData = nullptr);

   private:
    const bool bShouldIncludeChildren;
  };

  class METAXRAUDIO_API FAgeChecker final : public ITransformVisitor {
   public:
    FAgeChecker(const FDateTime& TimeStamp, const bool bUsePhysicalMaterials, const bool bShouldIncludeChildren);

    FString GetHash() const;
    bool IsStale() const;

   private:
    TArray<UMetaXRAcousticMaterialProperties*> VisitActor(
        const AActor* CurrentActor,
        const TArray<UMetaXRAcousticMaterialProperties*>* UserData) final;
    TArray<UMetaXRAcousticMaterialProperties*> VisitSceneComponent(
        const USceneComponent* CurrentSceneComponent,
        const TArray<UMetaXRAcousticMaterialProperties*>* UserData) final;

    bool MarkDirtyIfNeeded(const UObject* Asset);

   private:
    FDateTime TimeStamp;
    bool bUsePhysicalMaterials;
    bool bIsOlder = false;
    FString Hash;
  };

  class METAXRAUDIO_API FHashAppender final : public ITransformVisitor {
   public:
    FHashAppender(const bool bUsePhysicalMaterials, const bool bShouldIncludeChildren);

    FString GetHash() const;

   private:
    TArray<UMetaXRAcousticMaterialProperties*> VisitActor(
        const AActor* CurrentActor,
        const TArray<UMetaXRAcousticMaterialProperties*>* UserData) final;
    TArray<UMetaXRAcousticMaterialProperties*> VisitSceneComponent(
        const USceneComponent* CurrentSceneComponent,
        const TArray<UMetaXRAcousticMaterialProperties*>* UserData) final;

   private:
    FString Hash;
    bool bUsePhysicalMaterials;
  };

  class METAXRAUDIO_API FMeshGatherer final : public ITransformVisitor {
   public:
    FMeshGatherer(const bool IgnoreStatic, const bool UsePhysicalMaterials, const bool bShouldIncludeChildren, const int LODSelection = 0);

    const TArray<UMetaXRAcousticGeometry::FLandscapeMaterial>& GetTerrains() const {
      return Terrains;
    }
    const TArray<UMetaXRAcousticGeometry::FMeshMaterial>& GetMeshes() const {
      return Meshes;
    }
    int GetLODSelection() const {
      return LodSelection;
    }

   private:
    TArray<UMetaXRAcousticMaterialProperties*> VisitActor(
        const AActor* CurrentActor,
        const TArray<UMetaXRAcousticMaterialProperties*>* UserData) final;
    TArray<UMetaXRAcousticMaterialProperties*> VisitSceneComponent(
        const USceneComponent* CurrentSceneComponent,
        const TArray<UMetaXRAcousticMaterialProperties*>* UserData) final;

    void CollectTerrains(const AActor* Actor, const TArray<UMetaXRAcousticMaterialProperties*>& MaterialsToApply);

   private:
    const bool bIgnoreStatic;
    const bool bUsePhysicalMaterials = false;
    const int LodSelection = 0;

    int IgnoredMeshCount = 0;
    TArray<UMetaXRAcousticGeometry::FLandscapeMaterial> Terrains;
    TArray<UMetaXRAcousticGeometry::FMeshMaterial> Meshes;
  };

 private:
  void OnRegister() final;
  void OnUnregister() final;
  void BeginPlay() final;
  void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) final;
  void PostLoad() final;
  void BeginDestroy() final;
  void DestroyComponent(bool bPromoteChildren) final;
  void Activate(bool bReset = false) final;
  void Deactivate() final;

#if WITH_EDITOR
  void InitGizmoData();
  void UpdateGizmoMesh(ovrAudioGeometry GeometryHandle);
  int32 GetGizmoVertexCount() const;
  bool GetGizmoVertexPositions(TArray<FVector>& GizmoVertices) const;
  void GenerateFileNameIfEmpty();
  void RefreshNeedsRebaked();
  void OnComponentCreated() final;
#endif

  bool CreatePropagationGeometry();
  bool DestroyPropagationGeometry();
  void TraverseHierarchy(ITransformVisitor& Visitor) const;
  bool UploadMesh(ovrAudioGeometry GeometryHandle);
  bool UploadMesh(ovrAudioGeometry GeometryHandle, AActor* Owner, bool IgnoreStatic, int& OutIgnoredMeshCount);
  void ApplyTransform();
  void LoadGeometryAsync();
  bool IsStatic() const;
  bool IsPlaymodeActive() const;
  void CheckGeoTransformValid();

  ovrAudioGeometry OvrGeometry;
  ovrAudioContext CachedContext;
  FTransform PreviousTransform;
  ovrAudioGeometry PreviousGeometry;
#if WITH_EDITOR
  mutable FCriticalSection GizmoUpdateCS;
  TUniquePtr<FAcousticGeoGizmoData, FAcousticGeoGizmoDataDeleter> GizmoData = nullptr;
  bool bNeedsRebake;
#endif // WITH_EDITOR

  friend class FMetaXRAcousticGeometryDetails;
  friend class FMetaXRAcousticGeometrySceneProxy;
  friend class UAcousticGeometryTestUtils;
};

#if WITH_EDITOR
class FMetaXRAcousticGeometrySceneProxy final : public FDebugRenderSceneProxy {
 public:
  FMetaXRAcousticGeometrySceneProxy(const UPrimitiveComponent* InComponent);

  FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const final;

  void GetDynamicMeshElements(
      const TArray<const FSceneView*>& Views,
      const FSceneViewFamily& ViewFamily,
      uint32 VisibilityMap,
      FMeshElementCollector& Collector) const final;

  bool CanBeOccluded() const final {
    return false;
  }

 private:
  const UMetaXRAcousticGeometry* GeometryComponent = nullptr;
};
#endif
