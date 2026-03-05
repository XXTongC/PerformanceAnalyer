#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "PerformanceProfiler.generated.h"

/**
 * 性能问题详情
 */
USTRUCT(BlueprintType)
struct FPerformanceIssue
{
	GENERATED_BODY()
    
	UPROPERTY(BlueprintReadOnly, Category = "Performance")
	FString ActorName;
    
	UPROPERTY(BlueprintReadOnly, Category = "Performance")
	FString IssueType;
    
	UPROPERTY(BlueprintReadOnly, Category = "Performance")
	FString Description;
    
	UPROPERTY(BlueprintReadOnly, Category = "Performance")
	int32 Severity = 0; // 1=Low, 2=Medium, 3=High
};

/**
 * 性能分析报告结构
 */
USTRUCT(BlueprintType)
struct FPerformanceReport
{
	GENERATED_BODY()
    
	UPROPERTY(BlueprintReadOnly, Category = "Performance")
	int32 TotalActors = 0;
    
	UPROPERTY(BlueprintReadOnly, Category = "Performance")
	float FrameTimeMS = 0.0f;
	
	UPROPERTY(BlueprintReadOnly, Category = "Performance")
	int32 TotalStaticMeshes = 0;
	
	UPROPERTY(BlueprintReadOnly, Category = "Performance")
	int32 TotalTriangles = 0;
	
	UPROPERTY(BlueprintReadOnly, Category = "Performance")
	int32 HighPolyMeshCount = 0;
	
	UPROPERTY(BlueprintReadOnly, Category = "Performance")
	int32 MeshesWithoutLODs = 0;
    
	UPROPERTY(BlueprintReadOnly, Category = "Performance")
	TArray<FString> Warnings;
	
	UPROPERTY(BlueprintReadOnly, Category = "Performance")
	TArray<FPerformanceIssue> Issues;
};

/**
 * 性能分析器核心类
 */
UCLASS()
class PERFORMANCEANALYZER_API UPerformanceProfiler : public UObject
{
	GENERATED_BODY()
    
public:
	/** 分析当前场景 */
	UFUNCTION(BlueprintCallable, Category = "Performance")
	static FPerformanceReport AnalyzeCurrentScene();
    
	/** 导出报告到文本文件 */
	UFUNCTION(BlueprintCallable, Category = "Performance")
	static bool ExportReport(const FPerformanceReport& Report, const FString& FilePath);

private:
	// 分析静态网格
	static void AnalyzeStaticMeshes(class UWorld* World, FPerformanceReport& Report);

	// 分析材质
	static void AnalyzeMaterials(class UWorld* World, FPerformanceReport& Report);

	// 检测DrawCall热点
	static void DetectDrawCallHotspots(class UWorld* World, FPerformanceReport& Report);
    
	// 添加性能问题
	static void AddIssue(FPerformanceReport& Report, const FString& ActorName, 
		const FString& Type, const FString& Description, int32 Severity);
};