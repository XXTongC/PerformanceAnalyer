#pragma once

#include "CoreMinimal.h"
#include "Tickable.h"
#include "PerformanceMonitor.generated.h"

/**
 * 性能采样数据点
 */
USTRUCT(BlueprintType)
struct FPerformanceSample
{
    GENERATED_BODY()
    
    UPROPERTY(BlueprintReadOnly, Category = "Performance")
    float Timestamp = 0.0f;
    
    UPROPERTY(BlueprintReadOnly, Category = "Performance")
    float FrameTimeMS = 0.0f;
    
    UPROPERTY(BlueprintReadOnly, Category = "Performance")
    float GPUTimeMS = 0.0f;
    
    UPROPERTY(BlueprintReadOnly, Category = "Performance")
    float RenderThreadTimeMS = 0.0f;
    
    UPROPERTY(BlueprintReadOnly, Category = "Performance")
    float GameThreadTimeMS = 0.0f;
    
    UPROPERTY(BlueprintReadOnly, Category = "Performance")
    int32 DrawCalls = 0;
    
    UPROPERTY(BlueprintReadOnly, Category = "Performance")
    int32 Primitives = 0;
    
    UPROPERTY(BlueprintReadOnly, Category = "Performance")
    int32 Triangles = 0;
    
    UPROPERTY(BlueprintReadOnly, Category = "Performance")
    float MemoryUsedMB = 0.0f;
    
    UPROPERTY(BlueprintReadOnly, Category = "Performance")
    bool bIsEstimated = false;
};

/**
 * 数据源类型
 */
UENUM()
enum class EPerformanceDataSource : uint8
{
    RHI,
    Engine,
    Estimated
};

/**
 * 实时性能监控器
 */
class PERFORMANCEANALYZER_API FPerformanceMonitor : public FTickableGameObject
{
public:
    FPerformanceMonitor();
    virtual ~FPerformanceMonitor();
    
    // FTickableGameObject interface
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickable() const override { return bIsEnabled; }
    
    /** 启用/禁用监控 */
    void SetEnabled(bool bEnabled);
    bool IsEnabled() const { return bIsEnabled; }
    
    /** 获取最新的性能数据 */
    FPerformanceSample GetLatestSample() const;
    
    /** 获取历史数据 */
    TArray<FPerformanceSample> GetHistorySamples(float Seconds = 60.0f) const;
    
    /** 清除历史数据 */
    void ClearHistory();
    
    /** 获取统计信息 */
    float GetAverageFPS() const;
    float GetAverageFrameTime() const;
    int32 GetAverageDrawCalls() const;
    
    /** 获取当前数据源类型 */
    EPerformanceDataSource GetCurrentDataSource() const { return CurrentDataSource; }
    
private:
    /** 采样性能数据 */
    void SamplePerformance();
    
    /** Level 1: 尝试从RHI获取数据 */
    bool TryGetRHIStats(FPerformanceSample& OutSample);
    
    /** Level 2: 尝试从Engine统计获取数据 */
    bool TryGetEngineStats(FPerformanceSample& OutSample);
    
    /** Level 3: 场景分析估算 */
    void GetEstimatedStats(FPerformanceSample& OutSample);
    
    /** 辅助函数：计算场景三角形数 */
    int32 CalculateSceneTriangles();
    
    /** 辅助函数：计算场景Primitive数 */
    int32 CalculateScenePrimitives();
    
    
    void RequestGPUTime();
private:
    bool bIsEnabled;
    TArray<FPerformanceSample> HistorySamples;
    static const int32 MaxHistorySamples = 3600;
    int32 CurrentSampleIndex;
    float AccumulatedTime;
    float SampleInterval;
    EPerformanceDataSource CurrentDataSource;   
    std::atomic<float> CachedGPUTimeMS;
    std::atomic<bool> bGPUTimeValid;
};