// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
// Copyright Epic Games, Inc. All Rights Reserved.
#include "MetaXRAcousticGeometry.h"
#include "AudioDevice.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "IMetaXRAudioPlugin.h"
#include "Kismet/KismetMathLibrary.h"
#include "Landscape.h"
#include "LandscapeComponent.h"
#include "LandscapeDataAccess.h"
#include "LandscapeInfo.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MetaXRAcousticMaterial.h"
#include "MetaXRAcousticProjectSettings.h"
#include "MetaXRAudioContext.h"
#include "MetaXRAudioUtilities.h"
#include "Misc/FileHelper.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Runtime/Core/Public/Serialization/CustomVersion.h"
#if WITH_EDITORONLY_DATA
#include "Selection.h"
#endif
#include "Misc/EngineVersionComparison.h"
#include "StaticMeshResources.h"
#if UE_VERSION_OLDER_THAN(5, 2, 0)
#include "MaterialShared.h"
#else
#include "Materials/MaterialRenderProxy.h"
#endif
#include "MetaXRAudioLogging.h"
#if WITH_EDITOR
#include <regex>
#endif

#define UE_ACOUSTIC_GEOMETRY_COMPONENT_LATEST_VERSION 1
#define WITH_EDITOR_GIZMOS (WITH_EDITOR && WITH_EDITORONLY_DATA)

using AcousticMesh = UMetaXRAcousticGeometry::FMeshMaterial;

static const TArray<UMetaXRAcousticMaterialProperties*> EmptyAcousticMaterialProps;
static const FGuid UMetaXRAcousticGeometryGUID(0x7aa25489, 0x40d74392, 0xad87d336, 0x347cfae2); // random GUID, guaranteed to be random.
static const FCustomVersionRegistration UMetaXRAcousticGeometryGUIDRegistration(
    UMetaXRAcousticGeometryGUID,
    UE_ACOUSTIC_GEOMETRY_COMPONENT_LATEST_VERSION,
    TEXT("MetaXRAcousticGeometryVersion"));

// Forward declare hidden function
ovrResult ovrAudio_AudioGeometrySetObjectFlag(ovrAudioGeometry geometry, ovrAudioObjectFlags flag, int32_t enabled);

#if WITH_EDITOR
#define UE_ACOUSTIC_GEO_FILE_EXTENSION ".xrageo"

class FAcousticGeoGizmoData {
 public:
  void UpdateGizmoMeshData(ovrAudioGeometry GeometryHandle);
  void MapGizmoMaterials(UMetaXRAcousticGeometry::FMeshGatherer& Gatherer);

  const TArray<FDynamicMeshVertex>& GetGizmoVertexData() const;
  const TMap<UMetaXRAcousticMaterialProperties*, TArray<uint32>>& GetGizmoMaterialData() const;
  const TArray<UMetaXRAcousticMaterialProperties*>& GetGizmoMaterialMapping() const;

 private:
  TArray<FDynamicMeshVertex> GizmoVertices;
  TMap<UMetaXRAcousticMaterialProperties*, TArray<uint32>> GizmoMaterialIndices;
  TArray<UMetaXRAcousticMaterialProperties*> GizmoMaterialMapping;
};

const bool UMetaXRAcousticGeometry::IsValidAcousticGeoFilePath(const FString& FilePath) {
  if (FilePath.IsEmpty())
    return true;

  const std::regex pattern(R"((MetaXRAcoustics\/)[^\\:*?"<>| \r\n]*(\.xrageo)$)");
  const std::string stdString(TCHAR_TO_UTF8(*FilePath));
  return std::regex_match(stdString, pattern);
}

// Recursive function to build a filename representing the object hierarchy relative to the Actor owning the Geometry component
FString GenerateFileName(AActor* CurrentActor) {
  AActor* ParentActor = CurrentActor->GetAttachParentActor();

  // If there is no actor nested above this one, no need to recurse further
  if (ParentActor == nullptr) {
    FString LevelName = MetaXRAudioUtilities::GetActorLevelName(CurrentActor);
    if (!LevelName.IsEmpty()) {
      MetaXRAudioUtilities::CreateMetaXRAcousticContentDirectory(META_XR_AUDIO_DEFAULT_SAVE_FOLDER + FString("/") + LevelName);
    }
    return FPaths::Combine(META_XR_AUDIO_DEFAULT_SAVE_FOLDER, LevelName, "/", CurrentActor->GetActorNameOrLabel());
  }

  return GenerateFileName(ParentActor) + "-" + CurrentActor->GetActorNameOrLabel();
}

void UMetaXRAcousticGeometry::GenerateFileNameIfEmpty() {
  AActor* ActorOwner = GetOwner();

  if (!FilePath.IsEmpty() || ActorOwner == nullptr)
    return;

  if (MetaXRAudioUtilities::IsBlueprintInstance(ActorOwner)) {
    MetaXRAudioUtilities::CreateMetaXRAcousticContentDirectory(META_XR_AUDIO_DEFAULT_SHARED_FOLDER);
    FString BlueprintName;
    if (!MetaXRAudioUtilities::GetBlueprintAssetName(ActorOwner, BlueprintName))
      return;

    const FString NewFilePath = FString(META_XR_AUDIO_DEFAULT_SHARED_FOLDER) / BlueprintName + UE_ACOUSTIC_GEO_FILE_EXTENSION;
    const TFunction<void(UMetaXRAcousticGeometry*)> Modifier = [&NewFilePath](UMetaXRAcousticGeometry* DefaultBPComponent) {
      DefaultBPComponent->FilePath = NewFilePath;
    };

    if (MetaXRAudioUtilities::ModifyBlueprintAssetComponent<UMetaXRAcousticGeometry>(ActorOwner, Modifier))
      FilePath = NewFilePath;

    return;
  }

  MetaXRAudioUtilities::CreateMetaXRAcousticContentDirectory(META_XR_AUDIO_DEFAULT_SAVE_FOLDER);
  FilePath = GenerateFileName(ActorOwner);
  // Use end modifier to ensure file name collisions are avoided
  FString Suggestion;
  FString Modifier;
  FString FullFilePath;
  int Counter = 0;
  do {
    Suggestion = FilePath + Modifier + UE_ACOUSTIC_GEO_FILE_EXTENSION;
    Modifier = FString("-") + FString::FromInt(++Counter);

    // sanity check to prevent hang
    if (Counter > 10000) {
      METAXR_AUDIO_LOG_ERROR("Unable to find suitable file name for %s", *ActorOwner->GetActorLabel());
      return;
    }

    FullFilePath = FPaths::ProjectContentDir() / Suggestion;
  } while (FPaths::FileExists(*FullFilePath));
  FilePath = Suggestion;
  METAXR_AUDIO_LOG("No file path specified, using the autogenerated name of: %s", *FilePath);
}

void UMetaXRAcousticGeometry::RefreshNeedsRebaked() {
  // Determine if the geometry needs rebaking based on time stamp.
  FDateTime TimeStamp = FDateTime::MinValue();
  int64 FileSize = 0;
  FFileStatData FileStat;
  if (MetaXRAudioUtilities::GetFileStatData(FilePath, FileStat)) {
    TimeStamp = FileStat.ModificationTime;
    FileSize = FileStat.FileSize;
  }

  const bool bIsInvalid = (GetGizmoVertexCount() == 0) && (FileSize <= 0);
  if (bIsInvalid) {
    bNeedsRebake = true;
    return;
  }

  // The AgeChecker traversal can take a long time. Call it just once when GUI is enabled.
  FAgeChecker AgeChecker(TimeStamp, bUsePhysicalMaterials, bIncludeChildren);
  TraverseHierarchy(AgeChecker);
  bNeedsRebake = AgeChecker.IsStale() || AgeChecker.GetHash() != HierarchyHash;
}
#endif

UMetaXRAcousticGeometry::UMetaXRAcousticGeometry() : OvrGeometry(nullptr), CachedContext(nullptr), PreviousGeometry(nullptr) {
  bAutoActivate = true;
  PrimaryComponentTick.bCanEverTick = true;
  bWantsInitializeComponent = true;
  bTickInEditor = true;

#if WITH_EDITORONLY_DATA
  USelection::SelectObjectEvent.AddUObject(this, &UMetaXRAcousticGeometry::OnObjectSelected); // Click on actor in viewport
  USelection::SelectionChangedEvent.AddUObject(this, &UMetaXRAcousticGeometry::OnObjectSelected); // Click on actor in details pane
#endif

#if WITH_EDITOR
  bNeedsRebake = true;
  bUsePhysicalMaterials = false;
#endif
}

UMetaXRAcousticGeometry::~UMetaXRAcousticGeometry() = default;

#if WITH_EDITORONLY_DATA
void UMetaXRAcousticGeometry::OnObjectSelected(UObject* Object) {
  // Note this callback can be called for any object so we must check it was the Geometry component selected
  // When selected in the viewport, the UObject is the AActor owning the component selected
  if (Object == GetOwner()) {
    CheckGeoTransformValid();
    RefreshNeedsRebaked();
  } else { // When selected in the detailed pane, the UObject is a USelection struct
    // Get all the actors from the selection
    USelection* Selection = Cast<USelection>(Object);
    TArray<AActor*> AllSelectedActors;
    if (Selection != nullptr) {
      Selection->GetSelectedObjects<AActor>(AllSelectedActors);
    }

    // See if the Geometry's actor was part of the selection
    for (AActor* SelectedActor : AllSelectedActors) {
      if (SelectedActor == GetOwner()) {
        RefreshNeedsRebaked();
      }
    }
  }
}
#endif

// We consider the acoustic geo static if
// all the scene components from this components parent to root are static (not including this component unless its root).
// See: CheckGeoTransformValid
bool UMetaXRAcousticGeometry::IsStatic() const {
  USceneComponent* SC = this->GetAttachParent();
  // we must be the root component.
  if (SC == nullptr)
    return this->Mobility != EComponentMobility::Movable;

  while (SC) {
    if (SC->Mobility == EComponentMobility::Movable)
      return false;
    SC = SC->GetAttachParent();
  }
  return true;
}

bool UMetaXRAcousticGeometry::IsPlaymodeActive() const {
  return MetaXRAudioUtilities::PlayModeActive(GetWorld());
}

// We do not allow UGeometry transformations as we want our geometry to stay in sync with the Static Mesh
void UMetaXRAcousticGeometry::CheckGeoTransformValid() {
  const AActor* Owner = GetOwner();
  if (Owner == nullptr)
    return;

  if (!Owner->GetTransform().Equals(GetComponentTransform())) {
    ResetRelativeTransform();
    METAXR_AUDIO_LOG_WARNING(
        "You cannot change the transform of the UGeometry, only the transform of your static mesh or landscape. This ensures the audio geometry stays in sync with the physical geometry.");
  }
}

FString UMetaXRAcousticGeometry::ComputeHash() const {
  FHashAppender HashAppender(bUsePhysicalMaterials, bIncludeChildren);
  TraverseHierarchy(HashAppender);
  return HashAppender.GetHash();
}

void UMetaXRAcousticGeometry::OnRegister() {
  Super::OnRegister();

#if WITH_EDITOR
  GenerateFileNameIfEmpty();
#endif
  CheckGeoTransformValid();
}

void UMetaXRAcousticGeometry::OnUnregister() {
  Super::OnUnregister();
  DestroyInternal();
}

void UMetaXRAcousticGeometry::BeginPlay() {
  Super::BeginPlay();

  if (!StartInternal()) {
    METAXR_AUDIO_LOG_ERROR("Failed to initialize Acoustic Geometry. Destroying this component...");
    DestroyComponent(true);
    return;
  }

#if WITH_EDITOR
  SetComponentTickEnabled(true);
  // For visualizing acoustic geo in editor + playmode
  InitGizmoData();
#else
  // Tick is only used for Moveable acoustic geo or if we are in editor (visualization gizmo)
  const bool IsMovable = !IsStatic();
  SetComponentTickEnabled(IsMovable);
#endif
}

bool UMetaXRAcousticGeometry::StartInternal() {
  if (!CreatePropagationGeometry()) {
    METAXR_AUDIO_LOG_WARNING("Failed to setup geometry component");
    return false;
  }
  ApplyTransform();
  return true;
}

bool UMetaXRAcousticGeometry::CreatePropagationGeometry() {
  if (!GetOVRAContext(CachedContext, GetOwner(), GetWorld())) {
    METAXR_AUDIO_LOG_WARNING(
        "Please check that the Spatialization Plugin and Reverb Plugin are set to Meta XR Audio in project settings \
        or Meta XR Audio binaries have been placed correctly.");
    return false;
  }

  if (OvrGeometry != nullptr) {
    METAXR_AUDIO_LOG_WARNING("Tried to initialize geometry twice, destroying stale copy");
    DestroyPropagationGeometry();
  }

  if (OvrGeometry != nullptr) {
    METAXR_AUDIO_LOG_WARNING("Unable to clean up stale geometry");
    return false;
  }

  // our internal geometry object into which we'll merge this static mesh actor and all its attached meshes
  ovrResult Result = OVRA_CALL(ovrAudio_CreateAudioGeometry)(CachedContext, &OvrGeometry);
  if (Result != ovrSuccess) {
    METAXR_AUDIO_LOG_WARNING("Failed creating acoustic geometry object.");
    return false;
  } else {
    METAXR_AUDIO_LOG("Created new geometry handle %p", OvrGeometry);
  }

#if WITH_EDITOR
  if (!IsPlaymodeActive()) {
    // If this is startup during project startup, try loading an existing file before uploading because we don't have access to child meshes
    // at Startup time
    bool FileReadSuccessfully = false;
    if (bFileEnabled)
      FileReadSuccessfully = ReadFile();

    if (!FileReadSuccessfully && !UploadMesh(OvrGeometry))
      return false;
  } else
#endif
      if (bFileEnabled) {
    if (!ReadFile()) {
      return false;
    }
  } else {
    if (IsStatic()) {
      METAXR_AUDIO_LOG_WARNING("Static geometry requires \"File Enabled\"");
      return false;
    } else if (!UploadGeometry()) {
      return false;
    }
  }

  return true;
}

// ------------------------------------------------------------------------------------------------------------------------------------------------------------
// Note: a child (attached) static mesh will have it's geometry merged into the parent. Therefore, its ovrGeometry data member will be
// nullptr below and this is why we need the Uploaded member.
void UMetaXRAcousticGeometry::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

#if WITH_EDITOR
  // This will update the visualizer on startup but keep the geo handle empty for editor
  // This is needed because OnRegister does not yet know about the children of an actor
  if (!IsPlaymodeActive() && !GizmoData) {
    StartInternal();
    InitGizmoData();
    DestroyInternal();
  }
#endif

  // Update the transform when changed during runtime
  if (OvrGeometry != nullptr) {
    const FTransform& Transform = GetComponentTransform();
    const bool NeedsApplyTransform = !Transform.Equals(PreviousTransform) || (PreviousGeometry != OvrGeometry);
    if (NeedsApplyTransform)
      ApplyTransform();

#if WITH_EDITOR_GIZMOS
    if (NeedsApplyTransform && !IsPlaymodeActive())
      UpdateGizmoMesh(OvrGeometry); // we only update Gizmo mesh as we only allow changes to transform at runtime.
#endif
  }
}

#pragma region MESH_UPLOAD
// ------------------------------------------------------------------------------------------------------------------------------------------------------------
bool UMetaXRAcousticGeometry::UploadGeometry() {
  int IgnoredMeshCount = 0;
  if (!UploadMesh(OvrGeometry, GetOwner(), IsPlaymodeActive(), IgnoredMeshCount))
    return false;

  if (IgnoredMeshCount != 0) {
    METAXR_AUDIO_LOG_WARNING(
        "Failed to upload meshes, %i static meshes ignored. Turn on \"File Enabled\" to process static meshes offline", IgnoredMeshCount);
  }

#if WITH_EDITOR
  UpdateGizmoMesh(OvrGeometry);
#endif
  return true;
}

static void UpdateCountsForMesh(
    int32& TotalVertexCount,
    uint32& TotalIndexCount,
    int32& TotalFaceCount,
    int32& TotalMaterialCount,
    const AcousticMesh& AcousticMesh) {
  if (!AcousticMesh.StaticMesh)
    return;

  UStaticMesh* Mesh = AcousticMesh.StaticMesh->GetStaticMesh();
  if (!Mesh)
    return;

  int32 LodGroupToUse = FMath::Clamp(AcousticMesh.LOD, 0, Mesh->GetRenderData()->LODResources.Num() - 1);
  FStaticMeshLODResources& Model = Mesh->GetRenderData()->LODResources[LodGroupToUse];
  const FPositionVertexBuffer& VertexBuffer = Model.VertexBuffers.PositionVertexBuffer;

  // To account for instanced meshes, treat each instance as if it is an individual UStaticMeshComponent
  const int32 InstanceCount = AcousticMesh.IsInstanced() ? AcousticMesh.GetInstancedCount() : 1;
  TotalVertexCount += InstanceCount * VertexBuffer.GetNumVertices();
  TotalIndexCount += InstanceCount * Model.IndexBuffer.GetNumIndices();
  TotalFaceCount += InstanceCount * Model.IndexBuffer.GetNumIndices() / 3;
  TotalMaterialCount += InstanceCount * Mesh->GetNumSections(LodGroupToUse);
}

#if WITH_EDITOR
static void UpdateCountsForLandscape(
    int32& TotalVertexCount,
    uint32& TotalIndexCount,
    int32& TotalFaceCount,
    int32& TotalMaterialCount,
    ULandscapeInfo* LandscapeInfo) {
  if (!LandscapeInfo)
    return;

  for (auto It = LandscapeInfo->XYtoComponentMap.CreateIterator(); It; ++It) {
    ULandscapeComponent* Component = It.Value();
    const int32 Length = Component->ComponentSizeQuads + 1;
    TotalVertexCount += (Length * Length);
    TotalIndexCount += 4 * (Length - 1) * (Length - 1);
  }

  TotalMaterialCount += 1;
}
#endif

static bool UploadMeshFilter(
    ovrAudioContext Context,
    TArray<ovrAudioMeshGroup>& MeshGroups,
    TArray<FVector>& Vertices,
    TArray<uint32>& Indices,
    int32& VertexOffset,
    int32& IndexOffset,
    int32& GroupOffset,
    const UStaticMesh* Mesh,
    const int32 LOD,
    const TArray<UMetaXRAcousticMaterialProperties*>& Materials,
    const FMatrix Matrix) {
  // Lowest LOD
  int32 LodGroupToUse = FMath::Clamp(LOD, 0, Mesh->GetRenderData()->LODResources.Num() - 1);
  const FStaticMeshLODResources& Model = Mesh->GetRenderData()->LODResources[LodGroupToUse];
  const FPositionVertexBuffer& VertexBuffer = Model.VertexBuffers.PositionVertexBuffer;
  const int32 VertexCount = VertexBuffer.GetNumVertices();

  // append this mesh's vertices to the merged array after coordinate system conversion
  for (int32 Index = 0; Index < VertexCount; Index++) {
    int32 Offset = (VertexOffset + Index);
    FVector TransformedVertex = UKismetMathLibrary::Matrix_TransformPosition(Matrix, (FVector)VertexBuffer.VertexPosition(Index));
    Vertices[Offset] = MetaXRAudioUtilities::ToOVRVector(TransformedVertex);
  }
  const int32 IndexCount = Model.IndexBuffer.GetNumIndices();
  for (int32 Index = 0; Index < IndexCount; Index++) {
    Indices[IndexOffset + Index] = Model.IndexBuffer.GetIndex(Index) + VertexOffset;
  }

  // loop over each section in lowest LOD
  for (int32 Index = 0; Index < Model.Sections.Num(); Index++) {
    ovrAudioMeshGroup& MeshGroup = MeshGroups[GroupOffset + Index];
    MeshGroup.faceCount = Model.Sections[Index].NumTriangles;
    MeshGroup.faceType = ovrAudioFaceType_Triangles;
    MeshGroup.indexOffset = Model.Sections[Index].FirstIndex + IndexOffset;

    ovrAudioMaterial OvrMaterial = nullptr;
    if (!Materials.IsEmpty()) {
      ovrResult Result = OVRA_CALL(ovrAudio_CreateAudioMaterial)(Context, &OvrMaterial);
      if (Result != ovrSuccess) {
        return false;
      }

      int MatIndex = Index;
      if (MatIndex >= Materials.Num())
        MatIndex = Materials.Num() - 1;

      if (Materials[MatIndex]) {
        Materials[MatIndex]->ConstructMaterial(OvrMaterial);
      }
    }
    MeshGroup.material = OvrMaterial;
  }

  // Update the offsets for the next meshs
  VertexOffset += VertexCount;
  IndexOffset += IndexCount;
  GroupOffset += Model.Sections.Num();

  return true;
}

// Duplicate the mesh information for each Instanced Static Mesh
static bool UploadInstancedMeshFilter(
    ovrAudioContext Context,
    TArray<ovrAudioMeshGroup>& MeshGroups,
    TArray<FVector>& Vertices,
    TArray<uint32>& Indices,
    int32& VertexOffset,
    int32& IndexOffset,
    int32& GroupOffset,
    const AcousticMesh& AcousticMesh,
    const FMatrix& AcousticGeoCompWM) {
  const int32 InstanceCount = AcousticMesh.GetInstancedCount();
  const TObjectPtr<UStaticMesh> StaticMeshComponent = AcousticMesh.StaticMesh->GetStaticMesh();

  FMatrix AcousticMeshLM;
  bool uploadSuccessful = false;
  for (int InstanceID = 0; InstanceID < InstanceCount; InstanceID++) {
    if (!AcousticMesh.GetAcousticMeshLocalMatrix(AcousticGeoCompWM, InstanceID, AcousticMeshLM))
      return false;

    uploadSuccessful = UploadMeshFilter(
        Context,
        MeshGroups,
        Vertices,
        Indices,
        VertexOffset,
        IndexOffset,
        GroupOffset,
        StaticMeshComponent,
        AcousticMesh.LOD,
        AcousticMesh.Materials,
        AcousticMeshLM);

    if (!uploadSuccessful)
      return false;
  }

  return true;
}

// Handles the geometry upload and accounts for instanced static meshes...
static bool HandleNextMeshFilterUpload(
    ovrAudioContext Context,
    TArray<ovrAudioMeshGroup>& MeshGroups,
    TArray<FVector>& Vertices,
    TArray<uint32>& Indices,
    int32& VertexOffset,
    int32& IndexOffset,
    int32& GroupOffset,
    const AcousticMesh& AcousticMesh,
    const FMatrix& AcousticGeoCompWM) {
  if (AcousticMesh.IsInstanced()) {
    return UploadInstancedMeshFilter(
        Context, MeshGroups, Vertices, Indices, VertexOffset, IndexOffset, GroupOffset, AcousticMesh, AcousticGeoCompWM);
  }

  const UStaticMeshComponent* StaticMeshCompPtr = AcousticMesh.StaticMesh;
  const FMatrix AcousticMeshWorldMatrix = UKismetMathLibrary::Conv_TransformToMatrix(StaticMeshCompPtr->GetComponentTransform());
  // relative to UMetaXRAcousticGeometry
  const FMatrix AcousticMeshLocalMatrix =
      UKismetMathLibrary::Multiply_MatrixMatrix(AcousticMeshWorldMatrix, UKismetMathLibrary::Matrix_GetInverse(AcousticGeoCompWM));
  return UploadMeshFilter(
      Context,
      MeshGroups,
      Vertices,
      Indices,
      VertexOffset,
      IndexOffset,
      GroupOffset,
      StaticMeshCompPtr->GetStaticMesh(),
      AcousticMesh.LOD,
      AcousticMesh.Materials,
      AcousticMeshLocalMatrix);
}

#if WITH_EDITOR
static bool UploadLandscapeFilter(
    ovrAudioContext Context,
    TArray<ovrAudioMeshGroup>& MeshGroups,
    TArray<FVector>& Vertices,
    TArray<uint32>& Indices,
    int32& VertexOffset,
    int32& IndexOffset,
    int32& GroupOffset,
    UMetaXRAcousticGeometry::FLandscapeMaterial Landscape,
    FMatrix WorldToLocal) {
  int32 FirstIndex = IndexOffset;
  int32 FaceCount = 0;
  for (auto It = Landscape.LandscapeInfo->XYtoComponentMap.CreateIterator(); It; ++It) {
    ULandscapeComponent* Component = It.Value();

    // Compute the combined transform to go from mesh-local to geometry-local space.
    FMatrix localMatrix = UKismetMathLibrary::Conv_TransformToMatrix(Component->GetComponentTransform());
    FMatrix Matrix = UKismetMathLibrary::Multiply_MatrixMatrix(localMatrix, UKismetMathLibrary::Matrix_GetInverse(WorldToLocal));

    FLandscapeComponentDataInterface DataInterface(Component);
    const int Offset = VertexOffset;
    const int Length = Component->ComponentSizeQuads + 1;

    for (auto y = 0; y < Length; ++y) {
      for (auto x = 0; x < Length; ++x) {
        FVector vertex = DataInterface.GetLocalVertex(x, y);
        Vertices[VertexOffset++] = MetaXRAudioUtilities::ToOVRVector(UKismetMathLibrary::Matrix_TransformPosition(Matrix, vertex));

        // there are only (N-1)^2 quads for N^2 verts
        if (x == (Length - 1) || y == (Length - 1))
          continue;

        auto calcIndex = [](int x, int y, int length) { return (y * length) + x; };
        Indices[IndexOffset + 0] = Offset + calcIndex(x, y, Length);
        Indices[IndexOffset + 1] = Offset + calcIndex(x, y + 1, Length);
        Indices[IndexOffset + 2] = Offset + calcIndex(x + 1, y + 1, Length);
        Indices[IndexOffset + 3] = Offset + calcIndex(x + 1, y, Length);

        IndexOffset += 4;
        ++FaceCount;
      }
    }
  }

  ovrAudioMaterial OvrMaterial = nullptr;
  if (!Landscape.Materials.IsEmpty()) {
    ovrResult Result = OVRA_CALL(ovrAudio_CreateAudioMaterial)(Context, &OvrMaterial);
    if (Result != ovrSuccess) {
      METAXR_AUDIO_LOG_WARNING("Unabled to create audio material for landscape!");
      return false;
    }
    Landscape.Materials[0]->ConstructMaterial(OvrMaterial);
  }

  ovrAudioMeshGroup& MeshGroup = MeshGroups[GroupOffset++];
  MeshGroup.faceCount = FaceCount;
  MeshGroup.faceType = ovrAudioFaceType_Quads;
  MeshGroup.material = OvrMaterial;
  MeshGroup.indexOffset = FirstIndex;

  return true;
}
#endif

bool UMetaXRAcousticGeometry::UploadMesh(ovrAudioGeometry GeometryHandle) {
  int32 IgnoredMeshCount = 0;
  return UploadMesh(GeometryHandle, GetOwner(), false, IgnoredMeshCount);
}

bool UMetaXRAcousticGeometry::UploadMesh(
    ovrAudioGeometry GeometryHandle,
    AActor* Owner,
    bool IgnoreStatic,
    int& OutIgnoredMeshCount) { // output parameter
  OutIgnoredMeshCount = 0;
  if (CachedContext == nullptr)
    return false;

  CheckGeoTransformValid();

  // Get the child mesh objects.
  auto Gatherer = FMeshGatherer(IgnoreStatic, bUsePhysicalMaterials, bIncludeChildren, LOD);
  TraverseHierarchy(Gatherer);

  int32 TotalVertexCount = 0;
  uint32 TotalIndexCount = 0;
  int32 TotalFaceCount = 0;
  int32 TotalMaterialCount = 0;
  // Update the counts for all static meshes
  for (const auto& MeshMaterial : Gatherer.GetMeshes())
    UpdateCountsForMesh(TotalVertexCount, TotalIndexCount, TotalFaceCount, TotalMaterialCount, MeshMaterial);

#if WITH_EDITOR
  // Update the counts for all landscapes
  for (const auto& LandscapeMaterial : Gatherer.GetTerrains())
    UpdateCountsForLandscape(TotalVertexCount, TotalIndexCount, TotalFaceCount, TotalMaterialCount, LandscapeMaterial.LandscapeInfo);
#endif

  TArray<ovrAudioMeshGroup> MeshGroups{};
  MeshGroups.SetNumUninitialized(TotalMaterialCount);
  TArray<FVector> Vertices{};
  Vertices.SetNumUninitialized(TotalVertexCount);
  TArray<uint32> Indices{};
  Indices.SetNumUninitialized(TotalIndexCount);

  int32 VertexOffset = 0;
  int32 IndexOffset = 0;
  int32 GroupOffset = 0;
  // Append each static meshes details to the aggregate arrays
  const FMatrix AcousticGeoCompWorldMatrix = UKismetMathLibrary::Conv_TransformToMatrix(GetComponentTransform());
  for (const auto& Mesh : Gatherer.GetMeshes()) {
    if (!HandleNextMeshFilterUpload(
            CachedContext, MeshGroups, Vertices, Indices, VertexOffset, IndexOffset, GroupOffset, Mesh, AcousticGeoCompWorldMatrix)) {
      return false;
    }
  }

#if WITH_EDITOR
  // Append each landscape materials details to the aggregate arrays
  for (const auto& Landscape : Gatherer.GetTerrains()) {
    if (!UploadLandscapeFilter(
            CachedContext, MeshGroups, Vertices, Indices, VertexOffset, IndexOffset, GroupOffset, Landscape, AcousticGeoCompWorldMatrix)) {
      return false;
    }
  }
#endif

  if (TotalVertexCount == 0) {
    METAXR_AUDIO_LOG_ERROR("Unable to upload mesh, vertex count is zero %s", *FilePath);
    return false;
  }

  METAXR_AUDIO_LOG("Uploading mesh %s with %i vertices", *FilePath, TotalVertexCount);

  float UnitScale = 0.01f;
  ovrAudioMeshSimplification Simplification{};
  Simplification.thisSize = sizeof(ovrAudioMeshSimplification);
  Simplification.flags = static_cast<ovrAudioMeshFlags>(MeshFlags);
  // UI is in centimeters because game units but the ovrAudio API is meters
  Simplification.unitScale = UnitScale;
  Simplification.maxError = MaxError * UnitScale;
  Simplification.minDiffractionEdgeAngle = 1.0f;
  Simplification.minDiffractionEdgeLength = 1.0f * UnitScale;
  Simplification.flagLength = 100.0f * UnitScale;
#if WITH_EDITOR
  Simplification.threadCount = 0; // Use as many threads as CPUs
#else
  Simplification.threadCount = 1;
#endif

  ovrResult Result = OVRA_CALL(ovrAudio_AudioGeometryUploadSimplifiedMeshArrays)(
      GeometryHandle,
      Vertices.GetData(),
      0,
      Vertices.Num(),
      0,
      ovrAudioScalarType_Float64,
      Indices.GetData(),
      0,
      Indices.Num(),
      ovrAudioScalarType_UInt32,
      MeshGroups.GetData(),
      MeshGroups.Num(),
      &Simplification);
  if (Result != ovrSuccess) {
    METAXR_AUDIO_LOG_WARNING("Failed adding geometry to the audio propagation sub-system!");
    return false;
  } else {
    METAXR_AUDIO_LOG("Successfully uploaded geometry %p", GeometryHandle);
  }

  Result = OVRA_CALL(ovrAudio_AudioGeometrySetObjectFlag)(GeometryHandle, ovrAudioObjectFlag_Enabled, true);
  Result = OVRA_CALL(ovrAudio_AudioGeometrySetObjectFlag)(GeometryHandle, ovrAudioObjectFlag_Static, IsStatic());

#if WITH_EDITOR
  // Need to remap the gizmo materials after a bake
  if (GizmoData) {
    FScopeLock LockGuard(&GizmoUpdateCS);
    GizmoData->MapGizmoMaterials(Gatherer);
  }
  HierarchyHash = ComputeHash();
#endif

  // Clean up native handles
  for (const ovrAudioMeshGroup& Group : MeshGroups) {
    if (Group.material != nullptr) {
      Result = OVRA_CALL(ovrAudio_DestroyAudioMaterial)(Group.material);
      if (Result != ovrSuccess) {
        METAXR_AUDIO_LOG_WARNING("Failed to destroy material %p", GeometryHandle);
      }
    }
  }

  return (Result == ovrSuccess);
}
#pragma endregion

bool UMetaXRAcousticGeometry::DestroyInternal() {
  return DestroyPropagationGeometry();
}

bool UMetaXRAcousticGeometry::DestroyPropagationGeometry() {
  if (OvrGeometry == nullptr)
    return false;

  METAXR_AUDIO_LOG("Destroying geometry handle %p", OvrGeometry);
  ovrResult Result = OVRA_CALL(ovrAudio_DestroyAudioGeometry)(OvrGeometry);
  if (Result != ovrSuccess)
    METAXR_AUDIO_LOG_WARNING("Unable to destroy geometry");

  OvrGeometry = nullptr;
  return true;
}

void UMetaXRAcousticGeometry::TraverseHierarchy(ITransformVisitor& Visitor) const {
  if (TraversalMode == ETraversalMode::Default || TraversalMode == ETraversalMode::Actor)
    Visitor.TraverseActorHierarchy(GetOwner());
  else if (TraversalMode == ETraversalMode::SceneComponent)
    Visitor.TraverseSceneComponentHierarchy(this);
  else
    METAXR_AUDIO_LOG_ERROR("Unknown traversal mode. Cannot traverse hierarchy. Skipping...");
}

void UMetaXRAcousticGeometry::ApplyTransform() {
  if (OvrGeometry == nullptr) {
    METAXR_AUDIO_LOG_ERROR("No ovrGeoemtry for MetaXRAcousticGeometry");
    return;
  }

  const FTransform& UETransform = GetComponentTransform();
  float OVRTransform[16];
  MetaXRAudioUtilities::ConvertUETransformToOVRTransform(UETransform, OVRTransform);

  ovrResult Result = OVRA_CALL(ovrAudio_AudioGeometrySetTransform)(OvrGeometry, OVRTransform);
  if (Result != ovrSuccess) {
    METAXR_AUDIO_LOG("Failed at setting new audio propagation mesh transform!");
    return;
  } else {
    METAXR_AUDIO_LOG("Set transform for geometry %p", OvrGeometry);
  }

  PreviousTransform = UETransform;
  PreviousGeometry = OvrGeometry;
}

bool UMetaXRAcousticGeometry::ReadFile() {
  if (FilePath.IsEmpty()) {
    METAXR_AUDIO_LOG_WARNING("No geometry file path provided!");
    return false;
  }

  if (IsPlaymodeActive()) {
    LoadGeometryAsync();
  } else {
    const FString FullFilePath = FPaths::ProjectContentDir() / FilePath;
    if (!FPaths::FileExists(FullFilePath)) {
      METAXR_AUDIO_LOG_WARNING("Audio geometry file not found: %s", *FullFilePath);
      return false;
    }

    // Read the file data into a byte array
    ovrResult Result = OVRA_CALL(ovrAudio_AudioGeometryReadMeshFile)(OvrGeometry, TCHAR_TO_UTF8(*FullFilePath));
    if (Result != ovrSuccess) {
      METAXR_AUDIO_LOG_WARNING("Unable to read audio geometry from file: %s", *FullFilePath);
      return false;
    } else {
      METAXR_AUDIO_LOG("Successfully read geometry from file: %s", *FullFilePath);
    }

    Result = OVRA_CALL(ovrAudio_AudioGeometrySetObjectFlag)(OvrGeometry, ovrAudioObjectFlag_Enabled, true);
    Result = OVRA_CALL(ovrAudio_AudioGeometrySetObjectFlag)(OvrGeometry, ovrAudioObjectFlag_Static, IsStatic());
  }

#if WITH_EDITOR
  UpdateGizmoMesh(OvrGeometry);
#endif
  return true;
}

// See: FMetaXRAcousticGeometryDetails
bool UMetaXRAcousticGeometry::WriteFile() {
  // Create a temporary geometry.
  ovrAudioGeometry tempGeometryHandle;
  ovrResult Result = OVRA_CALL(ovrAudio_CreateAudioGeometry)(CachedContext, &tempGeometryHandle);
  if (Result != ovrSuccess) {
    METAXR_AUDIO_LOG_WARNING("Unable to create temp audio geometry");
    return false;
  } else {
    METAXR_AUDIO_LOG("Created new temporary geometry handle %p", tempGeometryHandle);
  }

  // Upload the mesh geometry and then write the result to disk
  bool Succeeded = false;
  if (UploadMesh(tempGeometryHandle)) {
    if (WriteFileInternal(tempGeometryHandle)) {
      Succeeded = true;
#if WITH_EDITOR
      bNeedsRebake = false;
#endif // WITH_EDITOR
    }
  }

  // Destroy the geometry.
  METAXR_AUDIO_LOG("Destroying temporary geometry handle %p", tempGeometryHandle);
  if (OVRA_CALL(ovrAudio_DestroyAudioGeometry)(tempGeometryHandle) != ovrSuccess) {
    METAXR_AUDIO_LOG_WARNING("Failed to destroy temp geometry handle");
    Succeeded = false;
  }

  return Succeeded;
}

bool UMetaXRAcousticGeometry::WriteFileInternal(ovrAudioGeometry GeometryHandle) {
  // Setup the filepaths for the output file
#if WITH_EDITOR
  GenerateFileNameIfEmpty();
#endif
  const FString FullFilePath = FPaths::ProjectContentDir() / FilePath;
#if WITH_EDITOR
  MetaXRAudioUtilities::CreateMetaXRAcousticContentDirectory(FPaths::GetPath(FilePath));
#endif

  // Write the mesh to a file
  ovrResult Result = OVRA_CALL(ovrAudio_AudioGeometryWriteMeshFile)(GeometryHandle, TCHAR_TO_ANSI(*FullFilePath));
  if (Result != ovrSuccess) {
    METAXR_AUDIO_LOG_WARNING("Unable to save geometry to file %s", *FullFilePath);
    return false;
  } else {
    METAXR_AUDIO_LOG("Successfully wrote geometry to file: %s", *FullFilePath);
  }

#if WITH_EDITOR
  UpdateGizmoMesh(GeometryHandle);
#endif

  if (!bFileEnabled) {
    METAXR_AUDIO_LOG_WARNING("File Successfully written but File Enabled is off, turn it on to use file");
  }

  return true;
}

void UMetaXRAcousticGeometry::LoadGeometryAsync() {
  FString FullFilePath = FPaths::ProjectContentDir() / FilePath;
#if WITH_EDITOR
  if (!FPaths::FileExists(FullFilePath)) {
    METAXR_AUDIO_LOG_WARNING("Audio geometry file not found: %s", *FullFilePath);
    return;
  }
#endif

  // Read the file data into a byte array
  TArray<uint8> FileData;
  if (!FFileHelper::LoadFileToArray(FileData, *FullFilePath)) {
    METAXR_AUDIO_LOG_WARNING("Failed to load audio geometry file: %s", *FullFilePath);
    return;
  }

  ovrResult Result = OVRA_CALL(ovrAudio_AudioGeometryReadMeshMemory)(OvrGeometry, (const int8_t*)FileData.GetData(), FileData.Num());
  if (Result != ovrSuccess) {
    METAXR_AUDIO_LOG_WARNING("Unable to read audio geometry from memory: %s", *FullFilePath);
    return;
  } else {
    METAXR_AUDIO_LOG("Successfully read audio geometry from memory: %s", *FullFilePath);
  }

  Result = OVRA_CALL(ovrAudio_AudioGeometrySetObjectFlag)(OvrGeometry, ovrAudioObjectFlag_Enabled, true);
  ApplyTransform();
  Result = OVRA_CALL(ovrAudio_AudioGeometrySetObjectFlag)(OvrGeometry, ovrAudioObjectFlag_Static, IsStatic());

#if WITH_EDITOR
  UpdateGizmoMesh(OvrGeometry);
#endif
}

void UMetaXRAcousticGeometry::PostLoad() {
  Super::PostLoad();

#if WITH_EDITOR
  if (!IsPlaymodeActive())
    DestroyInternal();
#endif
}

void UMetaXRAcousticGeometry::BeginDestroy() {
  Super::BeginDestroy();

  if (OvrGeometry != nullptr && CachedContext != nullptr)
    DestroyInternal();

#if WITH_EDITOR
  {
    FScopeLock LockGuard(&GizmoUpdateCS);
    GizmoData.Reset();
  }
#endif
}

void UMetaXRAcousticGeometry::DestroyComponent(bool bPromoteChildren) {
  Super::DestroyComponent(bPromoteChildren);
}

void UMetaXRAcousticGeometry::Activate(bool bReset) {
  Super::Activate(bReset);

  if (OvrGeometry == nullptr)
    return;

  ovrResult Result = OVRA_CALL(ovrAudio_AudioGeometrySetObjectFlag)(OvrGeometry, ovrAudioObjectFlag_Enabled, true);
  ApplyTransform();
  Result = OVRA_CALL(ovrAudio_AudioGeometrySetObjectFlag)(OvrGeometry, ovrAudioObjectFlag_Static, IsStatic());
  METAXR_AUDIO_LOG("Set transform and activated for geometry %p", OvrGeometry);
}

void UMetaXRAcousticGeometry::Deactivate() {
  Super::Deactivate();

  if (OvrGeometry == nullptr)
    return;

  ovrResult Result = OVRA_CALL(ovrAudio_AudioGeometrySetObjectFlag)(OvrGeometry, ovrAudioObjectFlag_Enabled, false);
  ApplyTransform();
  Result = OVRA_CALL(ovrAudio_AudioGeometrySetObjectFlag)(OvrGeometry, ovrAudioObjectFlag_Static, IsStatic());
  METAXR_AUDIO_LOG("Set transform and deactivated for geometry %p", OvrGeometry);
}

#if WITH_EDITOR
void UMetaXRAcousticGeometry::OnComponentCreated() {
  Super::OnComponentCreated();

  const AActor* Owner = GetOwner();
  if (!Owner || TraversalMode != ETraversalMode::Default)
    return;

  const TFunction<void(UMetaXRAcousticGeometry*)> Modifier = [](UMetaXRAcousticGeometry* DefaultBPComponent) {
    DefaultBPComponent->TraversalMode = ETraversalMode::SceneComponent;
  };

  switch (CreationMethod) {
    // user added this component in blueprint details panel.
    // This is called every blueprint load and this is why above TraversalMode check exists.
    case EComponentCreationMethod::SimpleConstructionScript:
      // change the value on the instance itself as well as the blueprint asset.
      TraversalMode = ETraversalMode::SceneComponent;
      MetaXRAudioUtilities::ModifyBlueprintAssetComponent<UMetaXRAcousticGeometry>(Owner, Modifier);
      break;
    case EComponentCreationMethod::Instance:
      // user added it directly on a placed Actor in the level
      Modify();
      TraversalMode = ETraversalMode::Actor;
      PostEditChange();
      break;
  }
}

void UMetaXRAcousticGeometry::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) {
  CheckGeoTransformValid();
  RefreshNeedsRebaked();
  // Must be called after our updated above in order to make sure everythings updated correctly.
  Super::PostEditChangeProperty(PropertyChangedEvent);
}

FPrimitiveSceneProxy* UMetaXRAcousticGeometry::CreateSceneProxy() {
#if WITH_EDITOR_GIZMOS
  return new FMetaXRAcousticGeometrySceneProxy(this);
#else
  return nullptr;
#endif
}

// This function tells the renderer where the geometry is so it can only render it while in frame
FBoxSphereBounds UMetaXRAcousticGeometry::CalcBounds(const FTransform& LocalToWorld) const {
  FBoxSphereBounds GeometryBounds = Super::CalcBounds(LocalToWorld);

  // Check if we are in a UWorld. Its possible for this to get called on Map transition or start of editor which can make UWorld* null.
  // This can cause a crash if we call CreateLandscapeInfo on a ALandscape actor.
  const UWorld* WorldPtr = GetWorld();
  if (WorldPtr == nullptr)
    return GeometryBounds;

  // Collect all the meshes and landscapes associated with this geometry.
  // Always traverse actor hierarchy for bounds.
  FMeshGatherer Gatherer = FMeshGatherer(false, bUsePhysicalMaterials, bIncludeChildren, LOD);
  Gatherer.TraverseActorHierarchy(GetOwner());

  // Update the counts for all static meshes
  for (const AcousticMesh& MeshData : Gatherer.GetMeshes()) {
    const UStaticMeshComponent* AcousticMesh = MeshData.StaticMesh;
    GeometryBounds = GeometryBounds + AcousticMesh->GetStaticMesh()->GetBounds().TransformBy(AcousticMesh->GetComponentTransform());
  }

  // Update the counts for all landscapes
  for (const FLandscapeMaterial& LandscapeMaterial : Gatherer.GetTerrains()) {
    for (auto It = LandscapeMaterial.LandscapeInfo->XYtoComponentMap.CreateIterator(); It; ++It) {
      const ULandscapeComponent* Component = It.Value();
      GeometryBounds = GeometryBounds + Component->Bounds.TransformBy(Component->GetComponentTransform());
    }
  }

  return GeometryBounds;
}

FMetaXRAcousticGeometrySceneProxy::FMetaXRAcousticGeometrySceneProxy(const UPrimitiveComponent* InComponent)
    : FDebugRenderSceneProxy(InComponent) {
  GeometryComponent = Cast<UMetaXRAcousticGeometry>(InComponent);
}

FPrimitiveViewRelevance FMetaXRAcousticGeometrySceneProxy::GetViewRelevance(const FSceneView* View) const {
  static const IConsoleVariable* const MetaXRAudioGlobalGizmoCVAR = IConsoleManager::Get().FindConsoleVariable(TEXT("MetaXRAudioGizmos"));
  static const IConsoleVariable* const MetaXRAudioEditorTimeGeoGizmoCVAR =
      IConsoleManager::Get().FindConsoleVariable(TEXT("MetaXRAudioGizmos.Geometries.EditorTime"));
  static const IConsoleVariable* const MetaXRAudioEditorPlaymodeGeoGizmoCVAR =
      IConsoleManager::Get().FindConsoleVariable(TEXT("MetaXRAudioGizmos.Geometries.EditorPlaymode"));

  bool ShouldShowGizmo = false;
  if (MetaXRAudioUtilities::PlayModeActive(GeometryComponent->GetWorld())) {
    ShouldShowGizmo = GeometryComponent->IsActive() && MetaXRAudioEditorPlaymodeGeoGizmoCVAR &&
        MetaXRAudioEditorPlaymodeGeoGizmoCVAR->GetInt() != 0 && MetaXRAudioGlobalGizmoCVAR && MetaXRAudioGlobalGizmoCVAR->GetInt() != 0;
  } else {
    ShouldShowGizmo = MetaXRAudioEditorTimeGeoGizmoCVAR && MetaXRAudioEditorTimeGeoGizmoCVAR->GetInt() != 0 && MetaXRAudioGlobalGizmoCVAR &&
        MetaXRAudioGlobalGizmoCVAR->GetInt() != 0;
  }

  FPrimitiveViewRelevance Result;
  Result.bDrawRelevance = ShouldShowGizmo;
  Result.bDynamicRelevance = true;
  return Result;
}

void FMetaXRAcousticGeometrySceneProxy::GetDynamicMeshElements(
    const TArray<const FSceneView*>& Views,
    const FSceneViewFamily& ViewFamily,
    uint32 VisibilityMap,
    FMeshElementCollector& Collector) const {
  if (!IsValid(GeometryComponent))
    return;

  if (!GeometryComponent->GizmoUpdateCS.TryLock())
    return;

  // Its possible that BeginDestroy was called just before acquiring lock.
  if (!GeometryComponent->GizmoData) {
    GeometryComponent->GizmoUpdateCS.Unlock();
    return;
  }

  for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++) {
    if (VisibilityMap & (1 << ViewIndex)) {
      const FSceneView* View = Views[ViewIndex];
      const TArray<FDynamicMeshVertex>& GizmoVertices = GeometryComponent->GizmoData->GetGizmoVertexData();
      const TMap<UMetaXRAcousticMaterialProperties*, TArray<uint32>>& GizmoMaterialData =
          GeometryComponent->GizmoData->GetGizmoMaterialData();

      for (const TPair<UMetaXRAcousticMaterialProperties*, TArray<uint32>>& MaterialMesh : GizmoMaterialData) {
        const UMetaXRAcousticMaterialProperties* Material = MaterialMesh.Key;
        const TArray<uint32>& MaterialIndices = MaterialMesh.Value;

        if (MaterialIndices.Num() == 0 || GizmoVertices.Num() == 0)
          continue;

        // Set up material for wireframe rendering
        const FLinearColor Color = Material ? Material->Color : FLinearColor::Yellow;
        FColoredMaterialRenderProxy* WireframeMaterialInstance =
            new FColoredMaterialRenderProxy(GEngine->WireframeMaterial->GetRenderProxy(), Color);

        // Add the wireframe material to the collector
        Collector.RegisterOneFrameMaterialProxy(WireframeMaterialInstance);

        // Add the vertices and corresponding indices for the mesh with this material
        FDynamicMeshBuilder MeshBuilder(View->GetFeatureLevel());
        MeshBuilder.AddVertices(GizmoVertices);
        MeshBuilder.AddTriangles(MaterialIndices);
        // Add mesh to the collector
        MeshBuilder.GetMesh(GetLocalToWorld(), WireframeMaterialInstance, SDPG_World, false, false, false, ViewIndex, Collector);
      }
    }
  }

  GeometryComponent->GizmoUpdateCS.Unlock();
}
#endif

#pragma region ACOUSTIC_MESH
const bool UMetaXRAcousticGeometry::FMeshMaterial::IsInstanced() const {
  return Cast<UInstancedStaticMeshComponent>(StaticMesh) != nullptr;
}

const int32 UMetaXRAcousticGeometry::FMeshMaterial::GetInstancedCount() const {
  const UInstancedStaticMeshComponent* ISMCmp = Cast<UInstancedStaticMeshComponent>(StaticMesh);
  if (ISMCmp == nullptr)
    return -1;

  return ISMCmp->GetInstanceCount();
}

const bool UMetaXRAcousticGeometry::FMeshMaterial::GetAcousticMeshLocalMatrix(
    const FMatrix& AcousticGeoCompWorldMatrix,
    const int32 InstanceID,
    FMatrix& OutLocalMatrix) const {
  const bool IsInstancedMesh = IsInstanced();
  if (InstanceID < 0) {
    // sanity check
    if (IsInstancedMesh) {
      METAXR_AUDIO_LOG_ERROR("Call to GetAcousticMeshLocalMatrix with InstanceID < 0, but the mesh is instanced");
      return false;
    }

    // get mesh transform relative to MetaXRAcousticGeometry component transform
    const FMatrix AcousticMeshWorldMatrix = UKismetMathLibrary::Conv_TransformToMatrix(StaticMesh->GetComponentTransform());
    OutLocalMatrix = UKismetMathLibrary::Multiply_MatrixMatrix(
        AcousticMeshWorldMatrix, UKismetMathLibrary::Matrix_GetInverse(AcousticGeoCompWorldMatrix));
    return true;
  } else if (IsInstancedMesh) {
    if (InstanceID >= GetInstancedCount()) {
      METAXR_AUDIO_LOG_ERROR("Call to GetAcousticMeshLocalMatrix with invalid instance ID");
      return false;
    }

    const UInstancedStaticMeshComponent* ISMCmp = Cast<UInstancedStaticMeshComponent>(StaticMesh);
    check(ISMCmp != nullptr);
    FTransform InstanceTransform;
    if (!ISMCmp->GetInstanceTransform(InstanceID, InstanceTransform, true)) {
      METAXR_AUDIO_LOG_ERROR("Unable to retrieve instance transform");
      return false;
    }

    const FMatrix AcousticMeshWorldMatrix = UKismetMathLibrary::Conv_TransformToMatrix(InstanceTransform);
    OutLocalMatrix = UKismetMathLibrary::Multiply_MatrixMatrix(
        AcousticMeshWorldMatrix, UKismetMathLibrary::Matrix_GetInverse(AcousticGeoCompWorldMatrix));
    return true;
  }

  METAXR_AUDIO_LOG_ERROR("Call to GetAcousticMeshLocalMatrix with InstanceID >= 0, but the mesh is NOT instanced");
  return false;
}
#pragma endregion

#pragma region UTILS
static TArray<FString> GetExcludeTags() {
  const UMetaXRAcousticProjectSettings* Settings = GetDefault<UMetaXRAcousticProjectSettings>();
  return Settings->ExcludeTags;
}

static bool HasExcludeTag(const AActor* Actor) {
  const TArray<FString> ExcludeTags = GetExcludeTags();
  for (const FName& ActorTag : Actor->Tags) {
    if (ExcludeTags.Contains(ActorTag.ToString()))
      return true;
  }
  return false;
}
static bool HasExcludeTag(const USceneComponent* SC) {
  const TArray<FString> ExcludeTags = GetExcludeTags();
  for (const FName& ComponentTag : SC->ComponentTags) {
    if (ExcludeTags.Contains(ComponentTag.ToString()))
      return true;
  }
  return false;
}

static uint32 GetAcousticMaterials(const USceneComponent* SC, TArray<UMetaXRAcousticMaterialProperties*>& OutAcousticMaterials) {
  if (!SC)
    return 0;

  const TArray<UAssetUserData*>* UserData = SC->GetAssetUserDataArray();
  if (!UserData || UserData->Num() <= 0)
    return 0;

  const TArray<UAssetUserData*>& UserDataRef = *UserData;
  UAcousticMaterialUserData* AcousticUserData = nullptr;
  for (UAssetUserData* UD : UserDataRef) {
    UAcousticMaterialUserData* AUD = Cast<UAcousticMaterialUserData>(UD);
    if (AUD && AUD->MaterialPresets.Num() > 0) {
      AcousticUserData = AUD;
      break; // we only allow 1 UAcousticMaterialUserData per static mesh
    }
  }
  if (!AcousticUserData)
    return 0;

  // IF UAcousticMaterialUserData was just added, MaterialPresets can have null materials but a count of greater 0.
  // Filter out the null materials...
  for (const TObjectPtr<UMetaXRAcousticMaterialProperties>& AcousticMat : AcousticUserData->MaterialPresets) {
    if (!IsValid(AcousticMat))
      continue;
    OutAcousticMaterials.Add(AcousticMat);
  }
  return OutAcousticMaterials.Num();
}
static uint32 GetAcousticMaterials(const AActor* Actor, TArray<UMetaXRAcousticMaterialProperties*>& OutAcousticMaterials) {
  if (!Actor)
    return 0;

  TArray<UMetaXRAcousticMaterial*> AcousticMaterialComponents;
  Actor->GetComponents<UMetaXRAcousticMaterial>(AcousticMaterialComponents);
  for (const UMetaXRAcousticMaterial* AcousticMatComp : AcousticMaterialComponents) {
    UMetaXRAcousticMaterialProperties* const AcousticMat = AcousticMatComp->GetMaterialPreset();
    if (IsValid(AcousticMat))
      OutAcousticMaterials.Add(AcousticMat);
  }
  return OutAcousticMaterials.Num();
}

static bool HasAcousticMaterial(const AActor* Actor) {
  return Actor->FindComponentByClass<UMetaXRAcousticMaterial>() != nullptr;
}
static bool HasAcousticMaterial(const USceneComponent* SC) {
  TArray<UMetaXRAcousticMaterialProperties*> OutMaterials;
  return GetAcousticMaterials(SC, OutMaterials) > 0;
}

static UMetaXRAcousticMaterialProperties* GetMaterialMapping(FBodyInstance* BodyInstance) {
  const UMetaXRAcousticProjectSettings* AcousticProjectSettings = GetDefault<UMetaXRAcousticProjectSettings>();
  UMetaXRAcousticMaterialProperties* MetaMaterial = nullptr;
  if (AcousticProjectSettings) {
    // Start by assuming fallback will be used unless overridden
    UMetaXRAcousticMaterialProperties* FallbackMaterial =
        Cast<UMetaXRAcousticMaterialProperties>(AcousticProjectSettings->FallbackMaterial.TryLoad());
    if (FallbackMaterial) {
      MetaMaterial = FallbackMaterial;
    }

    // Get the Physical Material and find its mapping to a Meta Material if applicable
    UPhysicalMaterial* MeshPhysicalMaterial = nullptr;
    if (BodyInstance) {
      // This function searches all possible materials for the body including overrides. Returns GEngine->DefaultPhysMaterial if nothing
      // available
      MeshPhysicalMaterial = BodyInstance->GetSimplePhysicalMaterial();
    }
    if (MeshPhysicalMaterial) {
      // Loop through the user defined material mappings and see if any match this mesh's physical material
      for (FMaterialLinkage Linkage : AcousticProjectSettings->MaterialMapping) {
        UPhysicalMaterial* CurrentPhysicalMaterial = Cast<UPhysicalMaterial>(Linkage.PhysicalMaterial.TryLoad());
        if (MeshPhysicalMaterial == CurrentPhysicalMaterial) {
          UMetaXRAcousticMaterialProperties* CurrentAcousticMaterial =
              Cast<UMetaXRAcousticMaterialProperties>(Linkage.AcousticMaterial.TryLoad());
          if (CurrentAcousticMaterial) {
            MetaMaterial = CurrentAcousticMaterial;
          }
        }
      }
    }
  }
  return MetaMaterial;
}

// Append a hash for a specific node that a Visitor is visiting using the Transform, Geometry, and Material information
static void AppendNodeHash(
    FString& Hash,
    const FTransform& Transform,
    const bool UsePhysicalMaterials,
    const ALandscape* LandscapeActor,
    const TArray<const UStaticMeshComponent*>& MeshComponents,
    const TArray<UMetaXRAcousticMaterialProperties*>& AcousticMaterials) {
  // Include transform in hash if it has any relevant components.
  if (!MeshComponents.IsEmpty() || LandscapeActor) {
    FString stringRepresentation = FString::Printf(
        TEXT("%f%f%f%f%f%f%f%f%f%f"),
        Transform.GetTranslation().X,
        Transform.GetTranslation().Y,
        Transform.GetTranslation().Z,
        Transform.GetRotation().W,
        Transform.GetRotation().X,
        Transform.GetRotation().Y,
        Transform.GetRotation().Z,
        Transform.GetScale3D().X,
        Transform.GetScale3D().Y,
        Transform.GetScale3D().Z);
    Hash.Append(stringRepresentation);
  }

  // Include the materials in the hash
  if (!AcousticMaterials.IsEmpty()) {
    for (const UMetaXRAcousticMaterialProperties* MaterialComponent : AcousticMaterials)
      MaterialComponent->AppendHash(Hash);
  } else if (UsePhysicalMaterials) {
    for (const UStaticMeshComponent* MeshComponent : MeshComponents) {
      UMetaXRAcousticMaterialProperties* MappedMaterial = GetMaterialMapping(MeshComponent->GetBodyInstance());
      if (MappedMaterial)
        MappedMaterial->AppendHash(Hash);
    }
  }
}
#pragma endregion

#pragma region TRANSFORM_VISITOR
void UMetaXRAcousticGeometry::ITransformVisitor::TraverseActorHierarchy(const AActor* Actor) {
  TraverseActorHierarchyInternal(Actor, false);
}

void UMetaXRAcousticGeometry::ITransformVisitor::TraverseActorHierarchyInternal(
    const AActor* Actor,
    const bool bParentWasExcluded,
    const TArray<UMetaXRAcousticMaterialProperties*>* ParentData) {
  if (Actor == nullptr)
    return;

  // Begin analysis of whether to visit this component or not
  bool bShouldVisit = true;
  if (bParentWasExcluded) {
    METAXR_AUDIO_LOG("Skipping Object %s based on excluded parent", *Actor->GetActorNameOrLabel());
    bShouldVisit = false;
  }

  // Check if an exclude tag matches this component and do not visit if so
  if (HasExcludeTag(Actor)) {
    METAXR_AUDIO_LOG("Skipping Object %s based on exclude tag", *Actor->GetActorNameOrLabel());
    bShouldVisit = false;
  }

  // We should override exclude tags if there is a material on this child component
  if (HasAcousticMaterial(Actor) && !bShouldVisit) {
    METAXR_AUDIO_LOG("Override exclude tag due to presence of acoustic material in child %s", *Actor->GetActorNameOrLabel());
    bShouldVisit = true;
  }

  TArray<UMetaXRAcousticMaterialProperties*> CurrentData;
  if (bShouldVisit)
    CurrentData = VisitActor(Actor, ParentData);

  if (!bShouldIncludeChildren)
    return;

  TArray<AActor*> AttachedActors;
  Actor->GetAttachedActors(AttachedActors);
  for (AActor* Child : AttachedActors) {
    if (UMetaXRAcousticGeometry* ChildAcousticGeometry = Child->FindComponentByClass<UMetaXRAcousticGeometry>()) {
      const FString ChildAcousticGeoName = ChildAcousticGeometry->GetOwner()->GetName();
      METAXR_AUDIO_LOG("Skipping child: %s, it has its own UMetaXRAcousticGeometry component", *ChildAcousticGeoName);
      continue;
    }

    this->TraverseActorHierarchyInternal(Child, !bShouldVisit, &CurrentData);
  }
}

void UMetaXRAcousticGeometry::ITransformVisitor::TraverseSceneComponentHierarchy(const USceneComponent* AcousticGeo) {
  TraverseSceneComponentHierarchyInternal(AcousticGeo, false);
}

void UMetaXRAcousticGeometry::ITransformVisitor::TraverseSceneComponentHierarchyInternal(
    const USceneComponent* SceneComponent,
    const bool bParentWasExcluded,
    const TArray<UMetaXRAcousticMaterialProperties*>* ParentData) {
  if (SceneComponent == nullptr)
    return;

  // Begin analysis of whether to visit this component or not
  bool bShouldVisit = true;
  if (bParentWasExcluded) {
    METAXR_AUDIO_LOG("Skipping Object %s based on excluded parent", *SceneComponent->GetName());
    bShouldVisit = false;
  }

  // Check if an exclude tag matches this component and do not visit if so
  if (HasExcludeTag(SceneComponent)) {
    METAXR_AUDIO_LOG("Skipping Object %s based on exclude tag", *SceneComponent->GetName());
    bShouldVisit = false;
  }

  // We should override exclude tags if there is a material on this child component
  if (HasAcousticMaterial(SceneComponent) && !bShouldVisit) {
    METAXR_AUDIO_LOG("Override exclude tag due to presence of acoustic material in child %s", *SceneComponent->GetName());
    bShouldVisit = true;
  }

  TArray<UMetaXRAcousticMaterialProperties*> CurrentData;
  if (bShouldVisit)
    CurrentData = VisitSceneComponent(SceneComponent, ParentData);

  const TArray<TObjectPtr<USceneComponent>>& Children = SceneComponent->GetAttachChildren();
  for (TObjectPtr<USceneComponent> Child : Children) {
    if (Child->IsA<UMetaXRAcousticGeometry>()) {
      const FString ChildSceneComponentName = Child->GetName();
      METAXR_AUDIO_LOG("Skipping child: %s, it has its own UMetaXRAcousticGeometry component", *ChildSceneComponentName);
      continue;
    }

    this->TraverseSceneComponentHierarchyInternal(Child, !bShouldVisit, &CurrentData);
  }
}
#pragma endregion

#pragma region MESH_GATHERER
UMetaXRAcousticGeometry::FMeshGatherer::FMeshGatherer(
    const bool bIgnoreStatic,
    const bool UsePhysicalMaterials,
    const bool bShouldIncludeChildren,
    const int LODSelection)
    : ITransformVisitor(bShouldIncludeChildren),
      bIgnoreStatic(bIgnoreStatic),
      bUsePhysicalMaterials(UsePhysicalMaterials),
      LodSelection(LODSelection) {}

TArray<UMetaXRAcousticMaterialProperties*> UMetaXRAcousticGeometry::FMeshGatherer::VisitActor(
    const AActor* CurrentActor,
    const TArray<UMetaXRAcousticMaterialProperties*>* UserData) {
  TArray<UStaticMeshComponent*> MeshComponents;
  CurrentActor->GetComponents<UStaticMeshComponent>(MeshComponents);
  // IF there are any meta acoustic materials, use those instead of parent materials.
  TArray<UMetaXRAcousticMaterialProperties*> MaterialsToApply;
  const uint32 NewMaterialCount = GetAcousticMaterials(CurrentActor, MaterialsToApply);
  if (NewMaterialCount == 0 && UserData)
    MaterialsToApply = *UserData;

  // Gather the meshes.
  for (UStaticMeshComponent* MeshComponent : MeshComponents) {
    if (bIgnoreStatic &&
        ((MeshComponent->Mobility == EComponentMobility::Static) || (MeshComponent->Mobility == EComponentMobility::Stationary))) {
      METAXR_AUDIO_LOG_ERROR("Mesh not readable. Use \"File Enabled\" for static geometry");
      ++IgnoredMeshCount;
      continue;
    }

    const UStaticMesh* Mesh = MeshComponent->GetStaticMesh();
    if (!Mesh)
      continue;

    const int32 LodGroupToUse = FMath::Clamp(LodSelection, 0, Mesh->GetRenderData()->LODResources.Num() - 1);

    const UAcousticMaterialUserData* AcousticUserData = MeshComponent->GetAssetUserData<UAcousticMaterialUserData>();
    if (IsValid(AcousticUserData) && AcousticUserData->MaterialPresets.Num() > 0) {
      // here we do NOT override MaterialsToApply. UAcousticMaterialUserData applied to the static mesh acts as an override in actor
      // traversal, but these materials will not be passed to children...
      Meshes.Add({MeshComponent, LodGroupToUse, AcousticUserData->MaterialPresets});
    } else if (NewMaterialCount == 0 && bUsePhysicalMaterials) {
      // If no Meta Materials were found, check mesh for Physical Materials to map them to Meta materials
      UMetaXRAcousticMaterialProperties* MetaMaterial = GetMaterialMapping(MeshComponent->GetBodyInstance());
      if (MetaMaterial) {
        MaterialsToApply.SetNum(1); // With Physical Materials, there can only be one per static mesh
        MaterialsToApply[0] = MetaMaterial;
      }
      Meshes.Add({MeshComponent, LodGroupToUse, MaterialsToApply});
    } else {
      Meshes.Add({MeshComponent, LodGroupToUse, MaterialsToApply});
    }
  }

  // Gather the terrains.
#if WITH_EDITOR
  CollectTerrains(CurrentActor, MaterialsToApply);
#endif

  // Check if there a BSP is used and warn the user we don't support it at this time
  const ABrush* brush = Cast<ABrush>(CurrentActor);
  if (brush != nullptr) {
    METAXR_AUDIO_LOG_WARNING(
        "Detected using Meta XR Audio on a BSP component. The plugin doesn't support BSPs at this time and will not be included in the geometry");
  }

  return MaterialsToApply;
}

TArray<UMetaXRAcousticMaterialProperties*> UMetaXRAcousticGeometry::FMeshGatherer::VisitSceneComponent(
    const USceneComponent* CurrentSceneComponent,
    const TArray<UMetaXRAcousticMaterialProperties*>* UserData) {
  // For non-UStaticMeshComponent's we pass along parent material data to children...
  if (!CurrentSceneComponent->IsA<UStaticMeshComponent>())
    return UserData == nullptr ? EmptyAcousticMaterialProps : *UserData;

  const UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(CurrentSceneComponent);
  check(StaticMeshComponent != nullptr);

  // if there are new acoustic materials present, use those, otherwise use the ones passed down from parent...
  TArray<UMetaXRAcousticMaterialProperties*> MaterialsToApply;
  const int32 NewMaterialCount = GetAcousticMaterials(StaticMeshComponent, MaterialsToApply);
  if (NewMaterialCount == 0 && UserData)
    MaterialsToApply = *UserData;

  if (bIgnoreStatic &&
      (StaticMeshComponent->Mobility == EComponentMobility::Static || StaticMeshComponent->Mobility == EComponentMobility::Stationary)) {
    METAXR_AUDIO_LOG_ERROR("Mesh not readable. Use \"File Enabled\" for static geometry");
    ++IgnoredMeshCount;
    return MaterialsToApply;
  }

  const UStaticMesh* Mesh = StaticMeshComponent->GetStaticMesh();
  if (!Mesh)
    return MaterialsToApply;

  if (NewMaterialCount == 0 && bUsePhysicalMaterials) {
    // If no Meta Materials were found, check mesh for Physical Materials to map them to Meta materials
    if (UMetaXRAcousticMaterialProperties* MetaMaterial = GetMaterialMapping(StaticMeshComponent->GetBodyInstance())) {
      MaterialsToApply.SetNum(1); // With Physical Materials, there can only be one per static mesh
      MaterialsToApply[0] = MetaMaterial;
    }
  }

  const int32 LodGroupToUse = FMath::Clamp(LodSelection, 0, Mesh->GetRenderData()->LODResources.Num() - 1);
  Meshes.Add({StaticMeshComponent, LodGroupToUse, MaterialsToApply});
  return MaterialsToApply;
}

void UMetaXRAcousticGeometry::FMeshGatherer::CollectTerrains(
    const AActor* Actor,
    const TArray<UMetaXRAcousticMaterialProperties*>& MaterialsToApply) {
  const ALandscape* LandscapeActor = Cast<ALandscape>(Actor);
  if (!LandscapeActor)
    return;

  // We check if the landscape has a valid UWorld*.
  // Its possible CalcBounds gets called after deleting a umap.
  // When this occurs, LandscapeActor can have a null UWorld.
  const UWorld* LandscapesCachedUWorldPtr = LandscapeActor->GetWorld();
  if (LandscapesCachedUWorldPtr == nullptr)
    return;

  ULandscapeInfo* LandscapeInfo = LandscapeActor->GetLandscapeInfo();
  if (!LandscapeInfo)
    return;

  UMetaXRAcousticGeometry::FLandscapeMaterial LandscapeMat;
  LandscapeMat.LandscapeInfo = LandscapeInfo;
  LandscapeMat.Materials = MaterialsToApply;
  if (LandscapeMat.Materials.Num() == 0) {
    // use the default acoustic mat properties...
    UMetaXRAcousticMaterialProperties* DefaultAcousticMaterialProp = NewObject<UMetaXRAcousticMaterialProperties>();
    LandscapeMat.Materials.Append(&DefaultAcousticMaterialProp, 1);
  }

  Terrains.Add(LandscapeMat);
}
#pragma endregion

#pragma region HASH_APPENDER
UMetaXRAcousticGeometry::FHashAppender::FHashAppender(const bool bUsePhysicalMaterials, const bool bShouldIncludeChildren)
    : ITransformVisitor(bShouldIncludeChildren), bUsePhysicalMaterials(bUsePhysicalMaterials) {}

TArray<UMetaXRAcousticMaterialProperties*> UMetaXRAcousticGeometry::FHashAppender::VisitActor(
    const AActor* CurrentActor,
    const TArray<UMetaXRAcousticMaterialProperties*>* UserData) {
  // This node should contribute to the hash if it has any relevant components.
  TArray<const UStaticMeshComponent*> MeshComponents;
  CurrentActor->GetComponents<const UStaticMeshComponent>(MeshComponents);
  TArray<UMetaXRAcousticMaterialProperties*> AcousticMaterials;
  const uint32 MaterialCount = GetAcousticMaterials(CurrentActor, AcousticMaterials);
  const ALandscape* LandscapeActor = Cast<ALandscape>(CurrentActor);
  AppendNodeHash(Hash, CurrentActor->GetTransform(), bUsePhysicalMaterials, LandscapeActor, MeshComponents, AcousticMaterials);
  return EmptyAcousticMaterialProps;
}

TArray<UMetaXRAcousticMaterialProperties*> UMetaXRAcousticGeometry::FHashAppender::VisitSceneComponent(
    const USceneComponent* CurrentSceneComponent,
    const TArray<UMetaXRAcousticMaterialProperties*>* UserData) {
  // Only static mesh components contribute to the hash...
  const UStaticMeshComponent* StaticMeshComp = Cast<UStaticMeshComponent>(CurrentSceneComponent);
  if (!StaticMeshComp)
    return EmptyAcousticMaterialProps;

  TArray<UMetaXRAcousticMaterialProperties*> AcousticMaterials;
  const uint32 MaterialCount = GetAcousticMaterials(CurrentSceneComponent, AcousticMaterials);
  AppendNodeHash(Hash, CurrentSceneComponent->GetComponentTransform(), bUsePhysicalMaterials, nullptr, {StaticMeshComp}, AcousticMaterials);
  return EmptyAcousticMaterialProps;
}

FString UMetaXRAcousticGeometry::FHashAppender::GetHash() const {
  return FMD5::HashAnsiString(*Hash);
}
#pragma endregion

#pragma region AGE_CHECKER
// Age checker and IsOlder function can check if the meshes have changed since the last bake
UMetaXRAcousticGeometry::FAgeChecker::FAgeChecker(
    const FDateTime& TimeStamp,
    const bool bUsePhysicalMaterials,
    const bool bShouldIncludeChildren)
    : ITransformVisitor(bShouldIncludeChildren), TimeStamp(TimeStamp), bUsePhysicalMaterials(bUsePhysicalMaterials) {}

TArray<UMetaXRAcousticMaterialProperties*> UMetaXRAcousticGeometry::FAgeChecker::VisitActor(
    const AActor* CurrentActor,
    const TArray<UMetaXRAcousticMaterialProperties*>* UserData) {
  // Check if static mesh was changed since last bake
  TArray<const UStaticMeshComponent*> MeshComponents;
  CurrentActor->GetComponents<const UStaticMeshComponent>(MeshComponents);
  for (const UStaticMeshComponent* MeshComponent : MeshComponents) {
    if (MarkDirtyIfNeeded(MeshComponent->GetStaticMesh()))
      return EmptyAcousticMaterialProps;
  }

  // Check if materials were changed since last bake
  TArray<UMetaXRAcousticMaterialProperties*> AcousticMaterials;
  GetAcousticMaterials(CurrentActor, AcousticMaterials);
  for (const UMetaXRAcousticMaterialProperties* AcousticMat : AcousticMaterials) {
    if (MarkDirtyIfNeeded(AcousticMat))
      return EmptyAcousticMaterialProps;
  }

  const ALandscape* LandscapeActor = Cast<ALandscape>(CurrentActor);
  AppendNodeHash(Hash, CurrentActor->GetTransform(), bUsePhysicalMaterials, LandscapeActor, MeshComponents, AcousticMaterials);
  return EmptyAcousticMaterialProps;
}

TArray<UMetaXRAcousticMaterialProperties*> UMetaXRAcousticGeometry::FAgeChecker::VisitSceneComponent(
    const USceneComponent* CurrentSceneComponent,
    const TArray<UMetaXRAcousticMaterialProperties*>* UserData) {
  if (!CurrentSceneComponent->IsA<UStaticMeshComponent>())
    return UserData == nullptr ? EmptyAcousticMaterialProps : *UserData;

  const UStaticMeshComponent* StaticMeshComp = Cast<UStaticMeshComponent>(CurrentSceneComponent);
  check(StaticMeshComp);
  if (MarkDirtyIfNeeded(StaticMeshComp->GetStaticMesh()))
    return EmptyAcousticMaterialProps;

  TArray<UMetaXRAcousticMaterialProperties*> AcousticMaterials;
  const uint32 NewMaterialCount = GetAcousticMaterials(StaticMeshComp, AcousticMaterials);
  for (const UMetaXRAcousticMaterialProperties* AcousticMat : AcousticMaterials) {
    if (MarkDirtyIfNeeded(AcousticMat))
      return EmptyAcousticMaterialProps;
  }

  AppendNodeHash(Hash, StaticMeshComp->GetComponentTransform(), bUsePhysicalMaterials, nullptr, {StaticMeshComp}, AcousticMaterials);
  return EmptyAcousticMaterialProps;
}

bool UMetaXRAcousticGeometry::FAgeChecker::MarkDirtyIfNeeded(const UObject* Asset) {
  if (!Asset)
    return false;

  if (MetaXRAudioUtilities::HasGameAssetChanged(*Asset, TimeStamp)) {
    bIsOlder = true; // Asset file changed
    return true;
  }
  return false;
}

FString UMetaXRAcousticGeometry::FAgeChecker::GetHash() const {
  return FMD5::HashAnsiString(*Hash);
}

bool UMetaXRAcousticGeometry::FAgeChecker::IsStale() const {
  return bIsOlder;
}
#pragma endregion

#pragma region GIZMO
#if WITH_EDITOR
void FAcousticGeoGizmoDataDeleter::operator()(const FAcousticGeoGizmoData* RawPtr) const {
  if (RawPtr)
    delete RawPtr;
}

void UMetaXRAcousticGeometry::InitGizmoData() {
  // StartInternal should always be called before calling this method.
  check(OvrGeometry);

  if (!GizmoData)
    GizmoData = TUniquePtr<FAcousticGeoGizmoData, FAcousticGeoGizmoDataDeleter>(new FAcousticGeoGizmoData, FAcousticGeoGizmoDataDeleter());

  auto MeshGatherer = FMeshGatherer(false, bUsePhysicalMaterials, bIncludeChildren, LOD);
  TraverseHierarchy(MeshGatherer);
  {
    FScopeLock LockGuard(&GizmoUpdateCS);
    GizmoData->MapGizmoMaterials(MeshGatherer);
    GizmoData->UpdateGizmoMeshData(OvrGeometry);
  }
  MarkRenderTransformDirty();
}

void UMetaXRAcousticGeometry::UpdateGizmoMesh(ovrAudioGeometry GeometryHandle) {
  if (!GizmoData)
    return;
  {
    FScopeLock LockGuard(&GizmoUpdateCS);
    GizmoData->UpdateGizmoMeshData(GeometryHandle);
  }
  // While in Editor and Play Mode this is required to render some gizmos.
  // When AActor with UMetaXRAcousticGeometry does not have a static mesh,
  // UE thinks there is no reason to call FMetaXRAcousticGeometrySceneProxy::GetDynamicMeshElements.
  MarkRenderTransformDirty();
}

int32 UMetaXRAcousticGeometry::GetGizmoVertexCount() const {
  if (!GizmoData)
    return 0;

  return GizmoData->GetGizmoVertexData().Num();
}

bool UMetaXRAcousticGeometry::GetGizmoVertexPositions(TArray<FVector>& GizmoVertices) const {
  GizmoVertices.Empty();
  if (!GizmoData.IsValid())
    return false;

  const FTransform& T = GetComponentTransform();
  const TArray<FDynamicMeshVertex>& GizmoVertexData = GizmoData->GetGizmoVertexData();
  const int32 N = GizmoVertexData.Num();
  GizmoVertices.Reserve(N);
  for (int32 i = 0; i < N; i++) {
    const FDynamicMeshVertex& GizmoMeshVert = GizmoVertexData[i];
    FVector GizmoVertPos(GizmoMeshVert.Position);
    GizmoVertPos = T.TransformPosition(GizmoVertPos);
    GizmoVertices.Add(GizmoVertPos);
  }

  return true;
}

void FAcousticGeoGizmoData::UpdateGizmoMeshData(ovrAudioGeometry GeometryHandle) {
  if (GeometryHandle == nullptr) {
    METAXR_AUDIO_LOG_WARNING("Unable to update gizmo: Geometry not loaded.");
    return;
  }

  uint32_t NumVertices, NumTriangles = 0;
  ovrResult Result = OVRA_CALL(ovrAudio_AudioGeometryGetSimplifiedMeshWithMaterials)(
      GeometryHandle, nullptr, &NumVertices, nullptr, nullptr, &NumTriangles);

  if (Result != ovrSuccess) {
    METAXR_AUDIO_LOG_WARNING("Failed getting simplified mesh from the audio propagation sub-system!");
    return;
  }

  GizmoVertices.SetNum(NumVertices);
  TArray<uint32> IndicesArray;
  IndicesArray.SetNum(NumTriangles * 3);
  TArray<FVector3f> VertexArray;
  VertexArray.SetNum(NumVertices);
  TArray<uint32> MaterialIndexArray;
  MaterialIndexArray.SetNum(NumTriangles);

  Result = OVRA_CALL(ovrAudio_AudioGeometryGetSimplifiedMeshWithMaterials)(
      GeometryHandle, (float*)VertexArray.GetData(), &NumVertices, IndicesArray.GetData(), MaterialIndexArray.GetData(), &NumTriangles);
  if (Result != ovrSuccess) {
    METAXR_AUDIO_LOG_WARNING("Failed getting simplified mesh from the audio propagation sub-system!");
  } else {
    for (size_t i = 0; i < NumVertices; i++)
      GizmoVertices[i] = FDynamicMeshVertex(MetaXRAudioUtilities::ToUEVector3f(VertexArray[i]));
  }

  // Create separate index arrays for each material so they can be uniquely colored
  const int NumMappedMaterials = GizmoMaterialMapping.Num();
  if (NumMappedMaterials <= 0)
    return;

  GizmoMaterialIndices.Empty();
  for (int i = 0; i < IndicesArray.Num(); i += 3) {
    const int MaterialIndex = MaterialIndexArray[i / 3];
    UMetaXRAcousticMaterialProperties* AcousticMaterial =
        MaterialIndex < NumMappedMaterials ? GizmoMaterialMapping[MaterialIndex] : nullptr;

    if (!GizmoMaterialIndices.Contains(AcousticMaterial))
      GizmoMaterialIndices.Add(AcousticMaterial);

    GizmoMaterialIndices[AcousticMaterial].Append(&IndicesArray[i], 3);
  }
}

// Store the materials used so their unique colors can be visualized
void FAcousticGeoGizmoData::MapGizmoMaterials(UMetaXRAcousticGeometry::FMeshGatherer& Gatherer) {
  // Empty any previous mappings
  GizmoMaterialMapping.Empty();
  const int32 LOD = Gatherer.GetLODSelection();

  // lambda for updating GizmoMaterialMapping for each instance
  const auto UpdateGizmoMaterialMappingInstance = [this, LOD](const AcousticMesh& Mesh) {
    int SubMeshCount = 0;
    const UStaticMesh* StaticMesh = Mesh.StaticMesh ? Mesh.StaticMesh->GetStaticMesh() : nullptr;
    if (StaticMesh) {
      const int32 LodGroupToUse = FMath::Clamp(LOD, 0, StaticMesh->GetRenderData()->LODResources.Num() - 1);
      const FStaticMeshLODResources& Model = StaticMesh->GetRenderData()->LODResources[LodGroupToUse];
      SubMeshCount = Model.Sections.Num();
    }

    if (Mesh.Materials.Num() != 0) {
      int Added = 0;
      const int AmountToAdd = FMath::Min(Mesh.Materials.Num(), SubMeshCount);
      for (Added = 0; Added < AmountToAdd; ++Added) {
        GizmoMaterialMapping.Add(Mesh.Materials[Added]);
      }

      // splat the last material on remaining submeshes
      for (Added = AmountToAdd; Added < SubMeshCount; ++Added) {
        GizmoMaterialMapping.Add(Mesh.Materials[Mesh.Materials.Num() - 1]);
      }
    } else {
      for (int i = 0; i < SubMeshCount; ++i) {
        GizmoMaterialMapping.Add(nullptr); // default material
      }
    }
  };

  // Add each mesh's material (or lack of) into the mapping
  for (const auto& Mesh : Gatherer.GetMeshes()) {
    int32 InstanceCount = Mesh.GetInstancedCount();
    do {
      UpdateGizmoMaterialMappingInstance(Mesh);
      InstanceCount--;
    } while (InstanceCount > 0);
  }

  for (const auto& Landscape : Gatherer.GetTerrains())
    GizmoMaterialMapping.Append(Landscape.Materials);
}

const TArray<FDynamicMeshVertex>& FAcousticGeoGizmoData::GetGizmoVertexData() const {
  return GizmoVertices;
}

const TMap<UMetaXRAcousticMaterialProperties*, TArray<uint32>>& FAcousticGeoGizmoData::GetGizmoMaterialData() const {
  return GizmoMaterialIndices;
}

const TArray<UMetaXRAcousticMaterialProperties*>& FAcousticGeoGizmoData::GetGizmoMaterialMapping() const {
  return GizmoMaterialMapping;
}
#endif // end WITH_EDITOR
#pragma endregion
