// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "PerformanceAnalyzerStyle.h"

class FPerformanceAnalyzerCommands : public TCommands<FPerformanceAnalyzerCommands>
{
public:

	FPerformanceAnalyzerCommands()
		: TCommands<FPerformanceAnalyzerCommands>(TEXT("PerformanceAnalyzer"), NSLOCTEXT("Contexts", "PerformanceAnalyzer", "PerformanceAnalyzer Plugin"), NAME_None, FPerformanceAnalyzerStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr<FUICommandInfo> OpenPluginWindow;
	TSharedPtr<FUICommandInfo> AnalyzeScene;
	TSharedPtr<FUICommandInfo> ExportReport;
	
	TSharedPtr<FUICommandInfo> GenerateLODsAuto;      // 自动生成 LOD
	TSharedPtr<FUICommandInfo> GenerateLODsBalanced;  // 使用平衡预设
	TSharedPtr<FUICommandInfo> OpenLODGenerator;      // 打开 LOD 生成器窗口

	TSharedPtr<FUICommandInfo> OpenMaterialOptimizer;      // 打开材质优化器窗口
    TSharedPtr<FUICommandInfo> OptimizeSelectedMaterials;  // 优化选中的材质
    TSharedPtr<FUICommandInfo> ScanSceneMaterials;         // 扫描场景材质
};