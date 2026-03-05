
#include "AutoLODGenerator.h"
#include "LODGenerationTransaction.h"
#include "Engine/StaticMesh.h"
#include "StaticMeshResources.h"

FLODGenerationResult FAutoLODGenerator::GenerateLODs(
    UStaticMesh* Mesh,
    const FLODGenerationSettings& Settings)
{
    FLODGenerationResult Result;
    
    if (!Mesh)
    {
        Result.ErrorMessage = TEXT("Invalid mesh");
        return Result;
    }
    
    if (!ValidateSettings(Settings))
    {
        Result.ErrorMessage = TEXT("Invalid settings");
        return Result;
    }
    
    UE_LOG(LogTemp, Log, TEXT("Generating %d LODs for mesh: %s"), 
        Settings.NumLODs, *Mesh->GetName());
    
    // 标记为可修改
    Mesh->Modify();
    
    // 设置 LOD 数量
    Mesh->SetNumSourceModels(Settings.NumLODs + 1);
    
    // 配置每个 LOD
    for (int32 LODIndex = 1; LODIndex <= Settings.NumLODs; ++LODIndex)
    {
        FStaticMeshSourceModel& LODModel = Mesh->GetSourceModel(LODIndex);
        FMeshReductionSettings& ReductionSettings = LODModel.ReductionSettings;
        
        // 设置简化参数
        ReductionSettings.PercentTriangles = Settings.ReductionPercentages[LODIndex - 1];
        ReductionSettings.PercentVertices = 1.0f;
        ReductionSettings.MaxDeviation = 0.0f;
        
        // 屏幕尺寸
        LODModel.ScreenSize = FMath::Clamp(Settings.ScreenSizes[LODIndex - 1], 0.01f, 1.0f);
        
        // 其他选项
        ReductionSettings.bRecalculateNormals = Settings.bRecalculateNormals;
        ReductionSettings.WeldingThreshold = Settings.WeldingThreshold;
        ReductionSettings.bGenerateUniqueLightmapUVs = Settings.bGenerateLightmapUVs;
        
        UE_LOG(LogTemp, Log, TEXT("  LOD%d: %.1f%% triangles, screen size: %.3f"), 
            LODIndex, 
            ReductionSettings.PercentTriangles * 100.0f,
            LODModel.ScreenSize.Default);
    }
    
    // 重新构建网格
    Mesh->Build();
    
    // 收集结果信息
    Result.bSuccess = true;
    int32 OriginalTriangles = GetTriangleCount(Mesh);
    Result.LODTriangleCounts.Add(OriginalTriangles);
    
    for (int32 LODIndex = 1; LODIndex <= Settings.NumLODs; ++LODIndex)
    {
        if (Mesh->GetRenderData() && Mesh->GetRenderData()->LODResources.IsValidIndex(LODIndex))
        {
            int32 LODTriangles = Mesh->GetRenderData()->LODResources[LODIndex].GetNumTriangles();
            Result.LODTriangleCounts.Add(LODTriangles);
            
            float Ratio = (float)LODTriangles / (float)OriginalTriangles;
            Result.ReductionRatios.Add(Ratio);
        }
    }
    
    Result.EstimatedPerformanceGain = CalculatePerformanceGain(Mesh, Settings);
    bool res = Mesh->MarkPackageDirty();
    
    UE_LOG(LogTemp, Log, TEXT("LOD generation complete. Performance gain: %.1f%%"),
        Result.EstimatedPerformanceGain * 100.0f);
    
    return Result;
}

FLODGenerationResult FAutoLODGenerator::GenerateLODsWithPreset(
    UStaticMesh* Mesh,
    ELODPreset Preset)
{
    FLODGenerationSettings Settings = GetPresetSettings(Preset);
    return GenerateLODs(Mesh, Settings);
}

FLODGenerationResult FAutoLODGenerator::GenerateLODsAuto(UStaticMesh* Mesh)
{
    if (!Mesh)
    {
        FLODGenerationResult Result;
        Result.ErrorMessage = TEXT("Invalid mesh");
        return Result;
    }
    
    ELODPreset OptimalPreset = DetermineOptimalPreset(Mesh);
    UE_LOG(LogTemp, Log, TEXT("Auto-generating LODs for %s using preset: %d"),
        *Mesh->GetName(), (int32)OptimalPreset);
    
    return GenerateLODsWithPreset(Mesh, OptimalPreset);
}

int32 FAutoLODGenerator::BatchGenerateLODs(
    const TArray<UStaticMesh*>& Meshes,
    ELODPreset Preset)
{
    int32 SuccessCount = 0;
    
    UE_LOG(LogTemp, Log, TEXT("Batch LOD generation for %d meshes"), Meshes.Num());
    
    for (UStaticMesh* Mesh : Meshes)
    {
        if (!Mesh || !NeedsLODGeneration(Mesh))
        {
            continue;
        }
        
        FLODGenerationResult Result = GenerateLODsWithPreset(Mesh, Preset);
        if (Result.bSuccess)
        {
            SuccessCount++;
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("Batch complete: %d/%d successful"),
        SuccessCount, Meshes.Num());
    
    return SuccessCount;
}

bool FAutoLODGenerator::NeedsLODGeneration(UStaticMesh* Mesh)
{
    if (!Mesh)
    {
        return false;
    }
    
    // 已经有 LOD
    if (Mesh->GetNumLODs() > 1)
    {
        return false;
    }
    
    // 三角形数太少
    int32 TriangleCount = GetTriangleCount(Mesh);
    if (TriangleCount < 1000)
    {
        return false;
    }
    
    return true;
}

FLODGenerationSettings FAutoLODGenerator::GetPresetSettings(ELODPreset Preset)
{
    FLODGenerationSettings Settings;
    
    switch (Preset)
    {
        case ELODPreset::HighQuality:
            Settings.NumLODs = 4;
            Settings.ReductionPercentages = { 0.7f, 0.5f, 0.3f, 0.15f };
            Settings.ScreenSizes = { 0.5f, 0.3f, 0.15f, 0.075f };
            Settings.bRecalculateNormals = true;
            Settings.WeldingThreshold = 0.1f;
            break;
            
        case ELODPreset::Balanced:
            Settings.NumLODs = 4;
            Settings.ReductionPercentages = { 0.5f, 0.25f, 0.125f, 0.0625f };
            Settings.ScreenSizes = { 0.5f, 0.25f, 0.125f, 0.0625f };
            Settings.bRecalculateNormals = true;
            Settings.WeldingThreshold = 0.1f;
            break;
            
        case ELODPreset::Performance:
            Settings.NumLODs = 5;
            Settings.ReductionPercentages = { 0.4f, 0.2f, 0.1f, 0.05f, 0.025f };
            Settings.ScreenSizes = { 0.5f, 0.25f, 0.125f, 0.0625f, 0.03125f };
            Settings.bRecalculateNormals = true;
            Settings.WeldingThreshold = 0.2f;
            break;
            
        case ELODPreset::Mobile:
            Settings.NumLODs = 3;
            Settings.ReductionPercentages = { 0.5f, 0.25f, 0.1f };
            Settings.ScreenSizes = { 0.5f, 0.25f, 0.1f };
            Settings.bRecalculateNormals = true;
            Settings.WeldingThreshold = 0.15f;
            break;
            
        case ELODPreset::Custom:
        default:
            // 使用默认设置
            break;
    }
    
    return Settings;
}

FLODGenerationResult FAutoLODGenerator::PreviewLODGeneration(
    UStaticMesh* Mesh,
    const FLODGenerationSettings& Settings)
{
    FLODGenerationResult Result;
    
    if (!Mesh)
    {
        Result.ErrorMessage = TEXT("Invalid mesh");
        return Result;
    }
    
    int32 OriginalTriangles = GetTriangleCount(Mesh);
    Result.LODTriangleCounts.Add(OriginalTriangles);
    
    for (int32 i = 0; i < Settings.NumLODs; ++i)
    {
        int32 PredictedTriangles = FMath::RoundToInt(
            OriginalTriangles * Settings.ReductionPercentages[i]
        );
        Result.LODTriangleCounts.Add(PredictedTriangles);
        Result.ReductionRatios.Add(Settings.ReductionPercentages[i]);
    }
    
    Result.EstimatedPerformanceGain = CalculatePerformanceGain(Mesh, Settings);
    Result.bSuccess = true;
    
    return Result;
}

// ===== 私有辅助函数 =====

ELODPreset FAutoLODGenerator::DetermineOptimalPreset(UStaticMesh* Mesh)
{
    if (!Mesh)
    {
        return ELODPreset::Balanced;
    }
    
    int32 TriangleCount = GetTriangleCount(Mesh);
    
    if (TriangleCount > 100000)
    {
        return ELODPreset::Performance;
    }
    else if (TriangleCount > 50000)
    {
        return ELODPreset::Balanced;
    }
    else if (TriangleCount > 10000)
    {
        return ELODPreset::HighQuality;
    }
    else
    {
        return ELODPreset::Mobile;
    }
}

int32 FAutoLODGenerator::GetTriangleCount(UStaticMesh* Mesh)
{
    if (!Mesh || !Mesh->GetRenderData())
    {
        return 0;
    }
    
    if (Mesh->GetRenderData()->LODResources.Num() > 0)
    {
        return Mesh->GetRenderData()->LODResources[0].GetNumTriangles();
    }
    
    return 0;
}

bool FAutoLODGenerator::ValidateSettings(const FLODGenerationSettings& Settings)
{
    if (Settings.NumLODs < 1 || Settings.NumLODs > 8)
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid NumLODs: %d (must be 1-8)"), Settings.NumLODs);
        return false;
    }
    
    if (Settings.ReductionPercentages.Num() != Settings.NumLODs)
    {
        UE_LOG(LogTemp, Error, TEXT("ReductionPercentages size mismatch"));
        return false;
    }
    
    if (Settings.ScreenSizes.Num() != Settings.NumLODs)
    {
        UE_LOG(LogTemp, Error, TEXT("ScreenSizes size mismatch"));
        return false;
    }
    
    for (int32 i = 0; i < Settings.NumLODs; ++i)
    {
        if (Settings.ReductionPercentages[i] <= 0.0f || Settings.ReductionPercentages[i] > 1.0f)
        {
            UE_LOG(LogTemp, Error, TEXT("Invalid reduction percentage at index %d: %f"),
                i, Settings.ReductionPercentages[i]);
            return false;
        }
    }
    
    return true;
}

float FAutoLODGenerator::CalculatePerformanceGain(
    UStaticMesh* Mesh,
    const FLODGenerationSettings& Settings)
{
    if (!Mesh)
    {
        return 0.0f;
    }
    
    int32 OriginalTriangles = GetTriangleCount(Mesh);
    if (OriginalTriangles == 0)
    {
        return 0.0f;
    }
    
    // 假设平均使用中间 LOD
    float AverageLODIndex = Settings.NumLODs / 2.0f;
    int32 LODIndex = FMath::Clamp(FMath::RoundToInt(AverageLODIndex), 0, Settings.NumLODs - 1);
    
    float AverageReduction = Settings.ReductionPercentages[LODIndex];
    int32 AverageTriangles = FMath::RoundToInt(OriginalTriangles * AverageReduction);
    
    float PerformanceGain = 1.0f - ((float)AverageTriangles / (float)OriginalTriangles);
    
    return FMath::Clamp(PerformanceGain, 0.0f, 1.0f);
}

int32 FAutoLODGenerator::BatchGenerateLODsWithTransaction(
    const TArray<UStaticMesh*>& Meshes,
    const FLODGenerationSettings& Settings,
    FLODGenerationTransaction* Transaction)
{
// 在函数开始处添加
    UE_LOG(LogTemp, Error, TEXT("=== BatchGenerateLODsWithTransaction START ==="));
    UE_LOG(LogTemp, Error, TEXT("Meshes count: %d"), Meshes.Num());
    UE_LOG(LogTemp, Error, TEXT("Transaction valid: %s"), Transaction ? TEXT("Yes") : TEXT("No"));

    if (Meshes.Num() == 0)
    {
        return 0;
    }
    
    // 如果没有提供事务，创建临时事务
    TUniquePtr<FLODGenerationTransaction> TempTransaction;
    FLODGenerationTransaction* ActiveTransaction = Transaction;
    
    if (!ActiveTransaction)
    {
        TempTransaction = MakeUnique<FLODGenerationTransaction>();
        ActiveTransaction = TempTransaction.Get();
    }
    
    // 开始事务
    ActiveTransaction->BeginTransaction(
        FText::FromString(FString::Printf(TEXT("Generate LODs for %d meshes"), Meshes.Num()))
    );
    
    int32 SuccessCount = 0;
    
    // 批量生成
    for (UStaticMesh* Mesh : Meshes)
    {
        if (!Mesh)
        {
            continue;
        }
        
        // 备份网格设置
        ActiveTransaction->BackupMeshLODSettings(Mesh);
        
        // 生成 LOD
        FLODGenerationResult Result = GenerateLODs(Mesh, Settings);
        
        if (Result.bSuccess)
        {
            SuccessCount++;
            UE_LOG(LogTemp, Log, TEXT("Successfully generated LODs for: %s"), *Mesh->GetName());
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to generate LODs for %s: %s"), 
                *Mesh->GetName(), *Result.ErrorMessage);
        }
    }
    
    // 结束事务
    ActiveTransaction->EndTransaction();
    
    UE_LOG(LogTemp, Log, TEXT("Batch LOD generation completed: %d/%d succeeded"), 
        SuccessCount, Meshes.Num());
    
    return SuccessCount;
}