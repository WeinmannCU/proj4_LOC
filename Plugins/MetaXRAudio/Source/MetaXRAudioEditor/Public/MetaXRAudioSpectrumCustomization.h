// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
#pragma once

#include "DetailWidgetRow.h"
#include "IPropertyTypeCustomization.h"
#include "PropertyHandle.h"

/**
 * Used to Render FMetaXRAudioSpectrum in Details panel
 */
class METAXRAUDIOEDITOR_API FMetaXRAudioSpectrumCustomization final : public IPropertyTypeCustomization {
 public:
  static TSharedRef<IPropertyTypeCustomization> MakeInstance();

  void CustomizeHeader(
      TSharedRef<IPropertyHandle> PropertyHandle,
      FDetailWidgetRow& HeaderRow,
      IPropertyTypeCustomizationUtils& CustomizationUtils) final;
  void CustomizeChildren(
      TSharedRef<IPropertyHandle> PropertyHandle,
      IDetailChildrenBuilder& ChildBuilder,
      IPropertyTypeCustomizationUtils& CustomizationUtils) final;

 private:
  // Use PreNotifyModifications before changes to data and PostNotifyModifications after changes to data.
  // These methods are used for data saving purposes. Otherwise, changes and be lost.
  void PreNotifyModifications();
  void PostNotifyModifications(const bool RefreshLayout = false);

  // Below methods are just convenienve methods for getting and updating data in spectrum.
  struct FMetaXRAudioSpectrum* GetSpectrumData() const;
  const float GetFrequencyData(const uint32 PointIndex) const;
  const bool UpdateFrequencyData(const uint32 PointIndex, const float NewFreq);
  const float GetRangeData(const uint32 PointIndex) const;
  const bool UpdateRangeData(const uint32 PointIndex, const float NewRangeValue, const FVector2D& RangeClamp);

 private:
  IPropertyHandle* PropertyHandlePtr;
  class IDetailLayoutBuilder* LayoutBuilder = nullptr;
  // the UObject being modified
  TWeakObjectPtr<UObject> UObjPtr;
};
