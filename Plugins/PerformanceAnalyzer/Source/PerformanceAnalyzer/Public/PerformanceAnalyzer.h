
// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

// 前向声明
class FPerformanceMonitor;
class FToolBarBuilder;
class FMenuBuilder;
class SMaterialOptimizations;

class FPerformanceAnalyzerModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
    
	/** This function will be bound to Command (by default it will bring up plugin window) */
	void PluginButtonClicked();
    
	/** 打开性能监控窗口 */
	void OnOpenMonitorWindow();
    
	/** 打开材质优化器窗口 */
	void OpenMaterialOptimizerWindow();
	
	/** 分析场景性能 */
	void AnalyzeScene();
    
	/** 导出性能报告 */
	void ExportReport();
	
	/** 自动生成 LOD */
	void GenerateLODsAuto();           
	
	/** 使用平衡预设 */
	void GenerateLODsBalanced();     
	
	/** 打开 LOD 生成器窗口 */
	void OpenLODGeneratorWindow();   
	
	/** 优化选中的材质 */
    void OptimizeSelectedMaterials();
	
	/** 扫描场景材质 */
	void ScanSceneMaterials();

private:

	void RegisterMenus();

	TSharedRef<class SDockTab> OnSpawnPluginTab(const class FSpawnTabArgs& SpawnTabArgs);
	TSharedRef<class SDockTab> OnSpawnMonitorTab(const class FSpawnTabArgs& Args);
	TSharedRef<class SDockTab> OnSpawnLODGeneratorTab(const class FSpawnTabArgs& Args);
	TSharedRef<class SDockTab> OnSpawnMaterialOptimizerTab(const class FSpawnTabArgs& Args);
private:
	TSharedPtr<class FUICommandList> PluginCommands;
	TSharedPtr<FPerformanceMonitor> PerformanceMonitor;
};