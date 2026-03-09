#include "PerformanceMonitor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "RenderingThread.h"
#include "RHI.h"
#include "RHICommandList.h"
#include "HAL/PlatformMemory.h"
#include "EngineUtils.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UnrealClient.h"

FPerformanceMonitor::FPerformanceMonitor()
    : bIsEnabled(false)
    , CurrentSampleIndex(0)
    , AccumulatedTime(0.0f)
    , SampleInterval(1.0f / 60.0f)
    , CurrentDataSource(EPerformanceDataSource::Estimated)
{
    HistorySamples.Reserve(MaxHistorySamples);
}

FPerformanceMonitor::~FPerformanceMonitor()
{
}

TStatId FPerformanceMonitor::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(FPerformanceMonitor, STATGROUP_Tickables);
}

void FPerformanceMonitor::Tick(float DeltaTime)
{
    if (!bIsEnabled)
    {
        return;
    }
    
    AccumulatedTime += DeltaTime;
    
    if (AccumulatedTime >= SampleInterval)
    {
        SamplePerformance();
        AccumulatedTime = 0.0f;
    }
}

void FPerformanceMonitor::SetEnabled(bool bEnabled)
{
    bIsEnabled = bEnabled;
    
    if (bEnabled)
    {
        UE_LOG(LogTemp, Log, TEXT("Performance Monitor: Enabled"));
        ClearHistory();
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("Performance Monitor: Disabled"));
    }
}

void FPerformanceMonitor::SamplePerformance()
{
    FPerformanceSample Sample;
    Sample.Timestamp = FPlatformTime::Seconds();
    
    // 三级数据获取策略
    if (TryGetRHIStats(Sample))
    {
        CurrentDataSource = EPerformanceDataSource::RHI;
        Sample.bIsEstimated = false;
    }
    else if (TryGetEngineStats(Sample))
    {
        CurrentDataSource = EPerformanceDataSource::Engine;
        Sample.bIsEstimated = false;
    }
    else
    {
        GetEstimatedStats(Sample);
        CurrentDataSource = EPerformanceDataSource::Estimated;
        Sample.bIsEstimated = true;
    }
    
    // 添加到历史记录
    if (HistorySamples.Num() < MaxHistorySamples)
    {
        HistorySamples.Add(Sample);
    }
    else
    {
        HistorySamples[CurrentSampleIndex] = Sample;
        CurrentSampleIndex = (CurrentSampleIndex + 1) % MaxHistorySamples;
    }
}

void FPerformanceMonitor::RequestGPUTime()
{
    ENQUEUE_RENDER_COMMAND(GetGPUFrameTime)(
        [this](FRHICommandListImmediate& RHICmdList)
        {
            uint32 GPUCycles = RHIGetGPUFrameCycles();
            if (GPUCycles > 0)
            {
                CachedGPUTimeMS.store(
                    FPlatformTime::ToMilliseconds(GPUCycles)
                );
                bGPUTimeValid.store(true);
            }
        }
    );
}
    

bool FPerformanceMonitor::TryGetRHIStats(FPerformanceSample& OutSample)
{
    // Level 1: 从RHI获取最准确的数据
    
#if STATS
        
    OutSample.DrawCalls = GNumDrawCallsRHI[0];
    OutSample.Primitives = GNumPrimitivesDrawnRHI[0];
        
    // 使用缓存的GPU时间（无阻塞）
    if (bGPUTimeValid.load())
    {
        OutSample.GPUTimeMS = CachedGPUTimeMS.load();
    }
    else
    {
        OutSample.GPUTimeMS = FApp::GetDeltaTime() * 1000.0f;
    }
        
    // 在后台请求下一帧的GPU时间
    RequestGPUTime();
        
    return (OutSample.DrawCalls > 0 || OutSample.Primitives > 0);
        
#else
    return false;
#endif
}

bool FPerformanceMonitor::TryGetEngineStats(FPerformanceSample& OutSample)
{
    // Level 2: 从Engine统计系统获取数据
    
    if (!GEngine)
    {
        return false;
    }
    
    // 获取FPS和帧时间
    float AverageFPS = GetAverageFPS();
    if (AverageFPS > 0.0f)
    {
        OutSample.FrameTimeMS = 1000.0f / AverageFPS;
    }
    else
    {
        OutSample.FrameTimeMS = FApp::GetDeltaTime() * 1000.0f;
    }
    
    // GPU时间（使用帧时间作为近似）
    OutSample.GPUTimeMS = OutSample.FrameTimeMS;
    
    // 线程时间
    OutSample.GameThreadTimeMS = FPlatformTime::ToMilliseconds(GGameThreadTime);
    OutSample.RenderThreadTimeMS = FPlatformTime::ToMilliseconds(GRenderThreadTime);
    
    // DrawCall和Primitive（从场景估算）
    OutSample.Primitives = CalculateScenePrimitives();
    OutSample.DrawCalls = OutSample.Primitives; // 粗略估计
    
    // 三角形数
    OutSample.Triangles = CalculateSceneTriangles();
    
        // 内存使用
    FPlatformMemoryStats MemStats = FPlatformMemory::GetStats();
    OutSample.MemoryUsedMB = MemStats.UsedPhysical / (1024.0f * 1024.0f);
    
    // 如果能获取到有效的FPS，认为Engine统计可用
    return (AverageFPS > 0.0f);
}

void FPerformanceMonitor::GetEstimatedStats(FPerformanceSample& OutSample)
{
    // Level 3: 场景分析估算（最后的回退方案）
    
    // 帧时间
    OutSample.FrameTimeMS = FApp::GetDeltaTime() * 1000.0f;
    
    // GPU时间（使用帧时间）
    OutSample.GPUTimeMS = OutSample.FrameTimeMS;
    
    // 线程时间
    OutSample.GameThreadTimeMS = FPlatformTime::ToMilliseconds(GGameThreadTime);
    OutSample.RenderThreadTimeMS = FPlatformTime::ToMilliseconds(GRenderThreadTime);
    
    // 场景统计
    OutSample.Primitives = CalculateScenePrimitives();
    OutSample.DrawCalls = OutSample.Primitives;
    OutSample.Triangles = CalculateSceneTriangles();
    
    // 内存使用
    FPlatformMemoryStats MemStats = FPlatformMemory::GetStats();
    OutSample.MemoryUsedMB = MemStats.UsedPhysical / (1024.0f * 1024.0f);
}



int32 FPerformanceMonitor::CalculateSceneTriangles()
{
    int32 TotalTriangles = 0;
    
    UWorld* World = nullptr;
    
#if WITH_EDITOR
    if (GEditor && GEditor->GetEditorWorldContext().World())
    {
        World = GEditor->GetEditorWorldContext().World();
    }
#endif
    
    if (!World && GEngine)
    {
        World = GEngine->GetWorld();
    }
    
    if (!World)
    {
        return 0;
    }
    
    // 遍历所有静态网格组件
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (!Actor || Actor->IsHidden())
        {
            continue;
        }
        
        TArray<UStaticMeshComponent*> MeshComponents;
        Actor->GetComponents<UStaticMeshComponent>(MeshComponents);
        
        for (UStaticMeshComponent* MeshComp : MeshComponents)
        {
            if (!MeshComp || !MeshComp->IsVisible() || !MeshComp->GetStaticMesh())
            {
                continue;
            }
            
            UStaticMesh* Mesh = MeshComp->GetStaticMesh();
            
            // 安全检查RenderData
            if (!Mesh->GetRenderData())
            {
                continue;
            }
            
            // 直接访问LODResources（不要创建TArray引用）
            FStaticMeshRenderData* RenderData = Mesh->GetRenderData();
            
            // 检查LOD数量
            if (RenderData->LODResources.Num() == 0)
            {
                continue;
            }
            
            // 使用LOD0（最高质量）
            TotalTriangles += RenderData->LODResources[0].GetNumTriangles();
        }
    }
    
    return TotalTriangles;
}

int32 FPerformanceMonitor::CalculateScenePrimitives()
{
    int32 TotalPrimitives = 0;
    
    UWorld* World = nullptr;
    
#if WITH_EDITOR
    if (GEditor && GEditor->GetEditorWorldContext().World())
    {
        World = GEditor->GetEditorWorldContext().World();
    }
#endif
    
    if (!World && GEngine)
    {
        World = GEngine->GetWorld();
    }
    
    if (!World)
    {
        return 0;
    }
    
    // 统计可见的Primitive组件数量
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (!Actor || Actor->IsHidden())
        {
            continue;
        }
        
        TArray<UPrimitiveComponent*> PrimitiveComponents;
        Actor->GetComponents<UPrimitiveComponent>(PrimitiveComponents);
        
        for (UPrimitiveComponent* PrimComp : PrimitiveComponents)
        {
            if (PrimComp && PrimComp->IsVisible())
            {
                TotalPrimitives++;
            }
        }
    }
    
    return TotalPrimitives;
}
FPerformanceSample FPerformanceMonitor::GetLatestSample() const
{
    if (HistorySamples.Num() == 0)
    {
        return FPerformanceSample();
    }
    
    if (HistorySamples.Num() < MaxHistorySamples)
    {
        return HistorySamples.Last();
    }
    else
    {
        int32 LatestIndex = (CurrentSampleIndex - 1 + MaxHistorySamples) % MaxHistorySamples;
        return HistorySamples[LatestIndex];
    }
}

TArray<FPerformanceSample> FPerformanceMonitor::GetHistorySamples(float Seconds) const
{
    TArray<FPerformanceSample> Result;
    
    if (HistorySamples.Num() == 0)
    {
        return Result;
    }
    
    int32 NumSamples = FMath::Min(
        static_cast<int32>(Seconds / SampleInterval),
        HistorySamples.Num()
    );
    
    Result.Reserve(NumSamples);
    
    for (int32 i = 0; i < NumSamples; ++i)
    {
        int32 Index;
        if (HistorySamples.Num() < MaxHistorySamples)
        {
            Index = HistorySamples.Num() - 1 - i;
        }
        else
        {
            Index = (CurrentSampleIndex - 1 - i + MaxHistorySamples) % MaxHistorySamples;
        }
        
        if (Index >= 0 && Index < HistorySamples.Num())
        {
            Result.Add(HistorySamples[Index]);
        }
    }
    
    // 反转数组，使其按时间顺序排列
    Algo::Reverse(Result);
    
    return Result;
}

void FPerformanceMonitor::ClearHistory()
{
    HistorySamples.Empty();
    CurrentSampleIndex = 0;
    AccumulatedTime = 0.0f;
    
    UE_LOG(LogTemp, Log, TEXT("Performance Monitor: History cleared"));
}

float FPerformanceMonitor::GetAverageFPS() const
{
    if (HistorySamples.Num() == 0)
    {
        return 0.0f;
    }
    
    float TotalFrameTime = 0.0f;
    for (const FPerformanceSample& Sample : HistorySamples)
    {
        TotalFrameTime += Sample.FrameTimeMS;
    }
    
    float AvgFrameTime = TotalFrameTime / HistorySamples.Num();
    return AvgFrameTime > 0.0f ? 1000.0f / AvgFrameTime : 0.0f;
}

float FPerformanceMonitor::GetAverageFrameTime() const
{
    if (HistorySamples.Num() == 0)
    {
        return 0.0f;
    }
    
    float TotalFrameTime = 0.0f;
    for (const FPerformanceSample& Sample : HistorySamples)
    {
        TotalFrameTime += Sample.FrameTimeMS;
    }
    
    return TotalFrameTime / HistorySamples.Num();
}

int32 FPerformanceMonitor::GetAverageDrawCalls() const
{
    if (HistorySamples.Num() == 0)
    {
        return 0;
    }
    
    int32 TotalDrawCalls = 0;
    for (const FPerformanceSample& Sample : HistorySamples)
    {
        TotalDrawCalls += Sample.DrawCalls;
    }
    
    return TotalDrawCalls / HistorySamples.Num();
}