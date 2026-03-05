#include "SPerformanceChart.h"
#include "Rendering/DrawElements.h"

void SPerformanceChart::Construct(const FArguments& InArgs)
{
}

void SPerformanceChart::SetHistorySamples(const TArray<FPerformanceSample>& InSamples)
{
    HistorySamples = InSamples;
}

FVector2D SPerformanceChart::ComputeDesiredSize(float) const
{
    return FVector2D(400.0f, 200.0f);
}

int32 SPerformanceChart::OnPaint(
    const FPaintArgs& Args,
    const FGeometry& AllottedGeometry,
    const FSlateRect& MyCullingRect,
    FSlateWindowElementList& OutDrawElements,
    int32 LayerId,
    const FWidgetStyle& InWidgetStyle,
    bool bParentEnabled) const
{
    // 绘制背景
    FSlateDrawElement::MakeBox(
        OutDrawElements,
        LayerId,
        AllottedGeometry.ToPaintGeometry(),
        FCoreStyle::Get().GetBrush("WhiteBrush"),
        ESlateDrawEffect::None,
        FLinearColor(0.02f, 0.02f, 0.02f, 1.0f)
    );
    
    if (HistorySamples.Num() < 2)
    {
        return LayerId + 1;
    }
    
    const FVector2D Size = AllottedGeometry.GetLocalSize();
    const float Width = Size.X;
    const float Height = Size.Y;
    
    // 找到最大帧时间用于缩放
    float MaxFrameTime = 33.33f; // 默认30fps
    for (const FPerformanceSample& Sample : HistorySamples)
    {
        MaxFrameTime = FMath::Max(MaxFrameTime, Sample.FrameTimeMS);
    }
    
    // 绘制参考线（60fps = 16.67ms, 30fps = 33.33ms）
    const float Line60FPS = Height * (1.0f - 16.67f / MaxFrameTime);
    const float Line30FPS = Height * (1.0f - 33.33f / MaxFrameTime);
    
    // 60fps参考线（绿色）
    TArray<FVector2D> Line60Points;
    Line60Points.Add(FVector2D(0, Line60FPS));
    Line60Points.Add(FVector2D(Width, Line60FPS));
    
    FSlateDrawElement::MakeLines(
        OutDrawElements,
        LayerId + 1,
        AllottedGeometry.ToPaintGeometry(),
        Line60Points,
        ESlateDrawEffect::None,
        FLinearColor(0.0f, 1.0f, 0.0f, 0.3f),
        true,
        1.0f
    );
    
    // 30fps参考线（黄色）
    TArray<FVector2D> Line30Points;
    Line30Points.Add(FVector2D(0, Line30FPS));
    Line30Points.Add(FVector2D(Width, Line30FPS));
    
    FSlateDrawElement::MakeLines(
        OutDrawElements,
        LayerId + 2,
        AllottedGeometry.ToPaintGeometry(),
        Line30Points,
        ESlateDrawEffect::None,
        FLinearColor(1.0f, 1.0f, 0.0f, 0.3f),
        true,
        1.0f
    );
    
    // 绘制帧时间曲线
    TArray<FVector2D> LinePoints;
    LinePoints.Reserve(HistorySamples.Num());
    
    for (int32 i = 0; i < HistorySamples.Num(); ++i)
    {
        const FPerformanceSample& Sample = HistorySamples[i];
        
        float X = (float)i / (float)(HistorySamples.Num() - 1) * Width;
        float Y = Height * (1.0f - FMath::Clamp(Sample.FrameTimeMS / MaxFrameTime, 0.0f, 1.0f));
        
        LinePoints.Add(FVector2D(X, Y));
    }
    
    // 根据最新性能选择颜色
    FLinearColor LineColor = FLinearColor::Green;
    if (HistorySamples.Num() > 0)
    {
        const FPerformanceSample& LatestSample = HistorySamples.Last();
        if (LatestSample.FrameTimeMS > 33.33f)
        {
            LineColor = FLinearColor::Red; // < 30fps
        }
        else if (LatestSample.FrameTimeMS > 16.67f)
        {
            LineColor = FLinearColor::Yellow; // 30-60fps
        }
    }
    
    FSlateDrawElement::MakeLines(
        OutDrawElements,
        LayerId + 3,
        AllottedGeometry.ToPaintGeometry(),
        LinePoints,
        ESlateDrawEffect::None,
        LineColor,
        true,
        2.0f
    );
    
    return LayerId + 4;
}