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
    , CachedTriangles(0)
    , CachedPrimitives(0)
    , LastCacheUpdateTime(0.0)
    , bCacheDirty(true)
    , MinCacheUpdateInterval(0.5f)
    , bEventListenersInitialized(false)
{
    HistorySamples.Reserve(MaxHistorySamples);
    
    // 注册事件监听
    UE_LOG(LogTemp, Log, TEXT("Performance Monitor: Constructor called"));
}

FPerformanceMonitor::~FPerformanceMonitor()
{
    // 取消事件监听
    if (bEventListenersInitialized && GEngine)
    {
        GEngine->OnLevelActorAdded().Remove(OnActorSpawnedHandle);
        GEngine->OnLevelActorDeleted().Remove(OnActorDestroyedHandle);
        
        UE_LOG(LogTemp, Log, TEXT("Performance Monitor: Event listeners unregistered"));
    }
}
void FPerformanceMonitor::InitializeEventListeners()
{

    // 防止重复初始化
    if (bEventListenersInitialized)
    {
        UE_LOG(LogTemp, Warning, TEXT("Performance Monitor: Event listeners already initialized"));
        return;
    }
    
    // 检查 GEngine 是否可用
    if (!GEngine)
    {
        UE_LOG(LogTemp, Warning, TEXT("Performance Monitor: GEngine is not available yet"));
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("Performance Monitor: Attempting to register event listeners..."));
    
    // 检查委托是否有效
   
     UE_LOG(LogTemp, Log, TEXT("Performance Monitor: OnLevelActorAdded delegate valid: %s"), 
        GEngine->OnLevelActorAdded().IsBound() ? TEXT("Yes") : TEXT("No (empty)"));
    UE_LOG(LogTemp, Log, TEXT("Performance Monitor: OnLevelActorDeleted delegate valid: %s"), 
        GEngine->OnLevelActorDeleted().IsBound() ? TEXT("Yes") : TEXT("No (empty)"));
    
    
        // 注册事件监听
    OnActorSpawnedHandle = GEngine->OnLevelActorAdded().AddRaw(
        this, &FPerformanceMonitor::OnActorSpawned);
    OnActorDestroyedHandle = GEngine->OnLevelActorDeleted().AddRaw(
        this, &FPerformanceMonitor::OnActorDestroyed);
    
    bEventListenersInitialized = true;
    
    
    UE_LOG(LogTemp, Log, TEXT("Performance Monitor: Event listeners registered successfully"));
    UE_LOG(LogTemp, Log, TEXT("Performance Monitor: OnActorSpawned handle valid: %s"), 
        OnActorSpawnedHandle.IsValid() ? TEXT("Yes") : TEXT("No"));
    UE_LOG(LogTemp, Log, TEXT("Performance Monitor: OnActorDestroyed handle valid: %s"), 
        OnActorDestroyedHandle.IsValid() ? TEXT("Yes") : TEXT("No"));

}
TStatId FPerformanceMonitor::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(FPerformanceMonitor, STATGROUP_Tickables);
}

void FPerformanceMonitor::Tick(float DeltaTime)
{
    if (!bEventListenersInitialized&&GEngine)
    {
        InitializeEventListeners();
    }
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
    UE_LOG(LogTemp, Log, TEXT("Performance Monitor: SetEnabled called with bEnabled=%s"), 
        bEnabled ? TEXT("true") : TEXT("false"));
    bIsEnabled = bEnabled;
    
    if (bEnabled)
    {
        if (!bEventListenersInitialized)
        {
            UE_LOG(LogTemp, Log, TEXT("Performance Monitor: Initializing event listeners..."));
            InitializeEventListeners();
        }else{
            UE_LOG(LogTemp, Log, TEXT("Performance Monitor: Event listeners already initialized"));
        }
        UE_LOG(LogTemp, Log, TEXT("Performance Monitor: Enabled"));
        ClearHistory();
        bCacheDirty = true;
        UpdateSceneCache();
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("Performance Monitor: Disabled"));
    }
}

void FPerformanceMonitor::OnActorSpawned(AActor* Actor)
{
    if (!Actor)
    {
        return;
    }
    
    if (!bIsEnabled)
    {
        return;
    }
    
    bCacheDirty = true;
    UE_LOG(LogTemp, Warning, TEXT("Performance Monitor: Scene cache invalidated (Actor spawned)"));
}

void FPerformanceMonitor::UpdateSceneCache()
{
    double CurrentTime = FPlatformTime::Seconds();
    
    if (bCacheDirty && (CurrentTime - LastCacheUpdateTime > MinCacheUpdateInterval))
    {
        int32 OldTriangles = CachedTriangles;
        int32 OldPrimitives = CachedPrimitives;

        CachedTriangles = CalculateSceneTriangles();
        CachedPrimitives = CalculateScenePrimitives();
        LastCacheUpdateTime = CurrentTime;
        bCacheDirty = false;

        UE_LOG(LogTemp, Warning, 
            TEXT("UpdateSceneCache: Triangles: %d->%d, Primitives: %d->%d, bCacheDirty was true"),
            OldTriangles, CachedTriangles, OldPrimitives, CachedPrimitives);
    }
    else if (!bCacheDirty)
    {
        UE_LOG(LogTemp, Verbose, 
            TEXT("UpdateSceneCache: Cache not dirty, using cached values - Triangles=%d, Primitives=%d"),
            CachedTriangles, CachedPrimitives);
    }
    else
    {
        UE_LOG(LogTemp, Verbose, 
            TEXT("UpdateSceneCache: Update interval not reached (%.2f < %.2f)"),
            CurrentTime - LastCacheUpdateTime, MinCacheUpdateInterval);
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
    OutSample.FrameTimeMS = FApp::GetDeltaTime() * 1000.0f;
    OutSample.FPS = OutSample.FrameTimeMS > 0.0f ? 1000.0f / OutSample.FrameTimeMS : 0.0f;
    
    // 线程时间
    OutSample.GameThreadTimeMS = FPlatformTime::ToMilliseconds(GGameThreadTime);
    OutSample.RenderThreadTimeMS = FPlatformTime::ToMilliseconds(GRenderThreadTime);
    
    // 场景统计
    UpdateSceneCache();
    OutSample.Triangles = CachedTriangles;
    OutSample.Primitives = CachedPrimitives;
    
    UE_LOG(LogTemp, Warning, 
       TEXT("TryGetRHIStats: Before STATS block - Triangles=%d, Primitives=%d"),
       OutSample.Triangles, OutSample.Primitives);
    
    // 内存使用
    FPlatformMemoryStats MemStats = FPlatformMemory::GetStats();
    OutSample.MemoryUsedMB = MemStats.UsedPhysical / (1024.0f * 1024.0f);
#if STATS
        
    OutSample.DrawCalls = GNumDrawCallsRHI[0];
    OutSample.Primitives = GNumPrimitivesDrawnRHI[0];
      
    UE_LOG(LogTemp, Warning, 
        TEXT("TryGetRHIStats: After STATS block - Triangles=%d, Primitives=%d, DrawCalls=%d"),
        OutSample.Triangles, OutSample.Primitives, OutSample.DrawCalls);
    
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
    OutSample.DrawCalls = OutSample.Primitives;
    OutSample.Primitives = CachedPrimitives;
    OutSample.GPUTimeMS = OutSample.FrameTimeMS;
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
    
    // 使用缓存的场景统计
    UpdateSceneCache();
    OutSample.Primitives = CachedPrimitives;
    OutSample.DrawCalls = CachedPrimitives; 
    OutSample.Triangles = CachedTriangles;
    
        // 内存使用
    FPlatformMemoryStats MemStats = FPlatformMemory::GetStats();
    OutSample.MemoryUsedMB = MemStats.UsedPhysical / (1024.0f * 1024.0f);
    
    OutSample.FPS = AverageFPS;
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
    UpdateSceneCache();
    OutSample.Primitives = CachedPrimitives;
    OutSample.DrawCalls = CachedPrimitives;
    OutSample.Triangles = CachedTriangles;
    
    // 内存使用
    FPlatformMemoryStats MemStats = FPlatformMemory::GetStats();
    OutSample.MemoryUsedMB = MemStats.UsedPhysical / (1024.0f * 1024.0f);
    OutSample.FPS = OutSample.FrameTimeMS > 0.0F ? 1000.0f / OutSample.FrameTimeMS : 0.0f;
}




int32 FPerformanceMonitor::CalculateSceneTriangles()
{
    int32 TotalTriangles = 0;
    
    UWorld* World = nullptr;
    
    if (GEngine)
    {
        // 尝试获取PIE World（Play In Editor）
        for (const FWorldContext& Context : GEngine->GetWorldContexts())
        {
            if (Context.WorldType == EWorldType::PIE || Context.WorldType == EWorldType::Game)
            {
                World = Context.World();
                UE_LOG(LogTemp, Warning, TEXT("CalculateSceneTriangles: Found PIE/Game World"));
                break;
            }
        }
        
        // 如果没有PIE World，尝试获取编辑器World
        if (!World)
        {
#if WITH_EDITOR
            if (GEditor && GEditor->GetEditorWorldContext().World())
            {
                World = GEditor->GetEditorWorldContext().World();
                UE_LOG(LogTemp, Warning, TEXT("CalculateSceneTriangles: Using Editor World"));
            }
#endif
        }

        if (!World && GEngine)
        {
            World = GEngine->GetWorld();
        }
    }
    
    
    if (!World)
    {
        UE_LOG(LogTemp, Warning, TEXT("CalculateSceneTriangles: World is nullptr!"));
        return 0;
    }
    
    int32 ActorCount = 0;
    int32 MeshComponentCount = 0;
    int32 ValidMeshCount = 0;
    
    // 遍历所有静态网格组件
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (!Actor || Actor->IsHidden())
        {
            continue;
        }
        
        ActorCount++;
        
        TArray<UStaticMeshComponent*> MeshComponents;
        Actor->GetComponents<UStaticMeshComponent>(MeshComponents);
        
        MeshComponentCount += MeshComponents.Num();
        
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
                UE_LOG(LogTemp, Warning, 
                    TEXT("CalculateSceneTriangles: Mesh '%s' has no RenderData"), 
                    *Mesh->GetName());
                continue;
            }
            
            // 直接访问LODResources
            FStaticMeshRenderData* RenderData = Mesh->GetRenderData();
            
            // 检查LOD数量
            if (RenderData->LODResources.Num() == 0)
            {
                UE_LOG(LogTemp, Warning, 
                    TEXT("CalculateSceneTriangles: Mesh '%s' has no LOD resources"), 
                    *Mesh->GetName());
                continue;
            }
            
            // 使用LOD0（最高质量）
            int32 MeshTriangles = RenderData->LODResources[0].GetNumTriangles();
            TotalTriangles += MeshTriangles;
            ValidMeshCount++;
            
            UE_LOG(LogTemp, Verbose, 
                TEXT("CalculateSceneTriangles: Mesh '%s' has %d triangles"), 
                *Mesh->GetName(), MeshTriangles);
        }
        
        
    }

    UE_LOG(LogTemp, Warning, 
        TEXT("CalculateSceneTriangles: ActorCount=%d, MeshComponentCount=%d, ValidMeshCount=%d, TotalTriangles=%d"),
        ActorCount, MeshComponentCount, ValidMeshCount, TotalTriangles);
    
    return TotalTriangles;
}

int32 FPerformanceMonitor::CalculateScenePrimitives()
{
    int32 TotalPrimitives = 0;
    
    UWorld* World = nullptr;
    
if (GEngine)
    {
        for (const FWorldContext& Context : GEngine->GetWorldContexts())
        {
            if (Context.WorldType == EWorldType::PIE || Context.WorldType == EWorldType::Game)
            {
                World = Context.World();
                break;
            }
        }
        
        if (!World)
        {
#if WITH_EDITOR
            if (GEditor && GEditor->GetEditorWorldContext().World())
            {
                World = GEditor->GetEditorWorldContext().World();
            }
#endif
        }
        
        if (!World)
        {
            World = GEngine->GetWorld();
        }
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

void FPerformanceMonitor::OnActorDestroyed(AActor* Actor)
{
    if (!Actor)
    {
        UE_LOG(LogTemp, Warning, TEXT("Performance Monitor: OnActorDestroyed called with nullptr Actor"));
        return;
    }
    
    UE_LOG(LogTemp, Verbose, TEXT("Performance Monitor: OnActorDestroyed called for Actor: %s"), *Actor->GetName());
    
    if (!bIsEnabled)
    {
            
        return;
    }
    
    bCacheDirty = true;
    UE_LOG(LogTemp, Verbose, TEXT("Performance Monitor: Scene cache invalidated (Actor destroyed)"));
}

void FPerformanceMonitor::TestActorSpawnedCallback(AActor* TestActor)
{
    UE_LOG(LogTemp, Log, TEXT("Performance Monitor: Manual test - OnActorSpawned"));
    OnActorSpawned(TestActor);
}

void FPerformanceMonitor::TestActorDestroyedCallback(AActor* TestActor)
{
    UE_LOG(LogTemp, Log, TEXT("Performance Monitor: Manual test - OnActorDestroyed"));
    OnActorDestroyed(TestActor);
}
