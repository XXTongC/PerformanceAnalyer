#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"
#include "PerformanceMonitor.h"

/**
 * 自定义性能图表Widget
 */
class SPerformanceChart : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SPerformanceChart) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
    
	/** 设置要显示的历史数据 */
	void SetHistorySamples(const TArray<FPerformanceSample>& InSamples);
    
	// SWidget interface
	virtual int32 OnPaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;
    
	virtual FVector2D ComputeDesiredSize(float) const override;

private:
	TArray<FPerformanceSample> HistorySamples;
};