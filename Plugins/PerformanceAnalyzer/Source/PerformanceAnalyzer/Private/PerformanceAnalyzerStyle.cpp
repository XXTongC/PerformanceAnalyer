// Copyright Epic Games, Inc. All Rights Reserved.

#include "PerformanceAnalyzerStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Framework/Application/SlateApplication.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleMacros.h"

#define RootToContentDir Style->RootToContentDir

TSharedPtr<FSlateStyleSet> FPerformanceAnalyzerStyle::StyleInstance = nullptr;

void FPerformanceAnalyzerStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FPerformanceAnalyzerStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FPerformanceAnalyzerStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("PerformanceAnalyzerStyle"));
	return StyleSetName;
}

const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);

TSharedRef< FSlateStyleSet > FPerformanceAnalyzerStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("PerformanceAnalyzerStyle"));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("PerformanceAnalyzer")->GetBaseDir() / TEXT("Resources"));

	Style->Set("PerformanceAnalyzer.OpenPluginWindow", new IMAGE_BRUSH_SVG(TEXT("PlaceholderButtonIcon"), Icon20x20));

	return Style;
}

void FPerformanceAnalyzerStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FPerformanceAnalyzerStyle::Get()
{
	return *StyleInstance;
}
