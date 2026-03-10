
// Copyright Epic Games, Inc. All Rights Reserved.

#include "PerformanceAnalyzer.h"

#include "AudioMixerBlueprintLibrary.h"
#include "PerformanceAnalyzerStyle.h"
#include "PerformanceAnalyzerCommands.h"
#include "PerformanceProfiler.h"
#include "PerformanceMonitor.h"
#include "SPerformanceMonitorWidget.h"
#include "LevelEditor.h"
#include "ToolMenus.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Framework/Docking/TabManager.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/MessageDialog.h"
#include "AutoLODGenerator.h" 
#include "Selection.h"
#include "SLODGeneratorWidget.h"
#include "SMaterialOptimizerWidget.h"
#include "MaterialOptimizer.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "MaterialOptimizer.h"
#include "SMaterialOptimizerWidget.h"
#include "EngineUtils.h"

static const FName PerformanceAnalyzerTabName("PerformanceAnalyzer");
static const FName PerformanceMonitorTabName("PerformanceMonitor");
static const FName LODGeneratorTabName("LODGenerator");
static const FName MaterialOptimizerTabName("MaterialOptimizer");

#define LOCTEXT_NAMESPACE "FPerformanceAnalyzerModule"

void FPerformanceAnalyzerModule::StartupModule()
{
    // This code will execute after your module is loaded into memory
    
    FPerformanceAnalyzerStyle::Initialize();
    FPerformanceAnalyzerStyle::ReloadTextures();

    FPerformanceAnalyzerCommands::Register();
    
    PluginCommands = MakeShareable(new FUICommandList);

    // 映射命令
    PluginCommands->MapAction(
        FPerformanceAnalyzerCommands::Get().OpenPluginWindow,
        FExecuteAction::CreateRaw(this, &FPerformanceAnalyzerModule::PluginButtonClicked),
        FCanExecuteAction());

    PluginCommands->MapAction(
        FPerformanceAnalyzerCommands::Get().AnalyzeScene,
        FExecuteAction::CreateRaw(this, &FPerformanceAnalyzerModule::AnalyzeScene),
        FCanExecuteAction());
    
    PluginCommands->MapAction(
        FPerformanceAnalyzerCommands::Get().ExportReport,
        FExecuteAction::CreateRaw(this, &FPerformanceAnalyzerModule::ExportReport),
        FCanExecuteAction());
    
    PluginCommands->MapAction(
        FPerformanceAnalyzerCommands::Get().GenerateLODsAuto,
        FExecuteAction::CreateRaw(this, &FPerformanceAnalyzerModule::GenerateLODsAuto),
        FCanExecuteAction());
    
    PluginCommands->MapAction(
        FPerformanceAnalyzerCommands::Get().GenerateLODsBalanced,
        FExecuteAction::CreateRaw(this, &FPerformanceAnalyzerModule::GenerateLODsBalanced),
        FCanExecuteAction());
    
    PluginCommands->MapAction(
        FPerformanceAnalyzerCommands::Get().OpenLODGenerator,
        FExecuteAction::CreateRaw(this, &FPerformanceAnalyzerModule::OpenLODGeneratorWindow),
        FCanExecuteAction());
    
    PluginCommands->MapAction(
        FPerformanceAnalyzerCommands::Get().OpenMaterialOptimizer,
        FExecuteAction::CreateRaw(this,&FPerformanceAnalyzerModule::OpenMaterialOptimizerWindow),
        FCanExecuteAction()
    );
    
    PluginCommands->MapAction(
        FPerformanceAnalyzerCommands::Get().OptimizeSelectedMaterials,
        FExecuteAction::CreateRaw(this,&FPerformanceAnalyzerModule::OptimizeSelectedMaterials),
        FCanExecuteAction()
    );
    
    PluginCommands->MapAction(
        FPerformanceAnalyzerCommands::Get().ScanSceneMaterials,
        FExecuteAction::CreateRaw(this,&FPerformanceAnalyzerModule::ScanSceneMaterials),
        FCanExecuteAction()
    );
    // 创建性能监控器实例
    PerformanceMonitor = MakeShareable(new FPerformanceMonitor());
    
    // 注册Tab生成器
    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
        PerformanceAnalyzerTabName, 
        FOnSpawnTab::CreateRaw(this, &FPerformanceAnalyzerModule::OnSpawnPluginTab))
        .SetDisplayName(LOCTEXT("FPerformanceAnalyzerTabTitle", "Performance Analyzer"))
        .SetMenuType(ETabSpawnerMenuType::Hidden);
    
    // 注册性能监控Tab
    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
        PerformanceMonitorTabName,
        FOnSpawnTab::CreateRaw(this, &FPerformanceAnalyzerModule::OnSpawnMonitorTab))
        .SetDisplayName(LOCTEXT("MonitorTabTitle", "Performance Monitor"))
        .SetMenuType(ETabSpawnerMenuType::Hidden);
   
    // 注册LOD生成Tab
    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
        LODGeneratorTabName,
        FOnSpawnTab::CreateRaw(this, &FPerformanceAnalyzerModule::OnSpawnLODGeneratorTab))
        .SetDisplayName(LOCTEXT("LODGeneratorTabTitle", "LOD Generator"))
        .SetMenuType(ETabSpawnerMenuType::Hidden);
    
    // 注册材质优化器Tab
    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
        MaterialOptimizerTabName,
        FOnSpawnTab::CreateRaw(this, &FPerformanceAnalyzerModule::OnSpawnMaterialOptimizerTab))
        .SetDisplayName(LOCTEXT("MaterialOptimizerTabTitle", "Material Optimizer"))
        .SetMenuType(ETabSpawnerMenuType::Hidden);
    
    UToolMenus::RegisterStartupCallback(
        FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FPerformanceAnalyzerModule::RegisterMenus));
    
    IConsoleManager::Get().RegisterConsoleCommand(
       TEXT("PerfMon.TestSpawn"),
       TEXT("Test actor spawned callback"),
       FConsoleCommandDelegate::CreateLambda([this]()
       {
           if (PerformanceMonitor)
           {
               UWorld* World = GEngine->GetWorld();
               if (World)
               {
                   AActor* TestActor = World->SpawnActor<AActor>();
                   PerformanceMonitor->TestActorSpawnedCallback(TestActor);
                   UE_LOG(LogTemp, Log, TEXT("Test: Spawned actor %s"), *TestActor->GetName());
               }
           }
       })
   );
    
    IConsoleManager::Get().RegisterConsoleCommand(
        TEXT("PerfMon.TestDestroy"),
        TEXT("Test actor destroyed callback"),
        FConsoleCommandDelegate::CreateLambda([this]()
        {
            if (PerformanceMonitor)
            {
                UWorld* World = GEngine->GetWorld();
                if (World)
                {
                    // 销毁最后一个生成的Actor
                    for (TActorIterator<AActor> It(World); It; ++It)
                    {
                        if (*It && !It->IsA<APlayerController>())
                        {
                            PerformanceMonitor->TestActorDestroyedCallback(*It);
                            It->Destroy();
                            UE_LOG(LogTemp, Log, TEXT("Test: Destroyed actor %s"), *It->GetName());
                            break;
                        }
                    }
                }
            }
        })
    );
    
    UE_LOG(LogTemp, Log, TEXT("PerformanceAnalyzer Module Started"));
}

void FPerformanceAnalyzerModule::OptimizeSelectedMaterials()
{
    UE_LOG(LogTemp, Warning, TEXT("\n========================================"));
    UE_LOG(LogTemp, Warning, TEXT("   MATERIAL OPTIMIZATION"));
    UE_LOG(LogTemp, Warning, TEXT("========================================\n"));
    
    // 获取选中的材质
    TArray<UMaterialInterface*> SelectedMaterials;
    
    // 从内容浏览器获取选中的材质
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    TArray<FAssetData> SelectedAssets;
    GEditor->GetContentBrowserSelections(SelectedAssets);
    
    for (const FAssetData& AssetData : SelectedAssets)
    {
        if (UMaterialInterface* Material = Cast<UMaterialInterface>(AssetData.GetAsset()))
        {
            SelectedMaterials.Add(Material);
        }
    }
    
    if (SelectedMaterials.Num() == 0)
    {
        FText Message = LOCTEXT("NoMaterialsSelected", "Please select one or more Materials in the Content Browser!");
        FMessageDialog::Open(EAppMsgType::Ok, Message);
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("Found %d materials to optimize"), SelectedMaterials.Num());
    
    // 批量优化材质
    int32 SuccessCount = 0;
    int32 SkippedCount = 0;
    
    for (UMaterialInterface* Material : SelectedMaterials)
    {
        FMaterialOptimizationResult Result = FMaterialOptimizer::OptimizeMaterial(Material);
        
        if (Result.bSuccess)
        {
            SuccessCount++;
            UE_LOG(LogTemp, Log, TEXT("✓ %s: Optimized successfully"), *Material->GetName());
        }
        else if (Result.bAlreadyOptimized)
        {
            SkippedCount++;
            UE_LOG(LogTemp, Log, TEXT("○ %s: Already optimized"), *Material->GetName());
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("✗ %s: %s"), *Material->GetName(), *Result.ErrorMessage);
        }
    }
    
    // 显示结果
    UE_LOG(LogTemp, Warning, TEXT("\n--- OPTIMIZATION SUMMARY ---"));
    UE_LOG(LogTemp, Warning, TEXT("Total Materials: %d"), SelectedMaterials.Num());
    UE_LOG(LogTemp, Warning, TEXT("Optimized: %d"), SuccessCount);
    UE_LOG(LogTemp, Warning, TEXT("Skipped: %d"), SkippedCount);
    UE_LOG(LogTemp, Warning, TEXT("========================================\n"));
    
    FString SummaryText = FString::Printf(
        TEXT("Material Optimization Complete!\n\n")
        TEXT("Total Materials: %d\n")
        TEXT("Successfully Optimized: %d\n")
        TEXT("Skipped (already optimized): %d\n\n")
        TEXT("Check Output Log for detailed results."),
        SelectedMaterials.Num(),
        SuccessCount,
        SkippedCount
    );
    
    FText Title = LOCTEXT("MaterialOptimizationComplete", "Material Optimization Complete");
    FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(SummaryText), Title);
}

void FPerformanceAnalyzerModule::ScanSceneMaterials()
{
    UE_LOG(LogTemp, Warning, TEXT("\n========================================"));
    UE_LOG(LogTemp, Warning, TEXT("   SCANNING SCENE MATERIALS"));
    UE_LOG(LogTemp, Warning, TEXT("========================================\n"));
    
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        FText Message = LOCTEXT("NoWorld", "No valid world found!");
        FMessageDialog::Open(EAppMsgType::Ok, Message);
        return;
    }
    
    // 扫描场景中的材质
    TArray<FMaterialUsageInfo> MaterialUsages = FMaterialOptimizer::ScanSceneMaterials(World, false);
    
    // 统计结果
    int32 TotalMaterials = MaterialUsages.Num();
    int32 OptimizableCount = 0;
    int32 AlreadyOptimized = 0;
    
    TArray<UMaterialInterface*> OptimizableMaterials;
    
    for (const FMaterialUsageInfo& Usage : MaterialUsages)
    {
        if (Usage.bIsInstance)
        {
            AlreadyOptimized++;
        }
        else if (Usage.bCanOptimize)
        {
            OptimizableCount++;
            if (Usage.Material.IsValid())
            {
                OptimizableMaterials.Add(Usage.Material.Get());
            }
        }
    }
    
    // 输出结果
    UE_LOG(LogTemp, Warning, TEXT("--- SCAN RESULTS ---"));
    UE_LOG(LogTemp, Warning, TEXT("Total Materials: %d"), TotalMaterials);
    UE_LOG(LogTemp, Warning, TEXT("Can Be Optimized: %d"), OptimizableCount);
    UE_LOG(LogTemp, Warning, TEXT("Already Optimized: %d"), AlreadyOptimized);
    
    if (OptimizableMaterials.Num() > 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("\n--- MATERIALS THAT CAN BE OPTIMIZED ---"));
        for (int32 i = 0; i < FMath::Min(10, OptimizableMaterials.Num()); i++)
        {
            UE_LOG(LogTemp, Warning, TEXT("  %s"), *OptimizableMaterials[i]->GetName());
        }
        if (OptimizableMaterials.Num() > 10)
        {
            UE_LOG(LogTemp, Warning, TEXT("  ... and %d more"), 
                OptimizableMaterials.Num() - 10);
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("========================================\n"));
    
    // 显示对话框
    FString SummaryText = FString::Printf(
        TEXT("Material Scan Complete!\n\n")
        TEXT("Total Materials in Scene: %d\n")
        TEXT("Can Be Optimized: %d\n")
        TEXT("Already Optimized: %d\n\n")
        TEXT("Open Material Optimizer window to optimize materials."),
        TotalMaterials,
        OptimizableCount,
        AlreadyOptimized
    );
    
    FText Title = LOCTEXT("MaterialScanComplete", "Material Scan Complete");
    FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(SummaryText), Title);
}
void FPerformanceAnalyzerModule::ShutdownModule()
{
    // This function may be called during shutdown to clean up your module
    
    UToolMenus::UnRegisterStartupCallback(this);
    UToolMenus::UnregisterOwner(this);

    FPerformanceAnalyzerStyle::Shutdown();
    FPerformanceAnalyzerCommands::Unregister();

    // 注销Tab
    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(PerformanceAnalyzerTabName);
    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(PerformanceMonitorTabName);
    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(LODGeneratorTabName);
    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(MaterialOptimizerTabName);
    
    // 清理监控器
    if (PerformanceMonitor.IsValid())
    {
        PerformanceMonitor->SetEnabled(false);
        PerformanceMonitor.Reset();
    }
    
    UE_LOG(LogTemp, Log, TEXT("PerformanceAnalyzer Module Shutdown"));
}

TSharedRef<SDockTab> FPerformanceAnalyzerModule::OnSpawnMaterialOptimizerTab(const FSpawnTabArgs& Args)
{
    return SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
        [
            SNew(SBox)
            .HAlign(HAlign_Fill)
            .VAlign(VAlign_Fill)
            [
                SNew(SMaterialOptimizerWidget)
            ]
        ];
}

void FPerformanceAnalyzerModule::OpenMaterialOptimizerWindow()
{
    FGlobalTabmanager::Get()->TryInvokeTab(MaterialOptimizerTabName);
}



TSharedRef<SDockTab> FPerformanceAnalyzerModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
    FText WidgetText = FText::Format(
        LOCTEXT("WindowWidgetText",  
        "Performance Analyzer Dashboard\n\n"
        "Commands:\n"
        "- Ctrl+Shift+P: Analyze Scene\n"
        "- Tools Menu: Export Report\n"
        "- Window Menu: Performance Monitor (Real-time monitoring)"),
        FText::FromString(TEXT("FPerformanceAnalyzerModule::OnSpawnPluginTab")),
        FText::FromString(TEXT("PerformanceAnalyzer.cpp"))
    );

    return SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
        [
            SNew(SBox)
            .HAlign(HAlign_Center)
            .VAlign(VAlign_Center)
            [
                SNew(STextBlock)
                .Text(WidgetText)
            ]
        ];
}

TSharedRef<SDockTab> FPerformanceAnalyzerModule::OnSpawnMonitorTab(const FSpawnTabArgs& Args)
{
    return SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
        [
            SNew(SPerformanceMonitorWidget, PerformanceMonitor)
        ];
}

TSharedRef<SDockTab> FPerformanceAnalyzerModule::OnSpawnLODGeneratorTab(const FSpawnTabArgs& Args)
{
    return SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
        [
            SNew(SBox)
            .HAlign(HAlign_Center)
            .VAlign(VAlign_Center)
            [
                SNew(SLODGeneratorWidget)
            ]
        ];
}

void FPerformanceAnalyzerModule::PluginButtonClicked()
{
    FGlobalTabmanager::Get()->TryInvokeTab(PerformanceAnalyzerTabName);
}

void FPerformanceAnalyzerModule::OnOpenMonitorWindow()
{
    FGlobalTabmanager::Get()->TryInvokeTab(PerformanceMonitorTabName);
}

void FPerformanceAnalyzerModule::OpenLODGeneratorWindow()
{
    FGlobalTabmanager::Get()->TryInvokeTab(LODGeneratorTabName);
}
void FPerformanceAnalyzerModule::GenerateLODsAuto()
{
    UE_LOG(LogTemp, Warning, TEXT("\n========================================"));
    UE_LOG(LogTemp, Warning, TEXT("   AUTO LOD GENERATION"));
    UE_LOG(LogTemp, Warning, TEXT("========================================\n"));
    
    // 获取选中的 Actor
    USelection* SelectedActors = GEditor->GetSelectedActors();
    if (!SelectedActors || SelectedActors->Num() == 0)
    {
        FText Message = LOCTEXT("NoSelection", "Please select one or more Static Mesh Actors!");
        FMessageDialog::Open(EAppMsgType::Ok, Message);
        return;
    }
    
    // 收集所有静态网格
    TArray<UStaticMesh*> MeshesToProcess;
    TSet<UStaticMesh*> ProcessedMeshes;  // 避免重复
    
    for (FSelectionIterator It(*SelectedActors); It; ++It)
    {
        AActor* Actor = Cast<AActor>(*It);
        
        if (!Actor)
        {
            continue;
        }
        
        // 获取 StaticMeshComponent
        TArray<UStaticMeshComponent*> MeshComponents;
        Actor->GetComponents<UStaticMeshComponent>(MeshComponents);
        
        for (UStaticMeshComponent* MeshComp : MeshComponents)
        {
            if (MeshComp && MeshComp->GetStaticMesh())
            {
                UStaticMesh* Mesh = MeshComp->GetStaticMesh();
                
                // 避免重复处理同一个网格
                if (!ProcessedMeshes.Contains(Mesh))
                {
                    ProcessedMeshes.Add(Mesh);
                    MeshesToProcess.Add(Mesh);
                }
            }
        }
    }
    
    if (MeshesToProcess.Num() == 0)
    {
        FText Message = LOCTEXT("NoMeshes", "No valid Static Meshes found in selection!");
        FMessageDialog::Open(EAppMsgType::Ok, Message);
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("Found %d unique meshes to process"), MeshesToProcess.Num());
    
    // 批量生成 LOD
    int32 SuccessCount = 0;
    int32 SkippedCount = 0;
    int32 FailedCount = 0;
    
    for (UStaticMesh* Mesh : MeshesToProcess)
    {
        // 检查是否需要生成
        if (!FAutoLODGenerator::NeedsLODGeneration(Mesh))
        {
            UE_LOG(LogTemp, Log, TEXT("Skipping %s (already has LODs or too simple)"), 
                *Mesh->GetName());
            SkippedCount++;
            continue;
        }
        
        // 自动生成 LOD
        FLODGenerationResult Result = FAutoLODGenerator::GenerateLODsAuto(Mesh);
        
        if (Result.bSuccess)
        {
            SuccessCount++;
            
            UE_LOG(LogTemp, Log, TEXT("✓ %s: Generated %d LODs, Performance gain: %.1f%%"),
                *Mesh->GetName(),
                Result.LODTriangleCounts.Num() - 1,
                Result.EstimatedPerformanceGain * 100.0f);
        }
        else
        {
            FailedCount++;
            UE_LOG(LogTemp, Error, TEXT("✗ %s: Failed - %s"),
                *Mesh->GetName(),
                *Result.ErrorMessage);
        }
    }
    
    // 显示结果
    UE_LOG(LogTemp, Warning, TEXT("\n--- LOD GENERATION SUMMARY ---"));
    UE_LOG(LogTemp, Warning, TEXT("Total Meshes: %d"), MeshesToProcess.Num());
    UE_LOG(LogTemp, Warning, TEXT("Success: %d"), SuccessCount);
    UE_LOG(LogTemp, Warning, TEXT("Skipped: %d"), SkippedCount);
    UE_LOG(LogTemp, Warning, TEXT("Failed: %d"), FailedCount);
    UE_LOG(LogTemp, Warning, TEXT("========================================\n"));
    
    // 显示对话框
    FString SummaryText = FString::Printf(
        TEXT("LOD Generation Complete!\n\n")
        TEXT("Total Meshes: %d\n")
        TEXT("Successfully Generated: %d\n")
        TEXT("Skipped (already has LODs): %d\n")
        TEXT("Failed: %d\n\n")
        TEXT("Check Output Log for detailed results."),
        MeshesToProcess.Num(),
        SuccessCount,
        SkippedCount,
        FailedCount
    );
    
    FText Title = LOCTEXT("LODGenerationComplete", "LOD Generation Complete");
    FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(SummaryText), Title);
}

void FPerformanceAnalyzerModule::GenerateLODsBalanced()
{
    UE_LOG(LogTemp, Warning, TEXT("\n========================================"));
    UE_LOG(LogTemp, Warning, TEXT("   BALANCED LOD GENERATION"));
    UE_LOG(LogTemp, Warning, TEXT("========================================\n"));
    
    // 获取选中的 Actor
    USelection* SelectedActors = GEditor->GetSelectedActors();
    if (!SelectedActors || SelectedActors->Num() == 0)
    {
        FText Message = LOCTEXT("NoSelection", "Please select one or more Static Mesh Actors!");
        FMessageDialog::Open(EAppMsgType::Ok, Message);
        return;
    }
    
    // 收集所有静态网格
    TArray<UStaticMesh*> MeshesToProcess;
    TSet<UStaticMesh*> ProcessedMeshes;
    
    for (FSelectionIterator It(*SelectedActors); It; ++It)
    {
        AActor* Actor = Cast<AActor>(*It);
        if (!Actor)
        {
            continue;
        }
        
        TArray<UStaticMeshComponent*> MeshComponents;
        Actor->GetComponents<UStaticMeshComponent>(MeshComponents);
        
        for (UStaticMeshComponent* MeshComp : MeshComponents)
        {
            if (MeshComp && MeshComp->GetStaticMesh())
            {
                UStaticMesh* Mesh = MeshComp->GetStaticMesh();
                if (!ProcessedMeshes.Contains(Mesh))
                {
                    ProcessedMeshes.Add(Mesh);
                    MeshesToProcess.Add(Mesh);
                }
            }
        }
    }
    
    if (MeshesToProcess.Num() == 0)
    {
        FText Message = LOCTEXT("NoMeshes", "No valid Static Meshes found in selection!");
        FMessageDialog::Open(EAppMsgType::Ok, Message);
        return;
    }
    
    // 使用平衡预设批量生成
    int32 SuccessCount = FAutoLODGenerator::BatchGenerateLODs(
        MeshesToProcess, 
        ELODPreset::Balanced
    );
    
    // 显示结果
    UE_LOG(LogTemp, Warning, TEXT("\n--- BALANCED LOD GENERATION SUMMARY ---"));
    UE_LOG(LogTemp, Warning, TEXT("Total Meshes: %d"), MeshesToProcess.Num());
    UE_LOG(LogTemp, Warning, TEXT("Success: %d"), SuccessCount);
    UE_LOG(LogTemp, Warning, TEXT("========================================\n"));
    
    FString SummaryText = FString::Printf(
        TEXT("Balanced LOD Generation Complete!\n\n")
        TEXT("Total Meshes: %d\n")
        TEXT("Successfully Generated: %d\n\n")
        TEXT("Preset Used: Balanced (4 LODs)\n")
        TEXT("- LOD1: 50%% triangles\n")
        TEXT("- LOD2: 25%% triangles\n")
        TEXT("- LOD3: 12.5%% triangles\n")
        TEXT("- LOD4: 6.25%% triangles"),
        MeshesToProcess.Num(),
        SuccessCount
    );
    
    FText Title = LOCTEXT("BalancedLODComplete", "Balanced LOD Generation Complete");
    FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(SummaryText), Title);
}


void FPerformanceAnalyzerModule::AnalyzeScene()
{
    UE_LOG(LogTemp, Warning, TEXT("\n========================================"));
    UE_LOG(LogTemp, Warning, TEXT("   STARTING PERFORMANCE ANALYSIS"));
    UE_LOG(LogTemp, Warning, TEXT("========================================\n"));
    
    // 执行分析
    FPerformanceReport Report = UPerformanceProfiler::AnalyzeCurrentScene();
    
    // 输出总览
    UE_LOG(LogTemp, Warning, TEXT("--- OVERVIEW ---"));
    UE_LOG(LogTemp, Warning, TEXT("Total Actors: %d"), Report.TotalActors);
    UE_LOG(LogTemp, Warning, TEXT("Total Static Meshes: %d"), Report.TotalStaticMeshes);
    UE_LOG(LogTemp, Warning, TEXT("Total Triangles: %d"), Report.TotalTriangles);
    UE_LOG(LogTemp, Warning, TEXT("Frame Time: %.2f ms"), Report.FrameTimeMS);
    
    // 输出问题统计
    UE_LOG(LogTemp, Warning, TEXT("\n--- ISSUE SUMMARY ---"));
    UE_LOG(LogTemp, Warning, TEXT("High Poly Meshes: %d"), Report.HighPolyMeshCount);
    UE_LOG(LogTemp, Warning, TEXT("Meshes Without LODs: %d"), Report.MeshesWithoutLODs);
    UE_LOG(LogTemp, Warning, TEXT("Total Issues: %d"), Report.Issues.Num());
    
    // 按严重程度统计
    int32 HighSeverity = 0;
    int32 MediumSeverity = 0;
    int32 LowSeverity = 0;
    
    for (const FPerformanceIssue& Issue : Report.Issues)
    {
        if (Issue.Severity == 3) HighSeverity++;
        else if (Issue.Severity == 2) MediumSeverity++;
        else LowSeverity++;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("\n--- SEVERITY BREAKDOWN ---"));
    UE_LOG(LogTemp, Warning, TEXT("High Severity: %d"), HighSeverity);
    UE_LOG(LogTemp, Warning, TEXT("Medium Severity: %d"), MediumSeverity);
    UE_LOG(LogTemp, Warning, TEXT("Low Severity: %d"), LowSeverity);
    
    // 显示前10个高严重度问题
    if (HighSeverity > 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("\n--- TOP HIGH SEVERITY ISSUES ---"));
        int32 Count = 0;
        for (const FPerformanceIssue& Issue : Report.Issues)
        {
            if (Issue.Severity == 3 && Count < 10)
            {
                UE_LOG(LogTemp, Warning, TEXT("[%s] %s"), *Issue.IssueType, *Issue.ActorName);
                UE_LOG(LogTemp, Warning, TEXT("  %s"), *Issue.Description);
                Count++;
            }
        }
        if (HighSeverity > 10)
        {
            UE_LOG(LogTemp, Warning, TEXT("  ... and %d more high severity issues"), HighSeverity - 10);
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("\n========================================"));
    UE_LOG(LogTemp, Warning, TEXT("   ANALYSIS COMPLETE"));
    UE_LOG(LogTemp, Warning, TEXT("========================================\n"));
    UE_LOG(LogTemp, Warning, TEXT("Use 'Tools -> Export Report' to save detailed results"));
    
    // 显示消息对话框
    FString SummaryText = FString::Printf(
        TEXT("Performance Analysis Complete!\n\n")
        TEXT("Total Actors: %d\n")
        TEXT("Total Static Meshes: %d\n")
        TEXT("Total Triangles: %d\n")
        TEXT("Frame Time: %.2f ms\n\n")
        TEXT("Issues Found:\n")
        TEXT("  High Severity: %d\n")
        TEXT("  Medium Severity: %d\n")
        TEXT("  Low Severity: %d\n\n")
        TEXT("Check Output Log for detailed results."),
        Report.TotalActors,
        Report.TotalStaticMeshes,
        Report.TotalTriangles,
        Report.FrameTimeMS,
        HighSeverity,
        MediumSeverity,
        LowSeverity
    );
    
    FText Title = LOCTEXT("AnalysisComplete", "Performance Analysis Complete");
    FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(SummaryText), Title);
}

void FPerformanceAnalyzerModule::ExportReport()
{
    UE_LOG(LogTemp, Warning, TEXT("Exporting Performance Report..."));
    
    // 执行分析
    FPerformanceReport Report = UPerformanceProfiler::AnalyzeCurrentScene();
    
    // 生成输出路径
    FString ProjectDir = FPaths::ProjectDir();
    FString OutputDir = FPaths::Combine(ProjectDir, TEXT("Saved"), TEXT("PerformanceReports"));
    
        // 确保目录存在
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    if (!PlatformFile.DirectoryExists(*OutputDir))
    {
        PlatformFile.CreateDirectory(*OutputDir);
    }
    
    // 生成文件名
    FString FileName = FString::Printf(
        TEXT("PerformanceReport_%s.txt"),
        *FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"))
    );
    FString FullPath = FPaths::Combine(OutputDir, FileName);
    
    // 导出报告
    bool bSuccess = UPerformanceProfiler::ExportReport(Report, FullPath);
    
    if (bSuccess)
    {
        UE_LOG(LogTemp, Warning, TEXT("Report exported to: %s"), *FullPath);
        
        FText Message = FText::Format(
            LOCTEXT("ExportSuccess", "Report exported successfully to:\n{0}"),
            FText::FromString(FullPath)
        );
        FMessageDialog::Open(EAppMsgType::Ok, Message);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to export report"));
        
        FText Message = LOCTEXT("ExportFailed", "Failed to export report!");
        FMessageDialog::Open(EAppMsgType::Ok, Message);
    }
}

void FPerformanceAnalyzerModule::RegisterMenus()
{
    // Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
    FToolMenuOwnerScoped OwnerScoped(this);

    // 扩展 Window 菜单
    {
        UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
        {
            FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
            
            // 添加 Performance Analyzer 窗口
            Section.AddMenuEntryWithCommandList(
                FPerformanceAnalyzerCommands::Get().OpenPluginWindow, 
                PluginCommands);
            
            // 添加 Performance Monitor 窗口
            Section.AddMenuEntry(
                "OpenPerformanceMonitor",
                LOCTEXT("OpenPerformanceMonitor", "Performance Monitor"),
                LOCTEXT("OpenPerformanceMonitorTooltip", "Open the real-time performance monitoring window"),
                FSlateIcon(FPerformanceAnalyzerStyle::GetStyleSetName(), "PerformanceAnalyzer.OpenPluginWindow"),
                FUIAction(FExecuteAction::CreateRaw(this, &FPerformanceAnalyzerModule::OnOpenMonitorWindow))
            );
            
            Section.AddMenuEntry(
              "LODGenerator",
              LOCTEXT("LODGeneratorTitle", "LOD Generator"),
              LOCTEXT("LODGeneratorTooltip", "Open LOD Generator window"),
              FSlateIcon(FPerformanceAnalyzerStyle::GetStyleSetName(), "PerformanceAnalyzer.OpenPluginWindow"),
              FUIAction(FExecuteAction::CreateRaw(this, &FPerformanceAnalyzerModule::OpenLODGeneratorWindow))
            );

             // 添加材质优化器窗口
            Section.AddMenuEntry(
                "OpenMaterialOptimizer",
                LOCTEXT("OpenMaterialOptimizer", "Material Optimizer"),
                LOCTEXT("OpenMaterialOptimizerTooltip", "Open the material optimization tool"),
                FSlateIcon(FPerformanceAnalyzerStyle::GetStyleSetName(), "PerformanceAnalyzer.OpenPluginWindow"),
                FUIAction(FExecuteAction::CreateRaw(this, &FPerformanceAnalyzerModule::OpenMaterialOptimizerWindow))
            );
        }
    }

    // 扩展工具栏
    {
        UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");
        {
            FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("PluginTools");
            {
                FToolMenuEntry& Entry = Section.AddEntry(
                    FToolMenuEntry::InitToolBarButton(FPerformanceAnalyzerCommands::Get().OpenPluginWindow));
                Entry.SetCommandList(PluginCommands);
            }
        }
    }
    
    // 添加到 Tools 菜单
    {
        UToolMenu* ToolsMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools");
        {
            FToolMenuSection& Section = ToolsMenu->FindOrAddSection("Performance");
            Section.Label = LOCTEXT("PerformanceSection", "Performance");
            
            Section.AddMenuEntryWithCommandList(
                FPerformanceAnalyzerCommands::Get().AnalyzeScene,
                PluginCommands);
            
            Section.AddMenuEntryWithCommandList(
                FPerformanceAnalyzerCommands::Get().ExportReport,
                PluginCommands);
            
            Section.AddSeparator(NAME_None);
            
            // LOD 生成功能
            Section.AddMenuEntryWithCommandList(
                FPerformanceAnalyzerCommands::Get().GenerateLODsAuto,
                PluginCommands);
            
            Section.AddMenuEntryWithCommandList(
                FPerformanceAnalyzerCommands::Get().GenerateLODsBalanced,
                PluginCommands);
            
            Section.AddMenuEntryWithCommandList(
                FPerformanceAnalyzerCommands::Get().OpenLODGenerator,
                PluginCommands);
            
            Section.AddMenuEntryWithCommandList(
               FPerformanceAnalyzerCommands::Get().OptimizeSelectedMaterials,
               PluginCommands);
            
            Section.AddMenuEntryWithCommandList(
                FPerformanceAnalyzerCommands::Get().ScanSceneMaterials,
                PluginCommands);
            
            Section.AddMenuEntryWithCommandList(
                FPerformanceAnalyzerCommands::Get().OpenMaterialOptimizer,
                PluginCommands);
        }
    }
    
    
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FPerformanceAnalyzerModule, PerformanceAnalyzer)