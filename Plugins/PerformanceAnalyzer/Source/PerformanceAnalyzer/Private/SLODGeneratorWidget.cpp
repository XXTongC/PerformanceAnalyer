
#include "SLODGeneratorWidget.h"
#include "AutoLODGenerator.h"
#include "LODGenerationTransaction.h"

// Slate includes
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Notifications/SProgressBar.h"

// Editor includes
#include "EditorStyleSet.h"
#include "Editor.h"
#include "Selection.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Misc/MessageDialog.h"
//#include "Components/ProgressBar.h"
#define LOCTEXT_NAMESPACE "SLODGeneratorWidget"

SLODGeneratorWidget::~SLODGeneratorWidget()
{
    // 清理事务
    if (CurrentTransaction.IsValid())
    {
        UE_LOG(LogTemp, Log, TEXT("Cleaning up transaction in widget destructor"));
        CurrentTransaction->EndTransaction();
        CurrentTransaction.Reset();
    }
}

void SLODGeneratorWidget::Construct(const FArguments& InArgs)
{
    CurrentPreset = ELODPreset::Balanced;
    CurrentSettings = FAutoLODGenerator::GetPresetSettings(CurrentPreset);
    bIsGenerating = false;
    CurrentMeshIndex = 0;
    TotalMeshes = 0;
    
    // 刷新选中的网格
    RefreshSelectedMeshes();
    
    ChildSlot
    [
        SNew(SVerticalBox)
        
        // ===== 标题栏 =====
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(5.0f)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("Title", "LOD Generator"))
            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
        ]
        
        // ===== 主内容区域 =====
        + SVerticalBox::Slot()
        .FillHeight(2.0f)
        .Padding(10.0f)
        [
            SNew(SSplitter)
            .Orientation(Orient_Horizontal)
            
            // 左侧：设置面板
            + SSplitter::Slot()
            .Value(0.6f)
            [
                SNew(SScrollBox)
                
                // 选中的网格信息
                + SScrollBox::Slot()
                .Padding(10.0f)
                [
                    SNew(SBorder)
                    .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
                    .Padding(10.0f)
                    [
                        SNew(SVerticalBox)
                        
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        [
                            SNew(STextBlock)
                            .Text(LOCTEXT("SelectedMeshes", "Selected Meshes"))
                            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
                        ]
                        
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 5.0f)
                        [
                            SNew(STextBlock)
                            .Text(this, &SLODGeneratorWidget::GetSelectedMeshesText)
                        ]
                        
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 5.0f)
                        [
                            SNew(SButton)
                            .Text(LOCTEXT("RefreshSelection", "Refresh Selection"))
                            .OnClicked(this, &SLODGeneratorWidget::OnRefreshSelectionClicked)
                        ]
                    ]
                ]
                
                // 预设选择
                + SScrollBox::Slot()
                .Padding(10.0f)
                [
                    SNew(SBorder)
                    .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
                    .Padding(10.0f)
                    [
                        SNew(SVerticalBox)
                        
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        [
                            SNew(STextBlock)
                            .Text(LOCTEXT("Presets", "Presets"))
                            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
                        ]
                        
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 5.0f)
                        [
                            SNew(SUniformGridPanel)
                            .SlotPadding(5.0f)
                            
                            // 高质量
                            + SUniformGridPanel::Slot(0, 0)
                            [
                                SNew(SButton)
                                .Text(LOCTEXT("HighQuality", "High Quality"))
                                .ToolTipText(LOCTEXT("HighQualityTooltip", "Conservative reduction, 4 LODs"))
                                .OnClicked(this, &SLODGeneratorWidget::OnPresetButtonClicked, ELODPreset::HighQuality)
                            ]
                            
                            // 平衡
                            + SUniformGridPanel::Slot(1, 0)
                            [
                                SNew(SButton)
                                .Text(LOCTEXT("Balanced", "Balanced"))
                                .ToolTipText(LOCTEXT("BalancedTooltip", "Balanced quality and performance, 4 LODs"))
                                .OnClicked(this, &SLODGeneratorWidget::OnPresetButtonClicked, ELODPreset::Balanced)
                            ]
                            
                            // 性能优先
                            + SUniformGridPanel::Slot(0, 1)
                            [
                                SNew(SButton)
                                .Text(LOCTEXT("Performance", "Performance"))
                                .ToolTipText(LOCTEXT("PerformanceTooltip", "Aggressive reduction, 5 LODs"))
                                .OnClicked(this, &SLODGeneratorWidget::OnPresetButtonClicked, ELODPreset::Performance)
                            ]
                            
                            // 移动端
                            + SUniformGridPanel::Slot(1, 1)
                            [
                                SNew(SButton)
                                .Text(LOCTEXT("Mobile", "Mobile"))
                                .ToolTipText(LOCTEXT("MobileTooltip", "Optimized for mobile, 3 LODs"))
                                .OnClicked(this, &SLODGeneratorWidget::OnPresetButtonClicked, ELODPreset::Mobile)
                            ]
                        ]
                        
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 10.0f, 0.0f, 0.0f)
                        [
                            SAssignNew(PresetDescriptionText, STextBlock)
                            .Text(LOCTEXT("BalancedDesc", "Balanced: 4 LODs with moderate reduction"))
                            .AutoWrapText(true)
                        ]
                    ]
                ]
                
                // 基本设置
                + SScrollBox::Slot()
                .Padding(5.0f)
                [
                    SNew(SBorder)
                    .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
                    .Padding(10.0f)
                    [
                        SNew(SVerticalBox)
                        
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        [
                            SNew(STextBlock)
                            .Text(LOCTEXT("BasicSettings", "Basic Settings"))
                            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
                        ]
                        
                        // LOD 数量
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 5.0f)
                        [
                            SNew(SHorizontalBox)
                            
                            + SHorizontalBox::Slot()
                            .FillWidth(0.4f)
                            [
                                SNew(STextBlock)
                                .Text(LOCTEXT("NumLODs", "Number of LODs:"))
                            ]
                            
                            + SHorizontalBox::Slot()
                            .FillWidth(0.6f)
                            [
                                SAssignNew(NumLODsTextBox, SEditableTextBox)
                                .Text(FText::AsNumber(CurrentSettings.NumLODs))
                                .OnTextChanged(this, &SLODGeneratorWidget::OnNumLODsChanged)
                            ]
                        ]
                        
                        // 重新计算法线
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 5.0f)
                        [
                            SNew(SHorizontalBox)
                            
                            + SHorizontalBox::Slot()
                            .AutoWidth()
                            [
                                SAssignNew(RecalculateNormalsCheckBox, SCheckBox)
                                .IsChecked(CurrentSettings.bRecalculateNormals ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
                                .OnCheckStateChanged(this, &SLODGeneratorWidget::OnRecalculateNormalsChanged)
                            ]
                            
                            + SHorizontalBox::Slot()
                            .Padding(5.0f, 0.0f)
                            [
                                SNew(STextBlock)
                                .Text(LOCTEXT("RecalculateNormals", "Recalculate Normals"))
                            ]
                        ]
                        
                        // 生成光照贴图 UV
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 5.0f)
                        [
                            SNew(SHorizontalBox)
                            
                            + SHorizontalBox::Slot()
                            .AutoWidth()
                            [
                                SAssignNew(GenerateLightmapUVsCheckBox, SCheckBox)
                                .IsChecked(CurrentSettings.bGenerateLightmapUVs ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
                                .OnCheckStateChanged(this, &SLODGeneratorWidget::OnGenerateLightmapUVsChanged)
                            ]
                            
                            + SHorizontalBox::Slot()
                            .Padding(5.0f, 0.0f)
                            [
                                SNew(STextBlock)
                                .Text(LOCTEXT("GenerateLightmapUVs", "Generate Lightmap UVs"))
                            ]
                        ]
                        
                        // 焊接阈值
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 5.0f)
                        [
                            SNew(SHorizontalBox)
                            
                            + SHorizontalBox::Slot()
                            .FillWidth(0.4f)
                            [
                                SNew(STextBlock)
                                .Text(LOCTEXT("WeldingThreshold", "Welding Threshold:"))
                            ]
                            
                            + SHorizontalBox::Slot()
                            .FillWidth(0.6f)
                            [
                                SAssignNew(WeldingThresholdTextBox, SEditableTextBox)
                                .Text(FText::AsNumber(CurrentSettings.WeldingThreshold))
                                .OnTextChanged(this, &SLODGeneratorWidget::OnWeldingThresholdChanged)
                            ]
                        ]
                    ]
                ]
                
                // LOD 级别配置
                + SScrollBox::Slot()
                .Padding(5.0f)
                [
                    SNew(SBorder)
                    .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
                    .Padding(10.0f)
                    [
                        SNew(SVerticalBox)
                        
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        [
                            SNew(STextBlock)
                            .Text(LOCTEXT("LODConfiguration", "LOD Configuration"))
                            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
                        ]
                        
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 5.0f)
                        [
                            RebuildLODConfigUI()
                        ]
                    ]
                ]
            ]
            
            // 右侧：预览面板
            + SSplitter::Slot()
            .Value(0.4f)
            [
                SNew(SBorder)
                .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
                .Padding(10.0f)
                [
                    SNew(SVerticalBox)
                    
                    // 预览标题
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        SNew(STextBlock)
                        .Text(LOCTEXT("Preview", "Preview"))
                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
                    ]
                    
                    // 预览信息
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 5.0f)
                    [
                        SAssignNew(PreviewInfoText, STextBlock)
                        .Text(LOCTEXT("NoPreview", "Click 'Preview' to see LOD generation results"))
                    ]
                    
                    // 预览内容
                    + SVerticalBox::Slot()
                    .FillHeight(1.0f)
                    .Padding(0.0f, 10.0f, 0.0f, 0.0f)
                    [
                        SAssignNew(PreviewScrollBox, SScrollBox)
                        
                        + SScrollBox::Slot()
                        [
                            SAssignNew(PreviewContentBox, SVerticalBox)
                        ]
                    ]
                ]
            ]
        ]
        
        // ===== 底部按钮栏 =====
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(10.0f)
        [
            SNew(SVerticalBox)
            
            // 进度条
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 0.0f, 0.0f, 5.0f)
            [
                SAssignNew(ProgressBar, SProgressBar)
                .Percent(0.0f)
                .Visibility(EVisibility::Collapsed)
            ]
            
            // 进度文本
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 0.0f, 0.0f, 5.0f)
            [
                SAssignNew(ProgressText, STextBlock)
                .Text(LOCTEXT("Processing", "Processing..."))
                .Visibility(EVisibility::Collapsed)
            ]
            
            // 按钮
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(SHorizontalBox)
                
                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                .Padding(0.0f, 0.0f, 5.0f, 0.0f)
                [
                    SAssignNew(PreviewButton, SButton)
                    .Text(LOCTEXT("Preview", "Preview"))
                    .ToolTipText(LOCTEXT("PreviewTooltip", "Preview LOD generation without applying changes"))
                    .OnClicked(this, &SLODGeneratorWidget::OnPreviewClicked)
                ]
                
                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                .Padding(0.0f, 0.0f, 5.0f, 0.0f)
                [
                    SAssignNew(GenerateButton, SButton)
                    .Text(LOCTEXT("Generate", "Generate LODs"))
                    .ToolTipText(LOCTEXT("GenerateTooltip", "Generate LODs for selected meshes"))
                    .OnClicked(this, &SLODGeneratorWidget::OnGenerateClicked)
                ]
                
                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                [
                    SAssignNew(CancelButton, SButton)
                    .Text(LOCTEXT("Cancel", "Cancel"))
                    .ToolTipText(LOCTEXT("CancelTooltip", "Cancel LOD generation"))
                    .OnClicked(this, &SLODGeneratorWidget::OnCancelClicked)
                    .IsEnabled(false)
                ]
            ]
        ]
    ];
    
    // 初始化预设描述
    UpdatePresetDescription(CurrentPreset);
}

// ===== 预设按钮回调 =====

FReply SLODGeneratorWidget::OnPresetButtonClicked(ELODPreset Preset)
{
    CurrentPreset = Preset;
    CurrentSettings = FAutoLODGenerator::GetPresetSettings(Preset);
    
    // 更新 UI
    ApplyPresetToUI(Preset);
    UpdatePresetDescription(Preset);
    
    UE_LOG(LogTemp, Log, TEXT("Applied preset: %d"), (int32)Preset);
    
    return FReply::Handled();
}

// ===== 生成按钮回调 =====

FReply SLODGeneratorWidget::OnGenerateClicked()
{
    if (bIsGenerating)
    {
        return FReply::Handled();
    }
    
    // 验证输入
    FString ErrorMessage;
    if (!ValidateInput(ErrorMessage))
    {
        FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(ErrorMessage));
        return FReply::Handled();
    }
    
    // 刷新选中的网格
    RefreshSelectedMeshes();
    
    if (SelectedMeshes.Num() == 0)
    {
        FMessageDialog::Open(EAppMsgType::Ok, 
            LOCTEXT("NoMeshes", "No meshes selected. Please select Static Mesh Actors in the viewport."));
        return FReply::Handled();
    }
    
    // 收集设置
    CurrentSettings = GatherSettingsFromUI();
    
    // 直接替换旧事务（让旧的自动析构）
    CurrentTransaction.Reset();  // 这会触发旧事务的析构
    CurrentTransaction = MakeUnique<FLODGenerationTransaction>();
    UE_LOG(LogTemp, Log, TEXT("✅ Created new transaction object"));
    
    // 开始生成
    bIsGenerating = true;
    CurrentMeshIndex = 0;
    TotalMeshes = SelectedMeshes.Num();
    
    // 更新 UI 状态
    GenerateButton->SetEnabled(false);
    PreviewButton->SetEnabled(false);
    CancelButton->SetEnabled(true);
    
    ProgressBar->SetVisibility(EVisibility::Visible);
    ProgressText->SetVisibility(EVisibility::Visible);
    
    UE_LOG(LogTemp, Log, TEXT("Starting LOD generation for %d meshes with transaction support"), TotalMeshes);
    
    // 使用事务批量生成
    int32 SuccessCount = FAutoLODGenerator::BatchGenerateLODsWithTransaction(
        SelectedMeshes, 
        CurrentSettings, 
        CurrentTransaction.Get());
    
    int32 FailedCount = TotalMeshes - SuccessCount;
    
    // 完成
    bIsGenerating = false;
    
    // 恢复 UI 状态
    GenerateButton->SetEnabled(true);
    PreviewButton->SetEnabled(true);
    CancelButton->SetEnabled(false);
    ProgressBar->SetVisibility(EVisibility::Collapsed);
    ProgressText->SetVisibility(EVisibility::Collapsed);
    
    UE_LOG(LogTemp, Log, TEXT("Transaction kept active for Undo support"));
    
    // 显示结果
    FString ResultMessage = FString::Printf(
        TEXT("LOD Generation Complete!\n\n")
        TEXT("Total Meshes: %d\n")
        TEXT("Successfully Generated: %d\n")
        TEXT("Failed: %d\n\n")
        TEXT("You can use Ctrl+Z to undo this operation."),
        TotalMeshes,
        SuccessCount,
        FailedCount
    );
    
    FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(ResultMessage));
    
    UE_LOG(LogTemp, Warning, TEXT("LOD Generation Complete: %d succeeded, %d failed"), 
        SuccessCount, FailedCount);
    
    // 更新预览
    UpdatePreview();
    
    // 清理事务
    //CurrentTransaction.Reset();
    
    return FReply::Handled();
}

FReply SLODGeneratorWidget::OnPreviewClicked()
{
    // 刷新选中的网格
    RefreshSelectedMeshes();
    
    if (SelectedMeshes.Num() == 0)
    {
        PreviewInfoText->SetText(LOCTEXT("NoMeshesSelected", "No meshes selected"));
        PreviewContentBox->ClearChildren();
        return FReply::Handled();
    }
    
    // 收集设置
    CurrentSettings = GatherSettingsFromUI();
    
    // 清空预览内容
    PreviewContentBox->ClearChildren();
    
    // 生成预览信息
    int32 TotalOriginalTriangles = 0;
    int32 TotalEstimatedTriangles = 0;
    
    for (UStaticMesh* Mesh : SelectedMeshes)
    {
        if (!Mesh)
        {
            continue;
        }
        
        FLODGenerationResult PreviewResult = FAutoLODGenerator::PreviewLODGeneration(Mesh, CurrentSettings);
        
        if (PreviewResult.bSuccess)
        {
            int32 OriginalTris = PreviewResult.LODTriangleCounts[0];
            TotalOriginalTriangles += OriginalTris;
            
            // 添加分隔线
            PreviewContentBox->AddSlot()
            .AutoHeight()
            .Padding(0.0f, 5.0f)
            [
                SNew(SSeparator)
            ];
            
            // 添加网格名称
            PreviewContentBox->AddSlot()
            .AutoHeight()
            [
                SNew(STextBlock)
                .Text(FText::FromString(FString::Printf(TEXT("Mesh: %s"), *Mesh->GetName())))
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
            ];
            
            
            // 原始三角形数
            PreviewContentBox->AddSlot()
            .AutoHeight()
            [
                SNew(STextBlock)
                .Text(FText::FromString(FString::Printf(TEXT("Original Triangles: %d"), OriginalTris)))
            ];
            
            // 添加每个 LOD 的信息
            for (int32 i = 1; i < PreviewResult.LODTriangleCounts.Num(); ++i)
            {
                int32 LODTris = PreviewResult.LODTriangleCounts[i];
                float Reduction = PreviewResult.ReductionRatios[i - 1];
                
                PreviewContentBox->AddSlot()
                .AutoHeight()
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(FString::Printf(
                        TEXT("  LOD%d: %d tris (%.1f%%)"), 
                        i, LODTris, Reduction * 100.0f)))
                ];
                
                if (i == 1)
                {
                    TotalEstimatedTriangles += LODTris;
                }
            }
            
            // 性能提升
            PreviewContentBox->AddSlot()
            .AutoHeight()
            [
                SNew(STextBlock)
                .Text(FText::FromString(FString::Printf(
                    TEXT("Performance Gain: %.1f%%"), 
                    PreviewResult.EstimatedPerformanceGain * 100.0f)))
                .ColorAndOpacity(FLinearColor::Green)
            ];
        }
    }
    
    // 添加总计
    PreviewContentBox->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 10.0f, 0.0f, 0.0f)
    [
        SNew(SSeparator)
    ];
    
    PreviewContentBox->AddSlot()
    .AutoHeight()
    [
        SNew(STextBlock)
        .Text(LOCTEXT("Summary", "SUMMARY"))
        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
    ];
    
    PreviewContentBox->AddSlot()
    .AutoHeight()
    [
        SNew(STextBlock)
        .Text(FText::FromString(FString::Printf(TEXT("Total Meshes: %d"), SelectedMeshes.Num())))
    ];
    
    PreviewContentBox->AddSlot()
    .AutoHeight()
    [
        SNew(STextBlock)
        .Text(FText::FromString(FString::Printf(TEXT("Total Original Triangles: %d"), TotalOriginalTriangles)))
    ];
    
    PreviewContentBox->AddSlot()
    .AutoHeight()
    [
        SNew(STextBlock)
        .Text(FText::FromString(FString::Printf(TEXT("Estimated LOD1 Triangles: %d"), TotalEstimatedTriangles)))
    ];
    
    if (TotalOriginalTriangles > 0)
    {
        float TotalReduction = 1.0f - ((float)TotalEstimatedTriangles / (float)TotalOriginalTriangles);
        PreviewContentBox->AddSlot()
        .AutoHeight()
        [
            SNew(STextBlock)
            .Text(FText::FromString(FString::Printf(TEXT("Overall Reduction: %.1f%%"), TotalReduction * 100.0f)))
            .ColorAndOpacity(FLinearColor::Green)
        ];
    }
    
    // 更新预览信息文本
    PreviewInfoText->SetText(FText::Format(
        LOCTEXT("PreviewInfo", "Preview for {0} meshes with {1} LODs"),
        FText::AsNumber(SelectedMeshes.Num()),
        FText::AsNumber(CurrentSettings.NumLODs)
    ));
    
    UE_LOG(LogTemp, Log, TEXT("Preview generated for %d meshes"), SelectedMeshes.Num());
    
    return FReply::Handled();
}

FReply SLODGeneratorWidget::OnCancelClicked()
{
    if (bIsGenerating)
    {
        bIsGenerating = false;
        
        // 取消事务（撤销已生成的 LOD）
        if (CurrentTransaction.IsValid())
        {
            CurrentTransaction->CancelTransaction();
            CurrentTransaction.Reset();
        }
        
        // 恢复 UI 状态
        GenerateButton->SetEnabled(true);
        PreviewButton->SetEnabled(true);
        CancelButton->SetEnabled(false);
        ProgressBar->SetVisibility(EVisibility::Collapsed);
        ProgressText->SetVisibility(EVisibility::Collapsed);
        
        FMessageDialog::Open(EAppMsgType::Ok, 
            LOCTEXT("Cancelled", "LOD generation cancelled. All changes have been reverted."));
        
        UE_LOG(LogTemp, Warning, TEXT("LOD generation cancelled by user"));
    }
    
    return FReply::Handled();
}

FReply SLODGeneratorWidget::OnRefreshSelectionClicked()
{
    RefreshSelectedMeshes();
    UpdatePreview();
    
    UE_LOG(LogTemp, Log, TEXT("Selection refreshed: %d meshes"), SelectedMeshes.Num());
    
    return FReply::Handled();
}

// ===== 设置更改回调 =====

void SLODGeneratorWidget::OnNumLODsChanged(const FText& Text)
{
    int32 NewNumLODs = FCString::Atoi(*Text.ToString());
    NewNumLODs = FMath::Clamp(NewNumLODs, 1, 8);
    
    if (NewNumLODs != CurrentSettings.NumLODs)
    {
        CurrentSettings.NumLODs = NewNumLODs;
        
        // 调整数组大小
        CurrentSettings.ReductionPercentages.SetNum(NewNumLODs);
        CurrentSettings.ScreenSizes.SetNum(NewNumLODs);
        
        // 重建 LOD 配置 UI
        // 注意：这里需要触发 UI 重建，实际实现中可能需要更复杂的逻辑
        UE_LOG(LogTemp, Log, TEXT("Number of LODs changed to: %d"), NewNumLODs);
    }
}

void SLODGeneratorWidget::OnReductionPercentageChanged(const FText& Text, int32 LODIndex)
{
    float Value = FCString::Atof(*Text.ToString()) / 100.0f;
    if (LODIndex < CurrentSettings.ReductionPercentages.Num())
    {
        CurrentSettings.ReductionPercentages[LODIndex] = FMath::Clamp(Value, 0.01f, 1.0f);
    }
}

void SLODGeneratorWidget::OnScreenSizeChanged( const FText& Text,int32 LODIndex)
{
    float Value = FCString::Atof(*Text.ToString());
    if (LODIndex < CurrentSettings.ScreenSizes.Num())
    {
        CurrentSettings.ScreenSizes[LODIndex] = FMath::Clamp(Value, 0.001f, 1.0f);
    }
}

void SLODGeneratorWidget::OnRecalculateNormalsChanged(ECheckBoxState NewState)
{
    CurrentSettings.bRecalculateNormals = (NewState == ECheckBoxState::Checked);
}

void SLODGeneratorWidget::OnGenerateLightmapUVsChanged(ECheckBoxState NewState)
{
    CurrentSettings.bGenerateLightmapUVs = (NewState == ECheckBoxState::Checked);
}

void SLODGeneratorWidget::OnWeldingThresholdChanged(const FText& Text)
{
    CurrentSettings.WeldingThreshold = FCString::Atof(*Text.ToString());
}

// ===== 辅助函数 =====

void SLODGeneratorWidget::RefreshSelectedMeshes()
{
    SelectedMeshes.Empty();
    
    if (!GEditor)
    {
        return;
    }
    
    // 获取选中的 Actor
    USelection* SelectedActors = GEditor->GetSelectedActors();
    if (!SelectedActors)
    {
        return;
    }
    
    // 遍历选中的 Actor
    for (FSelectionIterator It(*SelectedActors); It; ++It)
    {
        AActor* Actor = Cast<AActor>(*It);
        if (!Actor)
        {
            continue;
        }
        
        // 检查是否是 StaticMeshActor
        AStaticMeshActor* StaticMeshActor = Cast<AStaticMeshActor>(Actor);
        if (StaticMeshActor)
        {
            UStaticMeshComponent* MeshComponent = StaticMeshActor->GetStaticMeshComponent();
            if (MeshComponent && MeshComponent->GetStaticMesh())
            {
                SelectedMeshes.AddUnique(MeshComponent->GetStaticMesh());
            }
        }
        else
        {
            // 检查 Actor 的所有 StaticMeshComponent
            TArray<UStaticMeshComponent*> MeshComponents;
            Actor->GetComponents<UStaticMeshComponent>(MeshComponents);
            
            for (UStaticMeshComponent* MeshComponent : MeshComponents)
            {
                if (MeshComponent && MeshComponent->GetStaticMesh())
                {
                    SelectedMeshes.AddUnique(MeshComponent->GetStaticMesh());
                }
            }
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("Refreshed selection: %d unique meshes found"), SelectedMeshes.Num());
}

void SLODGeneratorWidget::UpdatePresetDescription(ELODPreset Preset)
{
    FText Description;
    
    switch (Preset)
    {
        case ELODPreset::HighQuality:
            Description = LOCTEXT("HighQualityDesc",
                "High Quality: Conservative reduction with 4 LODs\n"
                "Best for hero assets and close-up objects\n"
                "LOD1: 70%, LOD2: 50%, LOD3: 30%, LOD4: 15%");
            break;
            
        case ELODPreset::Balanced:
            Description = LOCTEXT("BalancedDesc",
                "Balanced: Moderate reduction with 4 LODs\n"
                "Good balance between quality and performance\n"
                "LOD1: 50%, LOD2: 25%, LOD3: 12.5%, LOD4: 6.25%");
            break;
            
        case ELODPreset::Performance:
            Description = LOCTEXT("PerformanceDesc",
                "Performance: Aggressive reduction with 5 LODs\n"
                "Best for background objects and large scenes\n"
                "LOD1: 40%, LOD2: 20%, LOD3: 10%, LOD4: 5%, LOD5: 2.5%");
            break;
            
        case ELODPreset::Mobile:
            Description = LOCTEXT("MobileDesc",
                "Mobile: Optimized for mobile platforms with 3 LODs\n"
                "Reduced LOD count for better mobile performance\n"
                "LOD1: 50%, LOD2: 25%, LOD3: 10%");
            break;
            
        case ELODPreset::Custom:
        default:
            Description = LOCTEXT("CustomDesc",
                "Custom: Manual configuration\n"
                "Configure LOD settings manually");
            break;
    }
    
    
    if (PresetDescriptionText.IsValid())
    {
        PresetDescriptionText->SetText(Description);
    }
}

void SLODGeneratorWidget::ApplyPresetToUI(ELODPreset Preset)
{
    // 更新 LOD 数量
    if (NumLODsTextBox.IsValid())
    {
        NumLODsTextBox->SetText(FText::AsNumber(CurrentSettings.NumLODs));
    }
    
    // 更新复选框
    if (RecalculateNormalsCheckBox.IsValid())
    {
        RecalculateNormalsCheckBox->SetIsChecked(
            CurrentSettings.bRecalculateNormals ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
    }
    
    if (GenerateLightmapUVsCheckBox.IsValid())
    {
        GenerateLightmapUVsCheckBox->SetIsChecked(
            CurrentSettings.bGenerateLightmapUVs ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
    }
    
    // 更新焊接阈值
    if (WeldingThresholdTextBox.IsValid())
    {
        WeldingThresholdTextBox->SetText(FText::AsNumber(CurrentSettings.WeldingThreshold));
    }
    
    // 更新 LOD 配置（如果已创建）
    for (int32 i = 0; i < ReductionTextBoxes.Num() && i < CurrentSettings.ReductionPercentages.Num(); ++i)
    {
        if (ReductionTextBoxes[i].IsValid())
        {
            ReductionTextBoxes[i]->SetText(
                FText::AsNumber(CurrentSettings.ReductionPercentages[i] * 100.0f));
        }
    }
    
    for (int32 i = 0; i < ScreenSizeTextBoxes.Num() && i < CurrentSettings.ScreenSizes.Num(); ++i)
    {
        if (ScreenSizeTextBoxes[i].IsValid())
        {
            ScreenSizeTextBoxes[i]->SetText(
                FText::AsNumber(CurrentSettings.ScreenSizes[i]));
        }
    }
}

FLODGenerationSettings SLODGeneratorWidget::GatherSettingsFromUI()
{
    FLODGenerationSettings Settings = CurrentSettings;
    
    // 从 UI 收集数值
    if (NumLODsTextBox.IsValid())
    {
        Settings.NumLODs = FCString::Atoi(*NumLODsTextBox->GetText().ToString());
        Settings.NumLODs = FMath::Clamp(Settings.NumLODs, 1, 8);
    }
    
    if (RecalculateNormalsCheckBox.IsValid())
    {
        Settings.bRecalculateNormals = 
            (RecalculateNormalsCheckBox->GetCheckedState() == ECheckBoxState::Checked);
    }
    
    if (GenerateLightmapUVsCheckBox.IsValid())
    {
        Settings.bGenerateLightmapUVs = 
            (GenerateLightmapUVsCheckBox->GetCheckedState() == ECheckBoxState::Checked);
    }
    
    if (WeldingThresholdTextBox.IsValid())
    {
        Settings.WeldingThreshold = FCString::Atof(*WeldingThresholdTextBox->GetText().ToString());
    }
    
    // 收集 LOD 配置
    for (int32 i = 0; i < ReductionTextBoxes.Num() && i < Settings.ReductionPercentages.Num(); ++i)
    {
        if (ReductionTextBoxes[i].IsValid())
        {
            float Value = FCString::Atof(*ReductionTextBoxes[i]->GetText().ToString()) / 100.0f;
            Settings.ReductionPercentages[i] = FMath::Clamp(Value, 0.01f, 1.0f);
        }
    }
    
    for (int32 i = 0; i < ScreenSizeTextBoxes.Num() && i < Settings.ScreenSizes.Num(); ++i)
    {
        if (ScreenSizeTextBoxes[i].IsValid())
        {
            float Value = FCString::Atof(*ScreenSizeTextBoxes[i]->GetText().ToString());
            Settings.ScreenSizes[i] = FMath::Clamp(Value, 0.001f, 1.0f);
        }
    }
    
    return Settings;
}

void SLODGeneratorWidget::UpdatePreview()
{
    // 自动触发预览更新
    OnPreviewClicked();
}

TSharedRef<SWidget> SLODGeneratorWidget::RebuildLODConfigUI()
{
    // 清空旧的引用
    ReductionTextBoxes.Empty();
    ScreenSizeTextBoxes.Empty();
    
    TSharedRef<SVerticalBox> ConfigBox = SNew(SVerticalBox);
    
    // 为每个 LOD 创建配置行
    for (int32 i = 0; i < CurrentSettings.NumLODs; ++i)
    {
        // LOD 标题
        ConfigBox->AddSlot()
        .AutoHeight()
        .Padding(0.0f, 5.0f, 0.0f, 2.0f)
        [
            SNew(STextBlock)
            .Text(FText::FromString(FString::Printf(TEXT("LOD %d"), i)))
            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
        ];
        
        // 简化百分比 - 使用 Lambda 修复绑定
        TSharedPtr<SEditableTextBox> ReductionBox;
        const int32 LODIndex = i; // 捕获当前索引
        
        ConfigBox->AddSlot()
        .AutoHeight()
        .Padding(10.0f, 2.0f, 0.0f, 2.0f)
        [
            SNew(SHorizontalBox)
            
            + SHorizontalBox::Slot()
            .FillWidth(0.5f)
            [
                SNew(STextBlock)
                .Text(LOCTEXT("Reduction", "Reduction %:"))
            ]
            
            + SHorizontalBox::Slot()
            .FillWidth(0.5f)
            [
                SAssignNew(ReductionBox, SEditableTextBox)
                .Text(FText::AsNumber(
                    i < CurrentSettings.ReductionPercentages.Num() 
                    ? CurrentSettings.ReductionPercentages[i] * 100.0f 
                    : 50.0f))
                .OnTextChanged_Lambda([this, LODIndex](const FText& Text)
                {
                    this->OnReductionPercentageChanged(Text, LODIndex);
                })
            ]
        ];
        ReductionTextBoxes.Add(ReductionBox);
        
        // 屏幕尺寸 - 使用 Lambda 修复绑定
        TSharedPtr<SEditableTextBox> ScreenSizeBox;
        
        ConfigBox->AddSlot()
        .AutoHeight()
        .Padding(10.0f, 2.0f, 0.0f, 2.0f)
        [
            SNew(SHorizontalBox)
            
            + SHorizontalBox::Slot()
            .FillWidth(0.5f)
            [
                SNew(STextBlock)
                .Text(LOCTEXT("ScreenSize", "Screen Size:"))
            ]
            
            + SHorizontalBox::Slot()
.FillWidth(0.5f)
[
    SAssignNew(ScreenSizeBox, SEditableTextBox)
    .Text(FText::AsNumber(
        i < CurrentSettings.ScreenSizes.Num() 
        ? CurrentSettings.ScreenSizes[i] 
        : 0.5f))
    .OnTextChanged_Lambda([this, LODIndex](const FText& Text)
    {
        this->OnScreenSizeChanged(Text, LODIndex);
    })
]
];
        ScreenSizeTextBoxes.Add(ScreenSizeBox);
    }
    
    return ConfigBox;
}

void SLODGeneratorWidget::UpdateProgress(int32 Current, int32 Total, const FString& CurrentMeshName)
{
    if (Total <= 0)
    {
        return;
    }
    
    float Progress = (float)Current / (float)Total;
    
    if (ProgressBar.IsValid())
    {
        ProgressBar->SetPercent(Progress);
    }
    
    if (ProgressText.IsValid())
    {
        FString ProgressString = FString::Printf(
            TEXT("Processing %d/%d: %s"),
            Current,
            Total,
            *CurrentMeshName
        );
        ProgressText->SetText(FText::FromString(ProgressString));
    }
}

bool SLODGeneratorWidget::ValidateInput(FString& OutErrorMessage)
{
    // 验证 LOD 数量
    if (CurrentSettings.NumLODs < 1 || CurrentSettings.NumLODs > 8)
    {
        OutErrorMessage = TEXT("Number of LODs must be between 1 and 8");
        return false;
    }
    
    // 验证简化百分比
    for (int32 i = 0; i < CurrentSettings.ReductionPercentages.Num(); ++i)
    {
        if (CurrentSettings.ReductionPercentages[i] < 0.01f || 
            CurrentSettings.ReductionPercentages[i] > 1.0f)
        {
            OutErrorMessage = FString::Printf(
                TEXT("LOD %d reduction percentage must be between 1%% and 100%%"), i);
            return false;
        }
    }
    
    // 验证屏幕尺寸
    for (int32 i = 0; i < CurrentSettings.ScreenSizes.Num(); ++i)
    {
        if (CurrentSettings.ScreenSizes[i] < 0.001f || 
            CurrentSettings.ScreenSizes[i] > 1.0f)
        {
            OutErrorMessage = FString::Printf(
                TEXT("LOD %d screen size must be between 0.001 and 1.0"), i);
            return false;
        }
    }
    
    // 验证焊接阈值
    if (CurrentSettings.WeldingThreshold < 0.0f || CurrentSettings.WeldingThreshold > 10.0f)
    {
        OutErrorMessage = TEXT("Welding threshold must be between 0.0 and 10.0");
        return false;
    }
    
    return true;
}

FText SLODGeneratorWidget::GetSelectedMeshesText() const
{
    if (SelectedMeshes.Num() == 0)
    {
        return LOCTEXT("NoMeshesSelected", "No meshes selected");
    }
    return FText::Format(
        LOCTEXT("MeshesSelectedText", "{0} unique mesh(es) selected"),
        FText::AsNumber(SelectedMeshes.Num())
    );
}



#undef LOCTEXT_NAMESPACE