// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
#include "MetaXRAudioSpectrumCustomization.h"
#include "DetailLayoutBuilder.h"
#include "IDetailChildrenBuilder.h"
#include "IDetailGroup.h"
#include "Misc/EngineVersionComparison.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SNumericEntryBox.h"

#include "MetaXRAcousticControlZone.h"
#include "MetaXRAudioSpectrum.h"
#include "SpectrumWidget.h"

struct FAudioSpectrumMetadata {
  static const FName SpectrumRangeMinMetadataKey;
  static const FName SpectrumRangeMaxMetadataKey;
  static const FName SpectrumGraphColorMetadataKey;
  static const FName SpectrumRangeLabelMetadataKey;

  FVector2D RangeClamp;
  FLinearColor GraphColor;
  FText RangeLabelText;

  // Extracts the metadata from UPROPERTY
  static FAudioSpectrumMetadata GetSpectrumMetadata(TSharedRef<IPropertyHandle> PropertyHandle) {
    FAudioSpectrumMetadata OutMetadata;

    OutMetadata.RangeClamp.X = -1.0;
    if (PropertyHandle->HasMetaData(SpectrumRangeMinMetadataKey))
      OutMetadata.RangeClamp.X = PropertyHandle->GetFloatMetaData(SpectrumRangeMinMetadataKey);

    OutMetadata.RangeClamp.Y = -1.0;
    if (PropertyHandle->HasMetaData(SpectrumRangeMaxMetadataKey))
      OutMetadata.RangeClamp.Y = PropertyHandle->GetFloatMetaData(SpectrumRangeMaxMetadataKey);

    OutMetadata.GraphColor = FLinearColor(0.5f, 0.5f, 0.5f);
    if (PropertyHandle->HasMetaData(SpectrumGraphColorMetadataKey)) {
      const FString& GraphColorMetaDataStr = PropertyHandle->GetMetaData(SpectrumGraphColorMetadataKey);
      OutMetadata.GraphColor.InitFromString(GraphColorMetaDataStr);
    }

    if (PropertyHandle->HasMetaData(SpectrumRangeLabelMetadataKey)) {
      const FString RangeLabelStr = PropertyHandle->GetMetaData(SpectrumRangeLabelMetadataKey);
      OutMetadata.RangeLabelText = FText::FromString(RangeLabelStr);
    }

    return OutMetadata;
  }
};

const FName FAudioSpectrumMetadata::SpectrumRangeMinMetadataKey("SpectrumRangeMin");
const FName FAudioSpectrumMetadata::SpectrumRangeMaxMetadataKey("SpectrumRangeMax");
const FName FAudioSpectrumMetadata::SpectrumGraphColorMetadataKey("SpectrumGraphColor");
const FName FAudioSpectrumMetadata::SpectrumRangeLabelMetadataKey("SpectrumRangeLabel");

TSharedRef<IPropertyTypeCustomization> FMetaXRAudioSpectrumCustomization::MakeInstance() {
  return MakeShareable(new FMetaXRAudioSpectrumCustomization);
}

void FMetaXRAudioSpectrumCustomization::CustomizeHeader(
    TSharedRef<IPropertyHandle> PropertyHandle,
    FDetailWidgetRow& HeaderRow,
    IPropertyTypeCustomizationUtils& CustomizationUtils) {
  FDetailWidgetDecl& RowName = HeaderRow.NameContent();
  RowName[PropertyHandle->CreatePropertyNameWidget()];
}

void FMetaXRAudioSpectrumCustomization::CustomizeChildren(
    TSharedRef<IPropertyHandle> PropertyHandle,
    IDetailChildrenBuilder& ChildBuilder,
    IPropertyTypeCustomizationUtils& CustomizationUtils) {
  // Set member pointers
  this->PropertyHandlePtr = &*PropertyHandle;

  IDetailCategoryBuilder& CategoryBuilder = ChildBuilder.GetParentCategory();
  IDetailLayoutBuilder& LayoutBuilderRef = CategoryBuilder.GetParentLayout();
  this->LayoutBuilder = &LayoutBuilderRef;
  TArray<TWeakObjectPtr<UObject>> OutObjects;
  LayoutBuilderRef.GetObjectsBeingCustomized(OutObjects);
  if (OutObjects.Num() != 1)
    return;

  this->UObjPtr = OutObjects[0];

  // get spectrum data for initialization purposes
  FMetaXRAudioSpectrum* SpectrumData = GetSpectrumData();
  if (!SpectrumData)
    return;

  // Get metadata values of UPROPERTY
  const FAudioSpectrumMetadata SpectrumMetadata = FAudioSpectrumMetadata::GetSpectrumMetadata(PropertyHandle);

  // Create & Init Widgets with spectrum data
  TSharedRef<SSpectrumWidget> GraphWidget = SNew(SSpectrumWidget)
                                                .Spectrum(SpectrumData)
                                                .GraphColor(SpectrumMetadata.GraphColor)
                                                .Scale(EAxisScale::SquareCentered)
                                                .RangeMin(SpectrumMetadata.RangeClamp.X)
                                                .RangeMax(SpectrumMetadata.RangeClamp.Y);
  GraphWidget->SetPropertyHandle(PropertyHandle);

  FDetailWidgetRow& SpectrumWidgetRow = ChildBuilder.AddCustomRow(FText::FromString("Spectrum"));
  SpectrumWidgetRow.WholeRowContent()[SNew(SBox).HeightOverride(120).WidthOverride(500)[GraphWidget]];

  IDetailGroup& SpecPointsGroup = ChildBuilder.AddGroup("Spectrum Points", FText::FromString("Points"));
  SpecPointsGroup.AddWidgetRow()
      .NameContent()
          [SNew(STextBlock).Text(FText::FromString("Size")).ToolTipText(FText::FromString("The number of points on the spectrum"))]
      .ValueContent()[SNew(SNumericEntryBox<int32>)
                          .Value_Lambda([this]() -> int32 {
                            const FMetaXRAudioSpectrum* SpectrumData = GetSpectrumData();
                            if (!SpectrumData)
                              return 0;
                            return SpectrumData->PointCount();
                          })
                          .OnValueChanged_Lambda([this](int32 Value) {
                            FMetaXRAudioSpectrum* SpectrumData = GetSpectrumData();
                            if (!SpectrumData)
                              return;

                            PreNotifyModifications();
                            SpectrumData->Resize(Value);
                            PostNotifyModifications(true);
                          })
                          .MinValue(TNumericLimits<int32>::Lowest())
                          .MaxValue(TNumericLimits<int32>::Max())];

  const FText DomainLabel = FText::FromString("Frequency");
  SpecPointsGroup.AddWidgetRow()
      .NameContent()[SNew(STextBlock).Text(DomainLabel)] // Domain Label
      .ValueContent()[SNew(STextBlock).Text(SpectrumMetadata.RangeLabelText)]; // Range Label

  const TArray<FMetaXRAudioPoint>& SpectrumPoints = SpectrumData->Points;
  for (int32 PointIndex = 0; PointIndex < SpectrumPoints.Num(); ++PointIndex) {
    SpecPointsGroup.AddWidgetRow()
        .NameContent()[SNew(SNumericEntryBox<float>)
#if !UE_VERSION_OLDER_THAN(5, 2, 0)
                           .MinFractionalDigits(0)
#endif
                           .Value_Lambda([this, PointIndex]() { return GetFrequencyData(PointIndex); })
                           .OnValueChanged_Lambda([this, PointIndex](float Value) {
                             PreNotifyModifications();
                             UpdateFrequencyData(PointIndex, Value);
                             PostNotifyModifications(false);
                           })
                           .MinValue(TNumericLimits<float>::Lowest())
                           .MaxValue(TNumericLimits<float>::Max())
                           .Delta(1)]
        .ValueContent()[SNew(SNumericEntryBox<float>)
                            .Value_Lambda([this, PointIndex]() { return GetRangeData(PointIndex); })
                            .OnValueChanged_Lambda([this, SpectrumMetadata, PointIndex](float Value) {
                              PreNotifyModifications();
                              UpdateRangeData(PointIndex, Value, SpectrumMetadata.RangeClamp);
                              PostNotifyModifications(false);
                            })
                            .MinValue(TNumericLimits<float>::Lowest())
                            .MaxValue(TNumericLimits<float>::Max())
                            .Delta(1)]
        .ExtensionContent()[SNew(SButton).Text(FText::FromString("del")).OnClicked_Lambda([this, PointIndex]() {
          PreNotifyModifications();

          FMetaXRAudioSpectrum* SpectrumData = GetSpectrumData();
          if (!SpectrumData)
            return FReply::Unhandled();

          SpectrumData->RemovePoint(PointIndex);
          PostNotifyModifications();
          return FReply::Handled();
        })];
  }
}

void FMetaXRAudioSpectrumCustomization::PreNotifyModifications() {
  this->UObjPtr->Modify();
  this->PropertyHandlePtr->NotifyPreChange();
}

void FMetaXRAudioSpectrumCustomization::PostNotifyModifications(const bool RefreshLayout) {
  this->PropertyHandlePtr->NotifyPostChange(EPropertyChangeType::Unspecified);
  this->UObjPtr->PostEditChange();

  if (RefreshLayout)
    this->LayoutBuilder->ForceRefreshDetails();
}

FMetaXRAudioSpectrum* FMetaXRAudioSpectrumCustomization::GetSpectrumData() const {
  if (!this->PropertyHandlePtr)
    return nullptr;

  TArray<void*> RawData;
  this->PropertyHandlePtr->AccessRawData(RawData);
  if (RawData.Num() <= 0)
    return nullptr;

  return static_cast<FMetaXRAudioSpectrum*>(RawData[0]);
}

const float FMetaXRAudioSpectrumCustomization::GetFrequencyData(const uint32 PointIndex) const {
  const FMetaXRAudioSpectrum* SpectrumData = GetSpectrumData();
  if (!SpectrumData)
    return 0.0f;

  return SpectrumData->GetFrequency(PointIndex);
}

const bool FMetaXRAudioSpectrumCustomization::UpdateFrequencyData(const uint32 PointIndex, const float NewFreq) {
  FMetaXRAudioSpectrum* SpectrumData = GetSpectrumData();
  if (!SpectrumData)
    return false;

  SpectrumData->UpdateFrequencyValue(PointIndex, NewFreq);
  return true;
}

const float FMetaXRAudioSpectrumCustomization::GetRangeData(const uint32 PointIndex) const {
  const FMetaXRAudioSpectrum* SpectrumData = GetSpectrumData();
  if (!SpectrumData)
    return 0.0f;

  return SpectrumData->GetData(PointIndex);
}

const bool
FMetaXRAudioSpectrumCustomization::UpdateRangeData(const uint32 PointIndex, const float NewRangeValue, const FVector2D& RangeClamp) {
  FMetaXRAudioSpectrum* SpectrumData = GetSpectrumData();
  if (!SpectrumData)
    return false;

  SpectrumData->UpdateDataValue(PointIndex, NewRangeValue, RangeClamp);
  return true;
}
