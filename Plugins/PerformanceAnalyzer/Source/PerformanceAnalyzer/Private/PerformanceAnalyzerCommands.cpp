// Copyright Epic Games, Inc. All Rights Reserved.

#include "PerformanceAnalyzerCommands.h"

#define LOCTEXT_NAMESPACE "FPerformanceAnalyzerModule"

void FPerformanceAnalyzerCommands::RegisterCommands()
{
	UI_COMMAND(OpenPluginWindow, "PerformanceAnalyzer", "Bring up PerformanceAnalyzer window", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(AnalyzeScene, "Analyze Scene", "Analyze current scene performance", EUserInterfaceActionType::Button, FInputChord(EKeys::P, EModifierKey::Control | EModifierKey::Shift));
	UI_COMMAND(ExportReport, "Export Report", "Export performance report", EUserInterfaceActionType::Button, FInputChord());

	UI_COMMAND(GenerateLODsAuto, "Generate LODs (Auto)", "Automatically generate LODs for selected meshes", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control | EModifierKey::Shift, EKeys::L));
	UI_COMMAND(GenerateLODsBalanced, "Generate LODs (Balanced)", "Generate LODs using balanced preset", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(OpenLODGenerator, "LOD Generator", "Open LOD Generator window", EUserInterfaceActionType::Button, FInputChord());

	UI_COMMAND(OpenMaterialOptimizer, "Material Optimizer", "Open Material Optimizer window", EUserInterfaceActionType::Button, FInputChord());
    UI_COMMAND(OptimizeSelectedMaterials, "Optimize Selected Materials", "Convert selected materials to material instances", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control | EModifierKey::Shift, EKeys::M));
    UI_COMMAND(ScanSceneMaterials, "Scan Scene Materials", "Scan and analyze all materials in the scene", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
