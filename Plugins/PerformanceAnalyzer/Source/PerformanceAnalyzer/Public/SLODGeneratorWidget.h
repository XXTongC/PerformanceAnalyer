
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "AutoLODGenerator.h"
#include "LODGenerationTransaction.h"

class SEditableTextBox;
class SCheckBox;
class SButton;
class STextBlock;
class SProgressBar;
class SScrollBox;
class SVerticalBox;

/**
 * LOD 生成器 UI 界面
 */
class PERFORMANCEANALYZER_API SLODGeneratorWidget : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SLODGeneratorWidget) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    virtual ~SLODGeneratorWidget() override;
private:
    // ===== UI 组件 =====
    
    // 预设选择
    TSharedPtr<STextBlock> PresetDescriptionText;
    
    // 自定义设置
    TSharedPtr<SEditableTextBox> NumLODsTextBox;
    TSharedPtr<SCheckBox> RecalculateNormalsCheckBox;
    TSharedPtr<SCheckBox> GenerateLightmapUVsCheckBox;
    TSharedPtr<SEditableTextBox> WeldingThresholdTextBox;
    
    // LOD 级别配置（动态生成）
    TArray<TSharedPtr<SEditableTextBox>> ReductionTextBoxes;
    TArray<TSharedPtr<SEditableTextBox>> ScreenSizeTextBoxes;
    
    // 预览信息
    TSharedPtr<STextBlock> PreviewInfoText;
    TSharedPtr<SVerticalBox> PreviewContentBox;  // 改用 SVerticalBox
    TSharedPtr<SScrollBox> PreviewScrollBox;     // 包裹在 ScrollBox 中
    
    // 进度条
    TSharedPtr<SProgressBar> ProgressBar;
    TSharedPtr<STextBlock> ProgressText;
    
    // 按钮
    TSharedPtr<SButton> GenerateButton;
    TSharedPtr<SButton> PreviewButton;
    TSharedPtr<SButton> CancelButton;
    
    // ===== 数据 =====
    
    // 当前选择的预设
    ELODPreset CurrentPreset;
    
    // 当前设置
    FLODGenerationSettings CurrentSettings;
    
    // 选中的网格
    TArray<UStaticMesh*> SelectedMeshes;
    
    // 生成状态
    bool bIsGenerating;
    int32 CurrentMeshIndex;
    int32 TotalMeshes;
    
    // 事务支持
    TUniquePtr<class FLODGenerationTransaction> CurrentTransaction;
    
    // ===== UI 回调函数 =====
    
    // 预设选择
    FReply OnPresetButtonClicked(ELODPreset Preset);
    
    // 生成按钮
    FReply OnGenerateClicked();
    FReply OnPreviewClicked();
    FReply OnCancelClicked();
    FReply OnRefreshSelectionClicked();
    
    // 设置更改
    void OnNumLODsChanged(const FText& Text);
    void OnReductionPercentageChanged(const FText& Text,int32 LODIndex);
    void OnScreenSizeChanged(const FText& Text,int32 LODIndex);
    void OnRecalculateNormalsChanged(ECheckBoxState NewState);
    void OnGenerateLightmapUVsChanged(ECheckBoxState NewState);
    void OnWeldingThresholdChanged(const FText& Text);
    
    // ===== 辅助函数 =====
    
    // 刷新选中的网格列表
    void RefreshSelectedMeshes();
    
    // 更新预设描述
    void UpdatePresetDescription(ELODPreset Preset);
    
    // 应用预设到 UI
    void ApplyPresetToUI(ELODPreset Preset);
    
    // 从 UI 收集设置
    FLODGenerationSettings GatherSettingsFromUI();
    
    // 更新预览信息
    void UpdatePreview();
    
    // 重建 LOD 配置 UI
    TSharedRef<SWidget> RebuildLODConfigUI();
    
    // 更新进度
    void UpdateProgress(int32 Current, int32 Total, const FString& CurrentMeshName);
    
    // 验证输入
    bool ValidateInput(FString& OutErrorMessage);
    
    // 获取选中网格文本
    FText GetSelectedMeshesText() const;
};