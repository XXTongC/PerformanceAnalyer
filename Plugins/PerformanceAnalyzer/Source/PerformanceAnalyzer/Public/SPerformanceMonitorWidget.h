#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "PerformanceMonitor.h"

// 前向声明
class SPerformanceChart;

/**
 * 性能监控UI Widget
 */
class SPerformanceMonitorWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPerformanceMonitorWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedPtr<FPerformanceMonitor> InMonitor);
    
	// SWidget interface
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	/** 创建控制按钮 */
	TSharedRef<SWidget> CreateControlButtons();
    
	/** 创建统计信息面板 */
	TSharedRef<SWidget> CreateStatsPanel();
    
	/** 创建图表面板 */
	TSharedRef<SWidget> CreateChartPanel();
    
	/** 按钮回调 */
	FText GetMonitorButtonText() const;
	FReply OnToggleMonitoring();
	FReply OnClearHistory();
    
	/** 获取显示文本 */
	FText GetFPSText() const;
	FSlateColor GetFPSColor() const;
	FText GetFrameTimeText() const;
	FText GetGPUTimeText() const;
	FText GetDrawCallsText() const;
	FText GetTrianglesText() const;
	FText GetMemoryText() const;

private:
	TSharedPtr<FPerformanceMonitor> PerformanceMonitor;
	TSharedPtr<SPerformanceChart> ChartWidget;
	FPerformanceSample LatestSample;
	TArray<FPerformanceSample> HistorySamples;
};