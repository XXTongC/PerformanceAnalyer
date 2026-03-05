
#pragma once

#include "CoreMinimal.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Engine/StaticMeshActor.h"
#include "MaterialOptimizationTransaction.h"

/**
 * 材质优化结果
 */
struct FMaterialOptimizationResult
{
    /** 原始材质 */
    TWeakObjectPtr<UMaterialInterface> OriginalMaterial;

    /** 创建的材质实例 */
    TWeakObjectPtr<UMaterialInstanceConstant> MaterialInstance;

    /** 使用该材质的 Actor 数量 */
    int32 ActorCount;

    /** 是否成功 */
    bool bSuccess;

    /** 是否已经优化过 */
    bool bAlreadyOptimized;

    /** 错误信息 */
    FString ErrorMessage;

    FMaterialOptimizationResult()
        : ActorCount(0)
        , bSuccess(false)
        , bAlreadyOptimized(false)
    {}
};

/**
 * 材质使用信息
 */
struct FMaterialUsageInfo
{
    /** 材质 */
    TWeakObjectPtr<UMaterialInterface> Material;

    /** 使用该材质的 Actor 列表 */
    TArray<TWeakObjectPtr<AActor>> Actors;

    /** 是否已经是材质实例 */
    bool bIsInstance;

    /** 是否可以优化 */
    bool bCanOptimize;

    /** 材质名称 */
    FString MaterialName;

    FMaterialUsageInfo()
        : bIsInstance(false)
        , bCanOptimize(true)
    {}

    /** 获取使用数量 */
    int32 GetUsageCount() const { return Actors.Num(); }
};

/**
 * 材质优化器设置
 */
struct FMaterialOptimizerSettings
{
    /** 是否创建材质实例的副本 */
    bool bCreateCopy;

    /** 材质实例保存路径 */
    FString SavePath;

    /** 是否自动保存资产 */
    bool bAutoSave;

    /** 是否只优化选中的 Actor */
    bool bOnlySelectedActors;

    /** 命名前缀 */
    FString InstancePrefix;

    /** 命名后缀 */
    FString InstanceSuffix;

    FMaterialOptimizerSettings()
        : bCreateCopy(true)
        , SavePath(TEXT("/Game/Materials/Instances"))
        , bAutoSave(true)
        , bOnlySelectedActors(false)
        , InstancePrefix(TEXT("MI_"))
        , InstanceSuffix(TEXT(""))
    {}
};

/**
 * 材质优化器 - 批量转换材质为材质实例
 */
class PERFORMANCEANALYZER_API FMaterialOptimizer
{
public:
    FMaterialOptimizer();
    ~FMaterialOptimizer();

    /**
     * 扫描场景中的材质使用情况
     * @param World 要扫描的世界
     * @param bOnlySelected 是否只扫描选中的 Actor
     * @return 材质使用信息列表
     */
    static TArray<FMaterialUsageInfo> ScanSceneMaterials(UWorld* World, bool bOnlySelected = false);

    /**
 * 优化单个材质
 * @param Material 要优化的材质
 * @param Transaction 事务对象（可选）
 * @return 优化结果
 */
    static FMaterialOptimizationResult OptimizeMaterial(
        UMaterialInterface* Material,
        FMaterialOptimizationTransaction* Transaction = nullptr
    );

    /**
     * 批量优化材质
     * @param MaterialsToOptimize 要优化的材质列表
     * @param Settings 优化设置
     * @param Transaction 事务对象（可选）
     * @return 优化结果列表
     */
    static TArray<FMaterialOptimizationResult> BatchOptimizeMaterials(
        const TArray<FMaterialUsageInfo>& MaterialsToOptimize,
        const FMaterialOptimizerSettings& Settings,
        FMaterialOptimizationTransaction* Transaction = nullptr
    );

    /**
     * 为单个材质创建材质实例
     * @param SourceMaterial 源材质
     * @param Settings 优化设置
     * @return 创建的材质实例
     */
    static UMaterialInstanceConstant* CreateMaterialInstance(
        UMaterialInterface* SourceMaterial,
        const FMaterialOptimizerSettings& Settings
    );

    /**
     * 替换 Actor 上的材质
     * @param Actor 目标 Actor
     * @param OldMaterial 旧材质
     * @param NewMaterial 新材质
     * @return 是否成功
     */
    static bool ReplaceMaterialOnActor(
        AActor* Actor,
        UMaterialInterface* OldMaterial,
        UMaterialInterface* NewMaterial
    );

    /**
     * 生成材质实例名称
     * @param SourceMaterial 源材质
     * @param Settings 设置
     * @return 生成的名称
     */
    static FString GenerateInstanceName(
        UMaterialInterface* SourceMaterial,
        const FMaterialOptimizerSettings& Settings
    );

    /**
     * 检查材质是否可以优化
     * @param Material 要检查的材质
     * @return 是否可以优化
     */
    static bool CanOptimizeMaterial(UMaterialInterface* Material);

    /**
     * 获取材质的参数信息
     * @param Material 材质
     * @return 参数信息字符串
     */
    static FString GetMaterialParameterInfo(UMaterialInterface* Material);
    
    /** 确保保存路径存在 */
    static bool EnsureSavePathExists(const FString& Path);

private:
    /** 收集 Actor 上的所有材质 */
    static void CollectActorMaterials(AActor* Actor, TMap<UMaterialInterface*, FMaterialUsageInfo>& OutMaterialMap);

};