
#include "MaterialOptimizer.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Editor.h"

FMaterialOptimizer::FMaterialOptimizer()
{
}

FMaterialOptimizer::~FMaterialOptimizer()
{
}

TArray<FMaterialUsageInfo> FMaterialOptimizer::ScanSceneMaterials(UWorld* World, bool bOnlySelected)
{
    TArray<FMaterialUsageInfo> Results;
    
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid World"));
        return Results;
    }

    UE_LOG(LogTemp, Log, TEXT("Scanning scene materials..."));

    // 材质使用映射表
    TMap<UMaterialInterface*, FMaterialUsageInfo> MaterialMap;

    // 遍历所有 Actor
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        
        // 如果只扫描选中的 Actor
        if (bOnlySelected && !Actor->IsSelected())
        {
            continue;
        }

        // 收集该 Actor 的材质
        CollectActorMaterials(Actor, MaterialMap);
    }

    // 转换为数组
    for (auto& Pair : MaterialMap)
    {
        FMaterialUsageInfo& Info = Pair.Value;
        
        // 检查是否可以优化
        Info.bCanOptimize = CanOptimizeMaterial(Info.Material.Get());
        
        Results.Add(Info);
    }

    UE_LOG(LogTemp, Log, TEXT("Found %d unique materials"), Results.Num());

    return Results;
}

void FMaterialOptimizer::CollectActorMaterials(AActor* Actor, TMap<UMaterialInterface*, FMaterialUsageInfo>& OutMaterialMap)
{
    if (!Actor)
    {
        return;
    }

    // 获取所有静态网格组件
    TArray<UStaticMeshComponent*> StaticMeshComponents;
    Actor->GetComponents<UStaticMeshComponent>(StaticMeshComponents);

    for (UStaticMeshComponent* Component : StaticMeshComponents)
    {
        if (!Component)
        {
            continue;
        }

        // 遍历所有材质槽
        TArray<UMaterialInterface*> Materials = Component->GetMaterials();
        for (UMaterialInterface* Material : Materials)
        {
            if (!Material)
            {
                continue;
            }

            // 添加到映射表
            FMaterialUsageInfo& Info = OutMaterialMap.FindOrAdd(Material);
            Info.Material = Material;
            Info.Actors.AddUnique(Actor);
            Info.bIsInstance = Material->IsA<UMaterialInstance>();
            Info.MaterialName = Material->GetName();
        }
    }

    // 获取所有骨骼网格组件
    TArray<USkeletalMeshComponent*> SkeletalMeshComponents;
    Actor->GetComponents<USkeletalMeshComponent>(SkeletalMeshComponents);

    for (USkeletalMeshComponent* Component : SkeletalMeshComponents)
    {
        if (!Component)
        {
            continue;
        }

        TArray<UMaterialInterface*> Materials = Component->GetMaterials();
        for (UMaterialInterface* Material : Materials)
        {
            if (!Material)
            {
                continue;
            }

            FMaterialUsageInfo& Info = OutMaterialMap.FindOrAdd(Material);
            Info.Material = Material;
            Info.Actors.AddUnique(Actor);
            Info.bIsInstance = Material->IsA<UMaterialInstance>();
            Info.MaterialName = Material->GetName();
        }
    }
}


TArray<FMaterialOptimizationResult> FMaterialOptimizer::BatchOptimizeMaterials(
    const TArray<FMaterialUsageInfo>& MaterialsToOptimize,
    const FMaterialOptimizerSettings& Settings,
    FMaterialOptimizationTransaction* Transaction)
{
    TArray<FMaterialOptimizationResult> Results;

    UE_LOG(LogTemp, Log, TEXT("Starting batch material optimization for %d materials"), MaterialsToOptimize.Num());

    // 确保保存路径存在
    if (!EnsureSavePathExists(Settings.SavePath))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create save path: %s"), *Settings.SavePath);
        return Results;
    }

    for (const FMaterialUsageInfo& MaterialInfo : MaterialsToOptimize)
    {
        UMaterialInterface* SourceMaterial = MaterialInfo.Material.Get();
        if (!SourceMaterial)
        {
            continue;
        }

        // 如果已经是材质实例，跳过
        if (MaterialInfo.bIsInstance)
        {
            UE_LOG(LogTemp, Warning, TEXT("Skipping %s - already a material instance"), *SourceMaterial->GetName());
            continue;
        }

        // 优化材质（传入事务）
        FMaterialOptimizationResult Result = OptimizeMaterial(SourceMaterial, Transaction);
        Results.Add(Result);
    }

    int32 SuccessCount = Results.FilterByPredicate([](const FMaterialOptimizationResult& R) { return R.bSuccess; }).Num();
    UE_LOG(LogTemp, Log, TEXT("Batch optimization complete: %d/%d successful"), 
        SuccessCount,
        Results.Num());

    return Results;
}
UMaterialInstanceConstant* FMaterialOptimizer::CreateMaterialInstance(
    UMaterialInterface* SourceMaterial,
    const FMaterialOptimizerSettings& Settings)
{
    if (!SourceMaterial)
    {
        return nullptr;
    }

    // 生成材质实例名称
    FString InstanceName = GenerateInstanceName(SourceMaterial, Settings);

    // 生成完整路径
    FString PackageName = Settings.SavePath / InstanceName;
    
    // 检查是否已存在
    UPackage* Package = CreatePackage(*PackageName);
    if (!Package)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create package: %s"), *PackageName);
        return nullptr;
    }

    // 创建材质实例
    UMaterialInstanceConstant* NewInstance = NewObject<UMaterialInstanceConstant>(
        Package,
        *InstanceName,
        RF_Public | RF_Standalone
    );

    if (!NewInstance)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create material instance object"));
        return nullptr;
    }

    // 设置父材质
    UMaterial* ParentMaterial = Cast<UMaterial>(SourceMaterial);
    if (ParentMaterial)
    {
        NewInstance->SetParentEditorOnly(ParentMaterial);
    }
    else
    {
        // 如果源材质是材质实例，获取其基础材质
        UMaterialInstance* SourceInstance = Cast<UMaterialInstance>(SourceMaterial);
        if (SourceInstance)
        {
            NewInstance->SetParentEditorOnly(SourceInstance->GetMaterial());
        }
    }

    // 复制参数（如果源材质是材质实例）
    UMaterialInstance* SourceInstance = Cast<UMaterialInstance>(SourceMaterial);
    if (SourceInstance)
    {
        // 复制标量参数
        TArray<FMaterialParameterInfo> ScalarParams;
        TArray<FGuid> ScalarGuids;
        SourceInstance->GetAllScalarParameterInfo(ScalarParams, ScalarGuids);
        
        for (const FMaterialParameterInfo& ParamInfo : ScalarParams)
        {
            float Value;
            if (SourceInstance->GetScalarParameterValue(ParamInfo, Value))
            {
                NewInstance->SetScalarParameterValueEditorOnly(ParamInfo, Value);
            }
        }

        // 复制向量参数
        TArray<FMaterialParameterInfo> VectorParams;
        TArray<FGuid> VectorGuids;
        SourceInstance->GetAllVectorParameterInfo(VectorParams, VectorGuids);
        
        for (const FMaterialParameterInfo& ParamInfo : VectorParams)
        {
            FLinearColor Value;
            if (SourceInstance->GetVectorParameterValue(ParamInfo, Value))
            {
                NewInstance->SetVectorParameterValueEditorOnly(ParamInfo, Value);
            }
        }

        // 复制纹理参数
        TArray<FMaterialParameterInfo> TextureParams;
        TArray<FGuid> TextureGuids;
        SourceInstance->GetAllTextureParameterInfo(TextureParams, TextureGuids);
        
        for (const FMaterialParameterInfo& ParamInfo : TextureParams)
        {
            UTexture* Value;
            if (SourceInstance->GetTextureParameterValue(ParamInfo, Value))
            {
                NewInstance->SetTextureParameterValueEditorOnly(ParamInfo, Value);
            }
        }
    }

    // 更新材质实例
    NewInstance->PostEditChange();

    // 保存资产
    if (Settings.bAutoSave)
    {
        FString PackageFileName = FPackageName::LongPackageNameToFilename(
            PackageName,
            FPackageName::GetAssetPackageExtension()
        );

        FSavePackageArgs SaveArgs;
        SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
        SaveArgs.SaveFlags = SAVE_NoError;

        if (UPackage::SavePackage(Package, NewInstance, *PackageFileName, SaveArgs))
        {
            UE_LOG(LogTemp, Log, TEXT("Saved material instance: %s"), *PackageFileName);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to save material instance: %s"), *PackageFileName);
        }
    }

    return NewInstance;
}

bool FMaterialOptimizer::ReplaceMaterialOnActor(
    AActor* Actor,
    UMaterialInterface* OldMaterial,
    UMaterialInterface* NewMaterial)
{
    if (!Actor || !OldMaterial || !NewMaterial)
    {
        return false;
    }

    bool bReplacedAny = false;

    // 标记 Actor 为修改状态
    Actor->Modify();

    // 替换静态网格组件上的材质
    TArray<UStaticMeshComponent*> StaticMeshComponents;
    Actor->GetComponents<UStaticMeshComponent>(StaticMeshComponents);

    for (UStaticMeshComponent* Component : StaticMeshComponents)
    {
        if (!Component)
        {
            continue;
        }

        Component->Modify();

        // 遍历所有材质槽
        for (int32 i = 0; i < Component->GetNumMaterials(); ++i)
        {
            if (Component->GetMaterial(i) == OldMaterial)
            {
                Component->SetMaterial(i, NewMaterial);
                bReplacedAny = true;
            }
        }
    }

    // 替换骨骼网格组件上的材质
    TArray<USkeletalMeshComponent*> SkeletalMeshComponents;
    Actor->GetComponents<USkeletalMeshComponent>(SkeletalMeshComponents);

    for (USkeletalMeshComponent* Component : SkeletalMeshComponents)
    {
        if (!Component)
        {
            continue;
        }

        Component->Modify();

        for (int32 i = 0; i < Component->GetNumMaterials(); ++i)
        {
            if (Component->GetMaterial(i) == OldMaterial)
            {
                Component->SetMaterial(i, NewMaterial);
                bReplacedAny = true;
            }
        }
    }

    if (bReplacedAny)
    {
        Actor->PostEditChange();
    }

    return bReplacedAny;
}

FString FMaterialOptimizer::GenerateInstanceName(
    UMaterialInterface* SourceMaterial,
    const FMaterialOptimizerSettings& Settings)
{
    if (!SourceMaterial)
    {
        return TEXT("MI_Unknown");
    }

    FString BaseName = SourceMaterial->GetName();
    
    // 移除现有的前缀（如果有）
    if (BaseName.StartsWith(TEXT("M_")))
    {
        BaseName = BaseName.RightChop(2);
    }
    else if (BaseName.StartsWith(TEXT("MI_")))
    {
        BaseName = BaseName.RightChop(3);
    }

    // 添加新的前缀和后缀
    return Settings.InstancePrefix + BaseName + Settings.InstanceSuffix;
}

bool FMaterialOptimizer::CanOptimizeMaterial(UMaterialInterface* Material)
{
    if (!Material)
    {
        return false;
    }

    // 如果已经是材质实例，不需要优化
    if (Material->IsA<UMaterialInstance>())
    {
        return false;
    }

    // 如果是动态材质实例，不能优化
    if (Material->IsA<UMaterialInstanceDynamic>())
    {
        return false;
    }

    // 检查是否是基础材质
    UMaterial* BaseMaterial = Cast<UMaterial>(Material);
    if (!BaseMaterial)
    {
        return false;
    }

    return true;
}

FString FMaterialOptimizer::GetMaterialParameterInfo(UMaterialInterface* Material)
{
    if (!Material)
    {
        return TEXT("Invalid Material");
    }

    FString Info;

    UMaterialInstance* MaterialInstance = Cast<UMaterialInstance>(Material);
    if (MaterialInstance)
    {
        // 获取标量参数数量
        TArray<FMaterialParameterInfo> ScalarParams;
        TArray<FGuid> ScalarGuids;
        MaterialInstance->GetAllScalarParameterInfo(ScalarParams, ScalarGuids);
        
        // 获取向量参数数量
        TArray<FMaterialParameterInfo> VectorParams;
        TArray<FGuid> VectorGuids;
        MaterialInstance->GetAllVectorParameterInfo(VectorParams, VectorGuids);
        
        // 获取纹理参数数量
        TArray<FMaterialParameterInfo> TextureParams;
        TArray<FGuid> TextureGuids;
        MaterialInstance->GetAllTextureParameterInfo(TextureParams, TextureGuids);

        Info = FString::Printf(
            TEXT("Scalar: %d, Vector: %d, Texture: %d"),
            ScalarParams.Num(),
            VectorParams.Num(),
            TextureParams.Num()
        );
    }
    else
    {
        Info = TEXT("Base Material (No Parameters)");
    }

    return Info;
}

bool FMaterialOptimizer::EnsureSavePathExists(const FString& Path)
{
    // 转换为文件系统路径
    FString FilePath = FPackageName::LongPackageNameToFilename(Path);
    
    // 确保目录存在
    if (!IFileManager::Get().DirectoryExists(*FilePath))
    {
        if (!IFileManager::Get().MakeDirectory(*FilePath, true))
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to create directory: %s"), *FilePath);
            return false;
        }
    } 
    
    // 转换为文件系统路径
    FString ContentDir = FPaths::ProjectContentDir();
    FString RelativePath = FilePath;
    RelativePath.RemoveFromStart(TEXT("/Game/"));
    
    FString FullPath = FPaths::Combine(ContentDir, RelativePath);

    // 创建目录
    if (!FPaths::DirectoryExists(FullPath))
    {
        if (!IFileManager::Get().MakeDirectory(*FullPath, true))
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to create directory: %s"), *FullPath);
            return false;
        }
        
        UE_LOG(LogTemp, Log, TEXT("Created directory: %s"), *FullPath);
    }
    
    return true;
}

FMaterialOptimizationResult FMaterialOptimizer::OptimizeMaterial(
    UMaterialInterface* Material,
    FMaterialOptimizationTransaction* Transaction)
{
    FMaterialOptimizationResult Result;
    
    if (!Material)
    {
        Result.bSuccess = false;
        Result.ErrorMessage = TEXT("Invalid material");
        return Result;
    }
    
    // 检查是否已经是材质实例
    if (Cast<UMaterialInstanceConstant>(Material))
    {
        Result.bAlreadyOptimized = true;
        Result.bSuccess = true;
        Result.ErrorMessage = TEXT("Material is already a material instance");
        return Result;
    }
    
    // 使用默认设置创建材质实例
    FMaterialOptimizerSettings Settings;
    UMaterialInstanceConstant* NewInstance = CreateMaterialInstance(Material, Settings);
    
    if (!NewInstance)
    {
        Result.bSuccess = false;
        Result.ErrorMessage = TEXT("Failed to create material instance");
        return Result;
    }
    
    Result.OriginalMaterial = Material;
    Result.MaterialInstance = NewInstance;
    
    // 替换场景中的材质
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        Result.bSuccess = false;
        Result.ErrorMessage = TEXT("No valid world");
        return Result;
    }
    
    int32 ActorCount = 0;
    
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (!Actor) continue;
        
        bool bReplacedOnActor = false;
        
        // 处理静态网格组件
        TArray<UStaticMeshComponent*> StaticMeshComponents;
        Actor->GetComponents<UStaticMeshComponent>(StaticMeshComponents);
        
        for (UStaticMeshComponent* Component : StaticMeshComponents)
        {
            if (!Component) continue;
            
            for (int32 i = 0; i < Component->GetNumMaterials(); ++i)
            {
                if (Component->GetMaterial(i) == Material)
                {
                    // 记录到事务
                    if (Transaction && Transaction->IsTransactionActive())
                    {
                        Transaction->RecordMaterialReplacement(Actor, Component, NewInstance);
                    }
                    
                    Component->SetMaterial(i, NewInstance);
                    bReplacedOnActor = true;
                }
            }
        }
        
        // 处理骨骼网格组件
        TArray<USkeletalMeshComponent*> SkeletalMeshComponents;
        Actor->GetComponents<USkeletalMeshComponent>(SkeletalMeshComponents);
        
        for (USkeletalMeshComponent* Component : SkeletalMeshComponents)
        {
            if (!Component) continue;
            
            for (int32 i = 0; i < Component->GetNumMaterials(); ++i)
            {
                if (Component->GetMaterial(i) == Material)
                {
                    // 记录到事务
                    if (Transaction && Transaction->IsTransactionActive())
                    {
                        Transaction->RecordMaterialReplacement(Actor, Component, NewInstance);
                    }
                    
                    Component->SetMaterial(i, NewInstance);
                    bReplacedOnActor = true;
                }
            }
        }
        
        if (bReplacedOnActor)
        {
            ActorCount++;
            Actor->MarkPackageDirty();
        }
    }
    
    Result.ActorCount = ActorCount;
    Result.bSuccess = true;
    Result.ErrorMessage = TEXT("Material optimized successfully");
    
    return Result;
}