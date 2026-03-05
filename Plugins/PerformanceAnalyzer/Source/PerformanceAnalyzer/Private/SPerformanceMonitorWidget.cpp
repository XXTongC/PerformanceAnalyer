#include "SPerformanceMonitorWidget.h"
#include "SPerformanceChart.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "PerformanceMonitor"

void SPerformanceMonitorWidget::Construct(const FArguments& InArgs, TSharedPtr<FPerformanceMonitor> InMonitor)
{
    PerformanceMonitor = InMonitor;
    
    ChildSlot
    [
        SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
        .Padding(8.0f)
        [
            SNew(SVerticalBox)
            
            // 标题
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 0, 0, 10)
            [
                SNew(STextBlock)
                .Text(LOCTEXT("Title", "Real-Time Performance Monitor"))
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
                .Justification(ETextJustify::Center)
            ]
            
            // 控制按钮
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 0, 0, 10)
            [
                CreateControlButtons()
            ]
            
            // 统计信息面板
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 0, 0, 10)
            [
                CreateStatsPanel()
            ]
            
            // 图表面板
            + SVerticalBox::Slot()
            .FillHeight(1.0f)
            [
                CreateChartPanel()
            ]
        ]
    ];
}

TSharedRef<SWidget> SPerformanceMonitorWidget::CreateControlButtons()
{
    return SNew(SHorizontalBox)
        
        // 启动/停止按钮
                // 启动/停止按钮
        + SHorizontalBox::Slot()
        .AutoWidth()
        .Padding(5, 0)
        [
            SNew(SButton)
            .Text(this, &SPerformanceMonitorWidget::GetMonitorButtonText)
            .OnClicked(this, &SPerformanceMonitorWidget::OnToggleMonitoring)
            .HAlign(HAlign_Center)
            .VAlign(VAlign_Center)
        ]
        
        // 清除历史按钮
        + SHorizontalBox::Slot()
        .AutoWidth()
        .Padding(5, 0)
        [
            SNew(SButton)
            .Text(LOCTEXT("ClearHistory", "Clear History"))
            .OnClicked(this, &SPerformanceMonitorWidget::OnClearHistory)
            .HAlign(HAlign_Center)
            .VAlign(VAlign_Center)
        ];
}

TSharedRef<SWidget> SPerformanceMonitorWidget::CreateStatsPanel()
{
    return SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
        .Padding(10.0f)
        [
            SNew(SGridPanel)
            .FillColumn(1, 1.0f)
            
            // FPS
            + SGridPanel::Slot(0, 0)
            .Padding(5)
            [
                SNew(STextBlock)
                .Text(LOCTEXT("FPS", "FPS:"))
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
            ]
            + SGridPanel::Slot(1, 0)
            .Padding(5)
            [
                SNew(STextBlock)
                .Text(this, &SPerformanceMonitorWidget::GetFPSText)
                .ColorAndOpacity(this, &SPerformanceMonitorWidget::GetFPSColor)
                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
            ]
            
            // Frame Time
            + SGridPanel::Slot(0, 1)
            .Padding(5)
            [
                SNew(STextBlock)
                .Text(LOCTEXT("FrameTime", "Frame Time:"))
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
            ]
            + SGridPanel::Slot(1, 1)
            .Padding(5)
            [
                SNew(STextBlock)
                .Text(this, &SPerformanceMonitorWidget::GetFrameTimeText)
                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
            ]
            
            // GPU Time
            + SGridPanel::Slot(0, 2)
            .Padding(5)
            [
                SNew(STextBlock)
                .Text(LOCTEXT("GPUTime", "GPU Time:"))
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
            ]
            + SGridPanel::Slot(1, 2)
            .Padding(5)
            [
                SNew(STextBlock)
                .Text(this, &SPerformanceMonitorWidget::GetGPUTimeText)
                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
            ]
            
            // DrawCalls
            + SGridPanel::Slot(0, 3)
            .Padding(5)
            [
                SNew(STextBlock)
                .Text(LOCTEXT("DrawCalls", "Draw Calls:"))
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
            ]
            + SGridPanel::Slot(1, 3)
            .Padding(5)
            [
                SNew(STextBlock)
                .Text(this, &SPerformanceMonitorWidget::GetDrawCallsText)
                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
            ]
            
            // Triangles
            + SGridPanel::Slot(0, 4)
            .Padding(5)
            [
                SNew(STextBlock)
                .Text(LOCTEXT("Triangles", "Triangles:"))
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
            ]
            + SGridPanel::Slot(1, 4)
            .Padding(5)
            [
                SNew(STextBlock)
                .Text(this, &SPerformanceMonitorWidget::GetTrianglesText)
                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
            ]
            
            // Memory
            + SGridPanel::Slot(0, 5)
            .Padding(5)
            [
                SNew(STextBlock)
                .Text(LOCTEXT("Memory", "Memory:"))
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
            ]
            + SGridPanel::Slot(1, 5)
            .Padding(5)
            [
                SNew(STextBlock)
                .Text(this, &SPerformanceMonitorWidget::GetMemoryText)
                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
            ]
        ];
}

TSharedRef<SWidget> SPerformanceMonitorWidget::CreateChartPanel()
{
    return SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
        .Padding(10.0f)
        [
            SNew(SBox)
            .HeightOverride(200.0f)
            [
                SAssignNew(ChartWidget, SPerformanceChart)
            ]
        ];
}

void SPerformanceMonitorWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
    SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
    
    if (PerformanceMonitor.IsValid() && PerformanceMonitor->IsEnabled())
    {
        // 更新最新样本
        LatestSample = PerformanceMonitor->GetLatestSample();
        
        // 更新历史样本（用于图表）
        HistorySamples = PerformanceMonitor->GetHistorySamples(60.0f);
        
        // 更新图表数据
        if (ChartWidget.IsValid())
        {
            ChartWidget->SetHistorySamples(HistorySamples);
        }
    }
}

FText SPerformanceMonitorWidget::GetMonitorButtonText() const
{
    if (PerformanceMonitor.IsValid() && PerformanceMonitor->IsEnabled())
    {
        return LOCTEXT("StopMonitoring", "Stop Monitoring");
    }
    return LOCTEXT("StartMonitoring", "Start Monitoring");
}

FReply SPerformanceMonitorWidget::OnToggleMonitoring()
{
    if (PerformanceMonitor.IsValid())
    {
        PerformanceMonitor->SetEnabled(!PerformanceMonitor->IsEnabled());
    }
    return FReply::Handled();
}

FReply SPerformanceMonitorWidget::OnClearHistory()
{
    if (PerformanceMonitor.IsValid())
    {
        PerformanceMonitor->ClearHistory();
        HistorySamples.Empty();
        
        if (ChartWidget.IsValid())
        {
            ChartWidget->SetHistorySamples(HistorySamples);
        }
    }
    return FReply::Handled();
}

FText SPerformanceMonitorWidget::GetFPSText() const
{
    if (LatestSample.FrameTimeMS > 0.0f)
    {
        float FPS = 1000.0f / LatestSample.FrameTimeMS;
        return FText::Format(LOCTEXT("FPSValue", "{0}"), FText::AsNumber(FMath::RoundToInt(FPS)));
    }
    return LOCTEXT("NoData", "N/A");
}

FSlateColor SPerformanceMonitorWidget::GetFPSColor() const
{
    if (LatestSample.FrameTimeMS > 0.0f)
    {
        float FPS = 1000.0f / LatestSample.FrameTimeMS;
        
        if (FPS >= 60.0f)
        {
            return FSlateColor(FLinearColor::Green);
        }
        else if (FPS >= 30.0f)
        {
            return FSlateColor(FLinearColor::Yellow);
        }
        else
        {
            return FSlateColor(FLinearColor::Red);
        }
    }
    return FSlateColor(FLinearColor::White);
}

FText SPerformanceMonitorWidget::GetFrameTimeText() const
{
    return FText::Format(
        LOCTEXT("FrameTimeValue", "{0} ms"),
        FText::AsNumber(FMath::RoundToFloat(LatestSample.FrameTimeMS * 10.0f) / 10.0f)
    );
}

FText SPerformanceMonitorWidget::GetGPUTimeText() const
{
    FString Suffix = LatestSample.bIsEstimated ? TEXT(" (Est)") : TEXT("");
    return FText::Format(
        LOCTEXT("GPUTimeValue", "{0} ms{1}"),
        FText::AsNumber(FMath::RoundToFloat(LatestSample.GPUTimeMS * 10.0f) / 10.0f),
        FText::FromString(Suffix)
    );
}

FText SPerformanceMonitorWidget::GetDrawCallsText() const
{
    return FText::Format(
        LOCTEXT("DrawCallsValue", "{0}"),
        FText::AsNumber(LatestSample.DrawCalls)
    );
}

FText SPerformanceMonitorWidget::GetTrianglesText() const
{
    // 格式化三角形数（使用千位分隔符）
    FNumberFormattingOptions Options;
    Options.UseGrouping = true;
    
    return FText::Format(
        LOCTEXT("TrianglesValue", "{0}"),
        FText::AsNumber(LatestSample.Triangles, &Options)
    );
}

FText SPerformanceMonitorWidget::GetMemoryText() const
{
    return FText::Format(
        LOCTEXT("MemoryValue", "{0} MB"),
        FText::AsNumber(FMath::RoundToFloat(LatestSample.MemoryUsedMB * 10.0f) / 10.0f)
    );
}

#undef LOCTEXT_NAMESPACE