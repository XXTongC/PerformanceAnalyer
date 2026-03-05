
#pragma once

#include "CoreMinimal.h"
#include "Engine/StaticMesh.h"

// 前向声明
class FLODGenerationTransaction;

/**
 * LOD 生成配置
 */
struct PERFORMANCEANALYZER_API FLODGenerationSettings
{
    // LOD 数量
    int32 NumLODs = 4;
    
    // 每个 LOD 的简化百分比
    TArray<float> ReductionPercentages = {
        0.5f,   // LOD1: 50%
        0.25f,  // LOD2: 25%
        0.125f, // LOD3: 12.5%
        0.0625f // LOD4: 6.25%
    };
    
    // 屏幕尺寸阈值
    TArray<float> ScreenSizes = {
        0.5f,
        0.25f,
        0.125f,
        0.0625f
    };
    
    // 简化选项
    bool bRecalculateNormals = true;
    bool bGenerateLightmapUVs = false;
    float WeldingThreshold = 0.1f;
    
    FLODGenerationSettings() {}
};

/**
 * LOD 预设类型
 */
enum class ELODPreset : uint8
{
    HighQuality,    // 高质量（保守简化）
    Balanced,       // 平衡
    Performance,    // 性能优先（激进简化）
    Mobile,         // 移动端优化
    Custom          // 自定义
};

/**
 * LOD 生成结果
 */
struct PERFORMANCEANALYZER_API FLODGenerationResult
{
    bool bSuccess = false;
    FString ErrorMessage;
    
    // 生成的 LOD 信息
    TArray<int32> LODTriangleCounts;
    TArray<float> ReductionRatios;
    
    // 性能估算
    float EstimatedPerformanceGain = 0.0f;
};

/**
 * 自动 LOD 生成器
 */
class PERFORMANCEANALYZER_API FAutoLODGenerator
{
public:
    /**
     * 使用自定义设置生成 LOD
     */
    static FLODGenerationResult GenerateLODs(
        UStaticMesh* Mesh,
        const FLODGenerationSettings& Settings
    );
    
    /**
     * 使用预设生成 LOD
     */
    static FLODGenerationResult GenerateLODsWithPreset(
        UStaticMesh* Mesh,
        ELODPreset Preset
    );
    
    /**
     * 智能生成 LOD（自动选择最佳参数）
     */
    static FLODGenerationResult GenerateLODsAuto(UStaticMesh* Mesh);
    
    /**
     * 批量生成 LOD
     */
    static int32 BatchGenerateLODs(
        const TArray<UStaticMesh*>& Meshes,
        ELODPreset Preset
    );
    
    /**
     * 批量生成 LOD（带事务支持） - 新增函数
     */
    static int32 BatchGenerateLODsWithTransaction(
        const TArray<UStaticMesh*>& Meshes,
        const FLODGenerationSettings& Settings,
        FLODGenerationTransaction* Transaction = nullptr
    );
    
    /**
     * 检查网格是否需要 LOD
     */
    static bool NeedsLODGeneration(UStaticMesh* Mesh);
    
    /**
     * 获取预设配置
     */
    static FLODGenerationSettings GetPresetSettings(ELODPreset Preset);
    
    /**
     * 预览 LOD 效果（不实际生成）
     */
    static FLODGenerationResult PreviewLODGeneration(
        UStaticMesh* Mesh,
        const FLODGenerationSettings& Settings
    );
   
private:
    static ELODPreset DetermineOptimalPreset(UStaticMesh* Mesh);
    static int32 GetTriangleCount(UStaticMesh* Mesh);
    static bool ValidateSettings(const FLODGenerationSettings& Settings);
    static float CalculatePerformanceGain(
        UStaticMesh* Mesh,
        const FLODGenerationSettings& Settings
    );
};