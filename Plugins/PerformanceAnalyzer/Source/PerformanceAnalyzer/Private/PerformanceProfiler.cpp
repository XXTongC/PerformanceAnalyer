#include "PerformanceProfiler.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "EngineUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformMemory.h"


FPerformanceReport UPerformanceProfiler::AnalyzeCurrentScene()
{
    FPerformanceReport Report;
    
    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    
    if (!World)
    {
        Report.Warnings.Add(TEXT("No valid world found"));
        UE_LOG(LogTemp, Error, TEXT("PerformanceProfiler: No valid world"));
        return Report;
    }
    
    UE_LOG(LogTemp, Log, TEXT("Starting comprehensive scene analysis..."));
    
    // 统计基本信息
    int32 ActorCount = 0;
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        ActorCount++;
    }
    
    Report.TotalActors = ActorCount;
    Report.FrameTimeMS = FApp::GetDeltaTime() * 1000.0f;
    
    // 执行各项分析
    AnalyzeStaticMeshes(World, Report);
    AnalyzeMaterials(World, Report);
    DetectDrawCallHotspots(World, Report);
    
    // 生成总结
    Report.Warnings.Add(FString::Printf(TEXT("Total Actors: %d"), Report.TotalActors));
    Report.Warnings.Add(FString::Printf(TEXT("Total Static Meshes: %d"), Report.TotalStaticMeshes));
    Report.Warnings.Add(FString::Printf(TEXT("Total Triangles: %d"), Report.TotalTriangles));
    Report.Warnings.Add(FString::Printf(TEXT("High Poly Meshes: %d"), Report.HighPolyMeshCount));
    Report.Warnings.Add(FString::Printf(TEXT("Meshes Without LODs: %d"), Report.MeshesWithoutLODs));
    Report.Warnings.Add(FString::Printf(TEXT("Frame Time: %.2f ms"), Report.FrameTimeMS));
    
    UE_LOG(LogTemp, Log, TEXT("Analysis complete - Found %d issues"), Report.Issues.Num());
    
    return Report;
}

void UPerformanceProfiler::AnalyzeStaticMeshes(UWorld* World, FPerformanceReport& Report)
{
    if (!World)
    {
        return;
    }
    
    TSet<UStaticMesh*> ProcessedMeshes;
    int32 TotalTriangles = 0;
    
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (!Actor)
        {
            continue;
        }
        
        TArray<UStaticMeshComponent*> MeshComponents;
        Actor->GetComponents<UStaticMeshComponent>(MeshComponents);
        
        Report.TotalStaticMeshes += MeshComponents.Num();
        
        for (UStaticMeshComponent* MeshComp : MeshComponents)
        {
            if (!MeshComp || !MeshComp->GetStaticMesh())
            {
                continue;
            }
            
            UStaticMesh* Mesh = MeshComp->GetStaticMesh();
            
            // 避免重复分析同一个网格
            if (ProcessedMeshes.Contains(Mesh))
            {
                continue;
            }
            ProcessedMeshes.Add(Mesh);
            
            // 检查LOD
            int32 NumLODs = Mesh->GetNumLODs();
            if (NumLODs < 2)
            {
                Report.MeshesWithoutLODs++;
                AddIssue(Report, Actor->GetName(), TEXT("Missing LODs"),
                    FString::Printf(TEXT("Mesh '%s' has no LOD levels"), *Mesh->GetName()), 2);
            }
            
            // 检查三角形数量
            if (Mesh->GetRenderData() && Mesh->GetRenderData()->LODResources.Num() > 0)
            {
                int32 TriangleCount = Mesh->GetRenderData()->LODResources[0].GetNumTriangles();
                TotalTriangles += TriangleCount;
                
                if (TriangleCount > 50000)
                {
                    Report.HighPolyMeshCount++;
                    AddIssue(Report, Actor->GetName(), TEXT("High Poly Count"),
                        FString::Printf(TEXT("Mesh '%s' has %d triangles"), *Mesh->GetName(), TriangleCount), 3);
                }
                else if (TriangleCount > 20000)
                {
                    AddIssue(Report, Actor->GetName(), TEXT("Medium Poly Count"),
                        FString::Printf(TEXT("Mesh '%s' has %d triangles"), *Mesh->GetName(), TriangleCount), 2);
                }
            }
        }
    }
    
    Report.TotalTriangles = TotalTriangles;
}

void UPerformanceProfiler::AnalyzeMaterials(UWorld* World, FPerformanceReport& Report)
{
    if (!World)
    {
        return;
    }
    
    TSet<UMaterialInterface*> ProcessedMaterials;
    
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (!Actor)
        {
            continue;
        }
        
        TArray<UStaticMeshComponent*> MeshComponents;
        Actor->GetComponents<UStaticMeshComponent>(MeshComponents);
        
        for (UStaticMeshComponent* MeshComp : MeshComponents)
        {
            if (!MeshComp)
            {
                continue;
            }
            
            int32 MaterialCount = MeshComp->GetNumMaterials();
            
            // 检查材质槽数量
            if (MaterialCount > 5)
            {
                AddIssue(Report, Actor->GetName(), TEXT("Too Many Materials"),
                    FString::Printf(TEXT("Component has %d material slots"), MaterialCount), 2);
            }
            
            // 分析每个材质
                        for (int32 i = 0; i < MaterialCount; ++i)
            {
                UMaterialInterface* Material = MeshComp->GetMaterial(i);
                if (!Material || ProcessedMaterials.Contains(Material))
                {
                    continue;
                }
                ProcessedMaterials.Add(Material);
                
                // 检查材质是否为实例
                if (!Material->IsA<UMaterialInstance>())
                {
                    AddIssue(Report, Actor->GetName(), TEXT("Non-Instanced Material"),
                        FString::Printf(TEXT("Material '%s' is not an instance"), *Material->GetName()), 1);
                }
            }
        }
    }
}

void UPerformanceProfiler::DetectDrawCallHotspots(UWorld* World, FPerformanceReport& Report)
{
    if (!World)
    {
        return;
    }
    
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (!Actor)
        {
            continue;
        }
        
        TArray<UStaticMeshComponent*> MeshComponents;
        Actor->GetComponents<UStaticMeshComponent>(MeshComponents);
        
        int32 TotalMaterialSlots = 0;
        for (UStaticMeshComponent* MeshComp : MeshComponents)
        {
            if (MeshComp)
            {
                TotalMaterialSlots += MeshComp->GetNumMaterials();
            }
        }
        
        // 检测高DrawCall的Actor
        if (TotalMaterialSlots > 10)
        {
            AddIssue(Report, Actor->GetName(), TEXT("High DrawCall Count"),
                FString::Printf(TEXT("Actor has %d material slots across %d components"), 
                    TotalMaterialSlots, MeshComponents.Num()), 3);
        }
        else if (TotalMaterialSlots > 5)
        {
            AddIssue(Report, Actor->GetName(), TEXT("Medium DrawCall Count"),
                FString::Printf(TEXT("Actor has %d material slots"), TotalMaterialSlots), 2);
        }
    }
}

void UPerformanceProfiler::AddIssue(FPerformanceReport& Report, const FString& ActorName, 
    const FString& Type, const FString& Description, int32 Severity)
{
    FPerformanceIssue Issue;
    Issue.ActorName = ActorName;
    Issue.IssueType = Type;
    Issue.Description = Description;
    Issue.Severity = Severity;
    
    Report.Issues.Add(Issue);
}

bool UPerformanceProfiler::ExportReport(const FPerformanceReport& Report, const FString& FilePath)
{
    FString Content = TEXT("========================================\n");
    Content += TEXT("   PERFORMANCE ANALYSIS REPORT\n");
    Content += TEXT("========================================\n\n");
    
    Content += FString::Printf(TEXT("Generated: %s\n\n"), *FDateTime::Now().ToString());
    
    // 总览部分
    Content += TEXT("--- OVERVIEW ---\n");
    Content += FString::Printf(TEXT("Total Actors: %d\n"), Report.TotalActors);
    Content += FString::Printf(TEXT("Total Static Meshes: %d\n"), Report.TotalStaticMeshes);
    Content += FString::Printf(TEXT("Total Triangles: %d\n"), Report.TotalTriangles);
    Content += FString::Printf(TEXT("Frame Time: %.2f ms\n"), Report.FrameTimeMS);
    Content += TEXT("\n");
    
    // 问题统计
    Content += TEXT("--- ISSUE SUMMARY ---\n");
    Content += FString::Printf(TEXT("High Poly Meshes: %d\n"), Report.HighPolyMeshCount);
    Content += FString::Printf(TEXT("Meshes Without LODs: %d\n"), Report.MeshesWithoutLODs);
    Content += FString::Printf(TEXT("Total Issues Found: %d\n"), Report.Issues.Num());
    Content += TEXT("\n");
    
    // 按严重程度分类问题
    int32 HighSeverity = 0;
    int32 MediumSeverity = 0;
    int32 LowSeverity = 0;
    
    for (const FPerformanceIssue& Issue : Report.Issues)
    {
        if (Issue.Severity == 3) HighSeverity++;
        else if (Issue.Severity == 2) MediumSeverity++;
        else LowSeverity++;
    }
    
    Content += TEXT("--- SEVERITY BREAKDOWN ---\n");
    Content += FString::Printf(TEXT("High Severity: %d\n"), HighSeverity);
    Content += FString::Printf(TEXT("Medium Severity: %d\n"), MediumSeverity);
    Content += FString::Printf(TEXT("Low Severity: %d\n"), LowSeverity);
    Content += TEXT("\n");
    
    // 详细问题列表
    if (Report.Issues.Num() > 0)
    {
        Content += TEXT("--- DETAILED ISSUES ---\n\n");
        
        // 高严重度问题
        if (HighSeverity > 0)
        {
            Content += TEXT("HIGH SEVERITY:\n");
            for (const FPerformanceIssue& Issue : Report.Issues)
            {
                if (Issue.Severity == 3)
                {
                    Content += FString::Printf(TEXT("  [%s] %s\n"), *Issue.IssueType, *Issue.ActorName);
                    Content += FString::Printf(TEXT("    %s\n\n"), *Issue.Description);
                }
            }
        }
        
        // 中等严重度问题
        if (MediumSeverity > 0)
        {
            Content += TEXT("MEDIUM SEVERITY:\n");
            for (const FPerformanceIssue& Issue : Report.Issues)
            {
                if (Issue.Severity == 2)
                {
                    Content += FString::Printf(TEXT("  [%s] %s\n"), *Issue.IssueType, *Issue.ActorName);
                    Content += FString::Printf(TEXT("    %s\n\n"), *Issue.Description);
                }
            }
        }
        
        // 低严重度问题
        if (LowSeverity > 0)
        {
            Content += TEXT("LOW SEVERITY:\n");
            for (const FPerformanceIssue& Issue : Report.Issues)
            {
                if (Issue.Severity == 1)
                {
                    Content += FString::Printf(TEXT("  [%s] %s\n"), *Issue.IssueType, *Issue.ActorName);
                    Content += FString::Printf(TEXT("    %s\n\n"), *Issue.Description);
                }
            }
        }
    }
    
    // 建议部分
    Content += TEXT("--- RECOMMENDATIONS ---\n");
    if (Report.HighPolyMeshCount > 0)
    {
        Content += TEXT("- Consider reducing polygon count on high-poly meshes\n");
        Content += TEXT("- Use LODs to improve performance at distance\n");
    }
    if (Report.MeshesWithoutLODs > 0)
    {
        Content += TEXT("- Add LOD levels to meshes for better performance\n");
    }
    if (HighSeverity > 0)
    {
        Content += TEXT("- Address high severity issues first for maximum impact\n");
    }
    Content += TEXT("\n");
    
    Content += TEXT("========================================\n");
    Content += TEXT("           END OF REPORT\n");
    Content += TEXT("========================================\n");
    
    // 保存文件
    bool bSuccess = FFileHelper::SaveStringToFile(Content, *FilePath);
    
    if (bSuccess)
    {
        UE_LOG(LogTemp, Log, TEXT("Detailed report exported to: %s"), *FilePath);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to export report to: %s"), *FilePath);
    }
    
    return bSuccess;
}